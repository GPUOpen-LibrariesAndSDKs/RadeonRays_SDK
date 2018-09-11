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
#ifndef SHAPEIMPL_H
#define SHAPEIMPL_H

#include "radeon_rays.h"
#include "math/float3.h"
#include "math/matrix.h"

namespace RadeonRays
{
    ///< Basic implementation of shape interface capable of handling the state required from
    ///< the API standpoint.
    ///<
    class ShapeImpl : public Shape
    {
    public:
        // State tracking flags
        enum StateChangeFlags
        {
            kStateChangeNone = 0x0,
            kStateChangeTransform = 0x1,
            kStateChangeMotion = 0x2,
            kStateChangeId = 0x4,
            kStateChangeMask = 0x5
        };
        
        // Constructor
        ShapeImpl() = default;

        // Destructor
        ~ShapeImpl() = default;

        // This is needed since instances need special API handling
        virtual bool is_instance() const;

        // World space transform
        void SetTransform(matrix const& m, matrix const& minv) override;
        
        // Get world space matrix along with its inverse
        void GetTransform(matrix& m, matrix& minv) const override;
        
        // Motion blur
        void SetLinearVelocity(float3 const& v) override;
        
        // Get linear motion
        float3 GetLinearVelocity() const override;
        
        // Set angular motion for motion blur
        void SetAngularVelocity(quaternion const& q) override;
        
        // Get angular motion
        quaternion GetAngularVelocity() const override;
        
        // ID of a shape
        void SetId(Id id) override;
        
        // Get ID
        Id GetId() const override;
        
        // Get state changes since last OnCommit
        int GetStateChange() const;

        // Clear state change
        void OnCommit() const;
        
    protected:
        // World transform
        matrix worldmat_;
        matrix worldmatinv_;
        float3 linearmotion_;
        quaternion angulrmotion_;
        // Id
        Id id_;
        // State change
        mutable int statechange_;
    };
    
    inline void ShapeImpl::SetTransform(matrix const& m, matrix const& minv)
    {
        worldmat_ = m;
        worldmatinv_ = minv;
        statechange_ |= kStateChangeTransform;
    }
    
    inline void ShapeImpl::GetTransform(matrix& m, matrix& minv) const
    {
        m = worldmat_;
        minv = worldmatinv_;
    }
    
    inline void ShapeImpl::SetLinearVelocity(float3 const& v)
    {
        linearmotion_ = v;
        statechange_ |= kStateChangeMotion;
    }
    
    inline float3 ShapeImpl::GetLinearVelocity() const
    {
        return linearmotion_;
    }
    
    inline void ShapeImpl::SetAngularVelocity(quaternion const& q)
    {
        angulrmotion_ = q;
        statechange_ |= kStateChangeMotion;
    }
    
    inline quaternion ShapeImpl::GetAngularVelocity() const
    {
        return angulrmotion_;
    }
    
    inline void ShapeImpl::SetId(Id id)
    {
        id_ = id;
        statechange_ |= kStateChangeId;
    }
    
    inline Id ShapeImpl::GetId() const
    {
        return id_;
    }
    
    inline int ShapeImpl::GetStateChange() const
    {
        return statechange_;
    }
    
    inline void ShapeImpl::OnCommit() const
    {
        statechange_ = kStateChangeNone;
    }

    inline bool ShapeImpl::is_instance() const
    {
        return false;
    }
}


#endif
