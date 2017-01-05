#include "scene1.h"
#include "light.h"
#include "camera.h"
#include "iterator.h"

#include <vector>
#include <list>
#include <cassert>

namespace Baikal
{
    using ShapeList = std::vector<Shape const*>;
    using LightList = std::vector<Light const*>;
    
    struct Scene1::SceneImpl
    {
        ShapeList m_shapes;
        LightList m_lights;
        
        Camera const* m_camera;
        
        DirtyFlags m_dirty_flags;
    };
    
    Scene1::Scene1()
    : m_impl(new SceneImpl)
    {
        m_impl->m_camera = nullptr;
        ClearDirtyFlags();
    }
    
    Scene1::~Scene1()
    {
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
        
        LightList::const_iterator citer =  std::find(m_impl->m_lights.cbegin(), m_impl->m_lights.cend(), light);
        
        if (citer == m_impl->m_lights.cend())
        {
            m_impl->m_lights.push_back(light);
            
            SetDirtyFlag(kLights);
        }
    }
    
    void Scene1::DetachLight(Light const* light)
    {
        LightList::const_iterator citer =  std::find(m_impl->m_lights.cbegin(), m_impl->m_lights.cend(), light);
        
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
        
        ShapeList::const_iterator citer =  std::find(m_impl->m_shapes.cbegin(), m_impl->m_shapes.cend(), shape);
        
        if (citer == m_impl->m_shapes.cend())
        {
            m_impl->m_shapes.push_back(shape);
            
            SetDirtyFlag(kShapes);
        }
    }
    
    void Scene1::DetachShape(Shape const* shape)
    {
        assert(shape);
        
        ShapeList::const_iterator citer =  std::find(m_impl->m_shapes.cbegin(), m_impl->m_shapes.cend(), shape);
        
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
    
    Iterator* Scene1::CreateLightIterator() const
    {
        return new IteratorImpl<LightList::const_iterator>(m_impl->m_lights.begin(), m_impl->m_lights.end());
    }
}
