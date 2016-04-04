//
//  CLWKernel.cpp
//  CLW
//
//  Created by dmitryk on 26.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#include "CLWKernel.h"
#include "ParameterHolder.h"
#include "CLWExcept.h"

CLWKernel CLWKernel::Create(cl_kernel kernel)
{
    CLWKernel k(kernel);
    clReleaseKernel(kernel);
    return k;
}

CLWKernel::CLWKernel(cl_kernel kernel)
: ReferenceCounter<cl_kernel, clRetainKernel, clReleaseKernel>(kernel)
{
}

void CLWKernel::SetArg(unsigned int idx, ParameterHolder param)
{
    param.SetArg(*this, idx); 
}

void CLWKernel::SetArg(unsigned int idx, size_t size, void* ptr)
{
	cl_int status = CL_SUCCESS;
	
	status = clSetKernelArg(*this, idx, size, ptr);
		
	ThrowIf(status != CL_SUCCESS, status, "clSetKernelArg failed");
}