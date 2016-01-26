//
//  CLWCommandQueue.cpp
//  CLW
//
//  Created by dmitryk on 02.12.13.
//  Copyright (c) 2013 dmitryk. All rights reserved.
//

#include "CLWCommandQueue.h"
#include "CLWContext.h"
#include "CLWDevice.h"
#include "CLWExcept.h"

// Disable OCL 2.0 deprecations
#pragma warning(push)
#pragma warning(disable:4996)

CLWCommandQueue CLWCommandQueue::Create(CLWDevice device, CLWContext context)
{
    cl_int status = CL_SUCCESS;

    cl_command_queue commandQueue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &status);

    ThrowIf(status != CL_SUCCESS, status, "clCreateCommandQueue failed");

    CLWCommandQueue cmdQueue(commandQueue);

    clReleaseCommandQueue(commandQueue);

    return cmdQueue;
}

CLWCommandQueue CLWCommandQueue::Create(cl_command_queue queue)
{
    return CLWCommandQueue(queue);
    
}

CLWCommandQueue::CLWCommandQueue(cl_command_queue cmdQueue)
: ReferenceCounter<cl_command_queue, clRetainCommandQueue, clReleaseCommandQueue>(cmdQueue)
{
}

CLWCommandQueue::CLWCommandQueue()
{
	
}

CLWCommandQueue::~CLWCommandQueue()
{
}

#pragma warning(pop)


