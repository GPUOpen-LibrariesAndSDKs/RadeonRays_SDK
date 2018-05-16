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
#include "world.h"

#include "../primitive/shapeimpl.h"

namespace RadeonRays
{
    void World::AttachShape(Shape const* shape)
    {
        if (std::find(shapes_.cbegin(), shapes_.cend(), shape) == shapes_.cend())
        {
            shapes_.push_back(shape);
            has_changed_ = true;
        }
    }

    void World::DetachShape(Shape const* shape)
    {
        auto iter = std::find(shapes_.begin(), shapes_.end(), shape);
        if (iter != shapes_.end())
        {
            shapes_.erase(iter);
            has_changed_ = true;
        }
    }
    
    void World::DetachAll()
    {
        shapes_.clear();
        has_changed_ = true;
    }

    int World::GetStateChange() const
    {
        int statechange_ = ShapeImpl::kStateChangeNone;

        for (auto shape : shapes_)
        {
            ShapeImpl const* shapeimpl = static_cast<ShapeImpl const*>(shape);

            statechange_ |= shapeimpl->GetStateChange();
        }

        return statechange_;
    }

    void World::OnCommit()
    {
        for (auto shape : shapes_)
        {
            auto shapeimpl = static_cast<ShapeImpl const*>(shape);

            shapeimpl->OnCommit();
        }

        has_changed_ = false;
    }
}