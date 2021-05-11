/**********************************************************************
Copyright (c) 2019 Advanced Micro Devices, Inc. All rights reserved.
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
#pragma once

#include <assert.h>
#include <functional>
#include <queue>
#include <type_traits>

namespace rt
{
/** @brief Pool of objects.
 *
 * Implements object pooling for a subsequent reuse.
 **/
template <typename T>
class Pool
{
public:
    //<! Producer function.
    using CreateFn = std::function<T*()>;
    //<! Deleter function.
    using DeleteFn = std::function<void(T*)>;

    /**
     * @brief Constructor.
     *
     * @param create_fn User function which is called to create a new object.
     * @param delete_fn User function which is called to delete an object.
     **/
    Pool(CreateFn create_fn = nullptr, DeleteFn delete_fn = nullptr) : create_fn_(create_fn), delete_fn_(delete_fn) {}

    /**
     * @brief Destructor.
     **/
    ~Pool()
    {
        while (!objects_.empty())
        {
            T* obj = objects_.front();
            objects_.pop();
            delete_fn_(obj);
        }
    }

    /**
     * @brief Get an object from a pool.
     *
     * If the pool is empty, a new object is created using create_fn,
     * otherwise existing object is returned.
     *
     * @return An object.
     **/
    T* AcquireObject()
    {
        if (!objects_.empty())
        {
            T* obj = objects_.front();
            objects_.pop();
            return obj;
        } else
        {
            return create_fn_();
        }
    }

    /**
     * @brief Release object back to the pool.
     **/
    void ReleaseObject(T* obj) { objects_.push(obj); }

    /**
     * @brief Set producer function.
     **/

    void SetCreateFn(CreateFn fn) { create_fn_ = fn; }
    /**
     * @brief Set deleter function.
     **/
    void SetDeleteFn(DeleteFn fn) { delete_fn_ = fn; }

private:
    //<! Objects are organised in the queue.
    std::queue<T*> objects_;
    //<! Producer function.
    CreateFn create_fn_ = nullptr;
    //<! Deleter function.
    DeleteFn delete_fn_ = nullptr;
};
}  // namespace rt
