#pragma once

#include <map>
#include "CLW.h"

namespace Baikal
{
    class Scene;
    class ClwScene;

    class SceneTracker
    {
    public:
        SceneTracker(CLWContext context);

        virtual ~SceneTracker() = default;

        virtual ClwScene& CompileScene(Scene const& scene) const;

    protected:
        virtual void RecompileFull(Scene const& scene, ClwScene& out) const;
        virtual void BakeTextures(Scene const& scene, ClwScene& out) const;

    private:
        CLWContext m_context;

        mutable std::map<Scene const*, ClwScene> m_scene_cache;
        mutable std::size_t m_vidmem_usage;
    };

    inline SceneTracker::SceneTracker(CLWContext context)
    : m_context(context)
    , m_vidmem_usage(0)
    {
    }
}
