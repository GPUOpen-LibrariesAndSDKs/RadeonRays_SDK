#include "SceneGraph/scene1.h"
#include "SceneGraph/camera.h"
#include "SceneGraph/light.h"
#include "SceneGraph/shape.h"
#include "SceneGraph/material.h"
#include "SceneGraph/texture.h"
#include "SceneGraph/Collector/collector.h"
#include "iterator.h"

#include <chrono>
#include <memory>
#include <stack>
#include <vector>
#include <array>

namespace Baikal
{
    template <typename CompiledScene>
    inline
    void SceneController<CompiledScene>::SplitMeshesAndInstances(Iterator* shape_iter, std::set<Mesh const*>& meshes, std::set<Instance const*>& instances, std::set<Mesh const*>& excluded_meshes)
    {
        // Clear all sets
        meshes.clear();
        instances.clear();
        excluded_meshes.clear();

        // Prepare instance check lambda
        auto is_instance = [](Shape const* shape)
        {
            if (dynamic_cast<Instance const*>(shape))
            {
                return true;
            }
            else
            {
                return false;
            }
        };

        for (; shape_iter->IsValid(); shape_iter->Next())
        {
            auto shape = shape_iter->ItemAs<Shape const>();

            if (!is_instance(shape))
            {
                meshes.emplace(static_cast<Mesh const*>(shape));
            }
            else
            {
                instances.emplace(static_cast<Instance const*>(shape));
            }
        }

        for (auto& i : instances)
        {
            auto base_mesh = static_cast<Mesh const*>(i->GetBaseShape());
            if (meshes.find(base_mesh) == meshes.cend())
            {
                excluded_meshes.emplace(base_mesh);
            }
        }
    }

    template <typename CompiledScene>
    inline
    SceneController<CompiledScene>::SceneController()
    {
    }
    
    template <typename CompiledScene>
    inline
    CompiledScene& SceneController<CompiledScene>::CompileScene(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, bool clear_ditry_flags) const
    {
        // The overall approach is:
        // 1) Check if materials have changed, update collector if yes
        // 2) Check if textures have changed, update collector if yes
        // Note, that material are collected from shapes (potentially recursively).
        // Textures are collected from materials and lights.
        // As soon as we updated both collectors we have established
        // correct pointer to buffer index mapping for both materials and textures.
        // As soon as we have this mapping we are analyzing dirty flags and
        // updating necessary parts.
        
        // We need to make sure collectors are empty before proceeding
        mat_collector.Clear();
        tex_collector.Clear();
        
        // Create shape and light iterators
        std::unique_ptr<Iterator> shape_iter(scene.CreateShapeIterator());
        std::unique_ptr<Iterator> light_iter(scene.CreateLightIterator());
        
        auto default_material = GetDefaultMaterial();
        // Collect materials from shapes first
        mat_collector.Collect(shape_iter.get(),
                              // This function adds all materials to resulting map
                              // recursively via Material dependency API
                              [default_material](void const* item) -> std::set<void const*>
                              {
                                  // Resulting material set
                                  std::set<void const*> mats;
                                  // Material stack
                                  std::stack<Material const*> material_stack;
                                  
                                  // Get material from current shape
                                  auto shape = reinterpret_cast<Shape const*>(item);
                                  auto material = shape->GetMaterial();
                                  
                                  // If shape does not have a material, use default one
                                  if (!material)
                                  {
                                      material = default_material;
                                  }
                                  
                                  // Push to stack as an initializer
                                  material_stack.push(material);
                                  
                                  // Drain the stack
                                  while (!material_stack.empty())
                                  {
                                      // Get current material
                                      Material const* m = material_stack.top();
                                      material_stack.pop();
                                      
                                      // Emplace into the set
                                      mats.emplace(m);
                                      
                                      // Create dependency iterator
                                      std::unique_ptr<Iterator> mat_iter(m->CreateMaterialIterator());
                                      
                                      // Push all dependencies into the stack
                                      for (; mat_iter->IsValid(); mat_iter->Next())
                                      {
                                          material_stack.push(mat_iter->ItemAs<Material const>());
                                      }
                                  }
                                  
                                  // Return resulting set
                                  return mats;
                              });
        
        // Commit stuff (we can iterate over it after commit has happened)
        mat_collector.Commit();
        
        // Now we need to collect textures from our materials
        // Create material iterator
        std::unique_ptr<Iterator> mat_iter(mat_collector.CreateIterator());
        
        // Collect textures from materials
        tex_collector.Collect(mat_iter.get(),
                              [](void const* item) -> std::set<void const*>
                              {
                                  // Texture set
                                  std::set<void const*> textures;
                                  
                                  auto material = reinterpret_cast<Material const*>(item);
                                  
                                  // Create texture dependency iterator
                                  std::unique_ptr<Iterator> tex_iter(material->CreateTextureIterator());
                                  
                                  // Emplace all dependent textures
                                  for (; tex_iter->IsValid(); tex_iter->Next())
                                  {
                                      textures.emplace(tex_iter->ItemAs<Texture const>());
                                  }
                                  
                                  // Return resulting set
                                  return textures;
                              });
        
        
        // Collect textures from lights
        tex_collector.Collect(light_iter.get(),
                              [](void const* item) -> std::set<void const*>
                              {
                                  // Resulting set
                                  std::set<void const*> textures;
                                  
                                  auto light = reinterpret_cast<Light const*>(item);
                                  
                                  // Create texture dependency iterator
                                  std::unique_ptr<Iterator> tex_iter(light->CreateTextureIterator());
                                  
                                  // Emplace all dependent textures
                                  for (; tex_iter->IsValid(); tex_iter->Next())
                                  {
                                      textures.emplace(tex_iter->ItemAs<Texture const>());
                                  }
                                  
                                  // Return resulting set
                                  return textures;
                              });
        
        // Commit textures
        tex_collector.Commit();
        
        // Try to find scene in cache first
        auto iter = m_scene_cache.find(&scene);
        
        if (iter == m_scene_cache.cend())
        {
            // If not found create scene entry in cache
            auto res = m_scene_cache.emplace(std::make_pair(&scene, CompiledScene()));
            
            // Recompile all the stuff into cached scene
            RecompileFull(scene, mat_collector, tex_collector, res.first->second);
            
            // Set scene as current
            m_current_scene = &scene;
            
            // Drop all dirty flags for the scene

            if (clear_ditry_flags)
            {
                scene.ClearDirtyFlags();
            }

            // Return the scene
            return res.first->second;
        }
        else
        {
            // Exctract cached scene entry
            auto& out = iter->second;
            auto dirty = scene.GetDirtyFlags();

            // Check if we have valid camera
            auto camera = scene.GetCamera();

            if (!camera)
            {
                throw std::runtime_error("No camera in the scene");
            }

            // Check if camera parameters have been changed
            auto camera_changed = camera->IsDirty();

            // Update camera if needed
            if (dirty & Scene1::kCamera || camera_changed)
            {
                UpdateCamera(scene, mat_collector, tex_collector, out);

                if (clear_ditry_flags)
                {
                    camera->SetDirty(false);
                }
            }
            
            {
                // Check if we have lights in the scene
                std::unique_ptr<Iterator> light_iter(scene.CreateLightIterator());
                
                if (!light_iter->IsValid())
                {
                    throw std::runtime_error("No lights in the scene");
                }
                
                
                // Check if light parameters have been changed
                bool lights_changed = false;
                
                for (; light_iter->IsValid(); light_iter->Next())
                {
                    auto light = light_iter->ItemAs<Light const>();
                    
                    if (light->IsDirty())
                    {
                        lights_changed = true;
                        break;
                    }
                }
                
                
                // Update lights if needed
                if (dirty & Scene1::kLights || lights_changed)
                {
                    UpdateLights(scene, mat_collector, tex_collector, out);

                    if (clear_ditry_flags)
                    {
                        DropDirties(light_iter.get());
                    }
                }
            }
            
            {
                // Check if we have shapes in the scene
                std::unique_ptr<Iterator> shape_iter(scene.CreateShapeIterator());
                
                if (!shape_iter->IsValid())
                {
                    throw std::runtime_error("No shapes in the scene");
                }
                
                // Check if shape parameters have been changed
                bool shapes_changed = false;
                
                for (; shape_iter->IsValid(); shape_iter->Next())
                {
                    auto shape = shape_iter->ItemAs<Shape const>();
                    
                    if (shape->IsDirty())
                    {
                        shapes_changed = true;
                        break;
                    }
                }
                
                // Update shapes if needed
                if (dirty & Scene1::kShapes || shapes_changed)
                {
                    UpdateShapes(scene, mat_collector, tex_collector, out);

                    if (clear_ditry_flags)
                    {
                        DropDirties(shape_iter.get());
                    }
                }
            }
            
            // If materials need an update, do it.
            // We are passing material dirty state detection function in there.
            if (!out.material_bundle ||
                mat_collector.NeedsUpdate(out.material_bundle.get(),
                                          [](void const* ptr)->bool
                                          {
                                              auto mat = reinterpret_cast<Material const*>(ptr);
                                              return mat->IsDirty();
                                          }
                                          ))
            {
                // Update material bundle first to be able to track differences
                out.material_bundle.reset(mat_collector.CreateBundle());

                UpdateMaterials(scene, mat_collector, tex_collector, out);

                if (clear_ditry_flags)
                {
                    std::unique_ptr<Iterator> mat_iter(mat_collector.CreateIterator());
                    DropDirties(mat_iter.get());
                }
            }
            
            // If textures need an update, do it.
            if (tex_collector.GetNumItems() > 0 && (
                                                    !out.texture_bundle ||
                                                    tex_collector.NeedsUpdate(out.texture_bundle.get(), [](void const* ptr) {
                auto tex = reinterpret_cast<Texture const*>(ptr);
                return tex->IsDirty(); })))
            {
                // Update material bundle first to be able to track differences
                out.texture_bundle.reset(tex_collector.CreateBundle());

                UpdateTextures(scene, mat_collector, tex_collector, out);

                if (clear_ditry_flags)
                {
                    std::unique_ptr<Iterator> tex_iter(tex_collector.CreateIterator());
                    DropDirties(tex_iter.get());
                }
            }
            
            // Set current scene
            if (m_current_scene != &scene)
            {
                m_current_scene = &scene;
                
                UpdateCurrentScene(scene, out);
            }
            
            // Make sure to clear dirty flags
            if (clear_ditry_flags)
            {
                scene.ClearDirtyFlags();
            }

            // Return the scene
            return out;
        }
    }
    
    template <typename CompiledScene>
    inline
    void SceneController<CompiledScene>::RecompileFull(Scene1 const& scene, Collector& mat_collector, Collector& tex_collector, CompiledScene& out) const
    {
        UpdateCamera(scene, mat_collector, tex_collector, out);

        scene.GetCamera()->SetDirty(false);

        UpdateLights(scene, mat_collector, tex_collector, out);

        std::unique_ptr<Iterator> light_iter(scene.CreateLightIterator());
        DropDirties(light_iter.get());

        UpdateShapes(scene, mat_collector, tex_collector, out);

        std::unique_ptr<Iterator> shape_iter(scene.CreateShapeIterator());
        DropDirties(shape_iter.get());
        
        UpdateMaterials(scene, mat_collector, tex_collector, out);

        std::unique_ptr<Iterator> mat_iter(mat_collector.CreateIterator());
        DropDirties(mat_iter.get());

        UpdateTextures(scene, mat_collector, tex_collector, out);

        std::unique_ptr<Iterator> tex_iter(tex_collector.CreateIterator());
        DropDirties(tex_iter.get());

        // Make sure to clear dirty flags
        scene.ClearDirtyFlags();
    }
}
