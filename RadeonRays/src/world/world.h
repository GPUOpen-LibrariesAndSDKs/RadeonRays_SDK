/**********************************************************************
Copyright (c) 2016 Advanced Micro Devices, Inc. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
********************************************************************/
#ifndef WORLD_H
#define WORLD_H

#include <memory>
#include <vector>

#include "radeon_rays.h"
#include "../util/options.h"

namespace RadeonRays
{
    ///< World class is a container for all entities for the scene. 
    ///< It hosts entities and is in charge of destroying them.
    ///< For convenience reasons it impelements Primitive interface
    ///< to be able to provide intersection capabilities.
    ///<
    class World
    {
    public:
        //
        World() = default;
        //
        virtual ~World() = default;
        // Attach the shape updating all the flags
        void AttachShape(Shape const* shape);
        // Detach the shape 
        void DetachShape(Shape const* shape);
        // Detach all
        void DetachAll();
        // Call this as scene has been commited
        void OnCommit();
        // 
        bool has_changed() const;
        //
        int GetStateChange() const;


    public:
        // Shapes in the scene
        std::vector<Shape const*> shapes_;
        // Flags to indicate shapes have been changed
        std::vector<int> shapes_dirty_;

        // TODO: Do more preciese dirty flags tracking
        bool has_changed_ = true;
        // Global flags
        int hint_;
        // Options
        Options options_;
    };

    inline bool World::has_changed() const
    {
        return has_changed_;
    }
}


#endif // WORLD_H