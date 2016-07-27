#pragma once

#include <map>
#include "CLW.h"

#include "radeon_rays_cl.h"

namespace Baikal
{
    class Scene;
    struct ClwScene;

    class SceneTracker
    {
    public:
        SceneTracker(CLWContext context, int devidx);

        virtual ~SceneTracker();

        virtual ClwScene& CompileScene(Scene const& scene) const;
        
        RadeonRays::IntersectionApiCL* GetIntersectionApi() { return  m_api; }

    protected:
        virtual void RecompileFull(Scene const& scene, ClwScene& out) const;
        virtual void BakeTextures(Scene const& scene, ClwScene& out) const;
        virtual void ReloadIntersector(Scene const& scene, ClwScene& inout) const;

    private:
        void UpdateCamera(Scene const& scene, ClwScene& out) const;
        void UpdateGeometry(Scene const& scene, ClwScene& out) const;
        void UpdateMaterials(Scene const& scene, ClwScene& out) const;
        void UpdateMaterialInputs(Scene const& scene, ClwScene& out) const;



    private:
        // Context
        CLWContext m_context;
        // Intersection API
        RadeonRays::IntersectionApiCL* m_api;
        // Current scene
        mutable Scene const* m_current_scene;

        mutable std::map<Scene const*, ClwScene> m_scene_cache;
        mutable std::size_t m_vidmem_usage;
    };
}
