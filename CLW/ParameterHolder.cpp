//
//  ParameterHolder.cpp
//  CLW
//
//  Created by dmitryk on 23.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#include "ParameterHolder.h"
#include <cassert>


void ParameterHolder::SetArg(cl_kernel kernel, unsigned int idx)
{
    cl_int status = CL_SUCCESS;
    switch (type_)
    {
        case ParameterHolder::kMem:
            clSetKernelArg(kernel, idx, sizeof(cl_mem), &mem_);
            break;
        case ParameterHolder::kInt:
            clSetKernelArg(kernel, idx, sizeof(cl_int), &intValue_);
            break;
            
        case ParameterHolder::kUInt:
            clSetKernelArg(kernel, idx, sizeof(cl_uint), &uintValue_);
            break;
            
        case ParameterHolder::kFloat:
            clSetKernelArg(kernel, idx, sizeof(cl_float), &floatValue_);
            break;
        
        case ParameterHolder::kFloat2:
            clSetKernelArg(kernel, idx, sizeof(cl_float2), &floatValue2_);
            break;
        
        case ParameterHolder::kFloat4:
            clSetKernelArg(kernel, idx, sizeof(cl_float4), &floatValue4_);
            break;
        
        case ParameterHolder::kDouble:
            clSetKernelArg(kernel, idx, sizeof(cl_double), &doubleValue_);
            break;
            
        case ParameterHolder::kShmem:
            clSetKernelArg(kernel, idx, uintValue_, nullptr);
            break;
            
        default:
            break;
    }
    
    assert(status == CL_SUCCESS);
    // TODO: add error handling
}
