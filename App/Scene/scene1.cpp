#include "scene1.h"
#include "light.h"
#include "camera.h"
#include "iterator.h"

#include <vector>
#include <list>
#include <cassert>
#include <set>

namespace Baikal
{
    // Data structures for shapes and lights
    using ShapeList = std::vector<Shape const*>;
    using LightList = std::vector<Light const*>;
    using AutoreleasePool = std::set<SceneObject const*>;

    // Internal data
    struct Scene1::SceneImpl
    {
        ShapeList m_shapes;
        LightList m_lights;
        Camera const* m_camera;

        DirtyFlags m_dirty_flags;

        AutoreleasePool m_autorelease_pool;
    };

    Scene1::Scene1()
    : m_impl(new SceneImpl)
    {
        m_impl->m_camera = nullptr;
        ClearDirtyFlags();
    }

    Scene1::~Scene1()
    {
        for(auto& i : m_impl->m_autorelease_pool)
        {
            delete i;
        }
    }

    Scene1::DirtyFlags Scene1::GetDirtyFlags() const
    {
        return m_impl->m_dirty_flags;
    }

    void Scene1::ClearDirtyFlags() const
    {
        m_impl->m_dirty_flags = 0;
    }

    void Scene1::SetDirtyFlag(DirtyFlags flag) const
    {
        m_impl->m_dirty_flags = m_impl->m_dirty_flags | flag;
    }

    void Scene1::SetCamera(Camera const* camera)
    {
        m_impl->m_camera = camera;
        SetDirtyFlag(kCamera);
    }

    Camera const* Scene1::GetCamera() const
    {
        return m_impl->m_camera;
    }

    void Scene1::AttachLight(Light const* light)
    {
        assert(light);

        // Check if the light is already in the scene
        LightList::const_iterator citer =  std::find(m_impl->m_lights.cbegin(), m_impl->m_lights.cend(), light);
        
        // And insert only if not
        if (citer == m_impl->m_lights.cend())
        {
            m_impl->m_lights.push_back(light);

            SetDirtyFlag(kLights);
        }
    }

    void Scene1::DetachLight(Light const* light)
    {
        // Check if the light is in the scene
        LightList::const_iterator citer =  std::find(m_impl->m_lights.cbegin(), m_impl->m_lights.cend(), light);
        
        // And remove it if yes
        if (citer != m_impl->m_lights.cend())
        {
            m_impl->m_lights.erase(citer);
            
            SetDirtyFlag(kLights);
        }
    }

    std::size_t Scene1::GetNumLights() const
    {
        return m_impl->m_lights.size();
    }

    Iterator* Scene1::CreateShapeIterator() const
    {
        return new IteratorImpl<ShapeList::const_iterator>(m_impl->m_shapes.begin(), m_impl->m_shapes.end());
    }
    
    void Scene1::AttachShape(Shape const* shape)
    {
        assert(shape);
        
        // Check if the shape is already in the scene
        ShapeList::const_iterator citer =  std::find(m_impl->m_shapes.cbegin(), m_impl->m_shapes.cend(), shape);
        
        // And attach it if not
        if (citer == m_impl->m_shapes.cend())
        {
            m_impl->m_shapes.push_back(shape);
            
            SetDirtyFlag(kShapes);
        }
    }
    
    void Scene1::DetachShape(Shape const* shape)
    {
        assert(shape);
        
        // Check if the shape is in the scene
        ShapeList::const_iterator citer =  std::find(m_impl->m_shapes.cbegin(), m_impl->m_shapes.cend(), shape);
        
        // And detach if yes
        if (citer != m_impl->m_shapes.cend())
        {
            m_impl->m_shapes.erase(citer);
            
            SetDirtyFlag(kShapes);
        }
    }
    
    std::size_t Scene1::GetNumShapes() const
    {
        return m_impl->m_shapes.size();
    }
    
    void Scene1::AttachAutoreleaseObject(SceneObject const* object)
    {
        assert(object);
        
        // Check if the light is already in the scene
        AutoreleasePool::const_iterator citer =  std::find(m_impl->m_autorelease_pool.cbegin(), m_impl->m_autorelease_pool.cend(), object);
        
        // And insert only if not
        if (citer == m_impl->m_autorelease_pool.cend())
        {
            m_impl->m_autorelease_pool.insert(object);
        }
    }
    
    void Scene1::DetachAutoreleaseObject(SceneObject const* object)
    {
        assert(object);
        
        // Check if the light is already in the scene
        AutoreleasePool::const_iterator citer =  std::find(m_impl->m_autorelease_pool.cbegin(), m_impl->m_autorelease_pool.cend(), object);
        
        // And insert only if not
        if (citer != m_impl->m_autorelease_pool.cend())
        {
            m_impl->m_autorelease_pool.erase(citer);
        }
    }
    
    Iterator* Scene1::CreateLightIterator() const
    {
        return new IteratorImpl<LightList::const_iterator>(m_impl->m_lights.begin(), m_impl->m_lights.end());
    }
    
    bool Scene1::IsValid() const
    {
        return GetCamera() &&
        GetNumLights() > 0 &&
        GetNumShapes() > 0;
    }
}
