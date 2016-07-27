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
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>
#include <iostream>

namespace RadeonRays
{
    ///< An implementation of a concurrent queue
    ///< which is providing the means to wait until 
    ///< elements are available or use non-blocking
    ///< try_pop method.
    ///<
    template <typename T> class thread_safe_queue
    {
    public:
        thread_safe_queue(){}
        ~thread_safe_queue(){}

        // Push element: one of the threads is going to 
        // be woken up to process this if any
        void push(T const& t)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(t);
            cv_.notify_one();
        }

        // Push element: one of the threads is going to 
        // be woken up to process this if any
        void push(T&& t)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(std::move(t));
            cv_.notify_one();
        }

        // Wait until there are element to process.
        void wait_and_pop(T& t)
        {
            std::unique_lock<std::mutex> lock(mutex_);
            cv_.wait(lock, [this](){return !queue_.empty();});
            t = queue_.front();
            queue_.pop();
        }

        // Try to pop element. Returns true if element has been popped, 
        // false if there are no element in the queue
        bool try_pop(T& t)
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (queue_.empty())
                return false;
            t = std::move(queue_.front());
            queue_.pop();
            return true;
        }

        size_t size() const
        {
            std::unique_lock<std::mutex> lock(mutex_);
            return queue_.size();
        }

    private:
        mutable std::mutex mutex_;
        std::condition_variable cv_;
        std::queue<T> queue_;
    };


    ///< Thread pool implementation which is using concurrency
    ///< available in the system
    ///<
    template <typename RetType> class thread_pool
    {
    public:
        explicit thread_pool(int sleep_time = 50)
            : sleep_time_(sleep_time)
        {
            done_ = false;
            int num_threads = std::thread::hardware_concurrency();
            num_threads = num_threads == 0 ? 2 : num_threads;

            //#ifdef _DEBUG
            //        std::cout << num_threads << " hardware threads available\n";
            //#endif

            for (int i=0; i < num_threads; ++i)
            {
                threads_.push_back(std::thread(&thread_pool::run_loop, this));
            }
        }

        ~thread_pool()
        {
            done_ = true;
            std::for_each(threads_.begin(), threads_.end(), std::mem_fun_ref(&std::thread::join));
        }

        // Submit a new task into the pool. Future is returned in
        // order for caller to track the execution of the task
        std::future<RetType> submit(std::function<RetType()>&& f)
        {
            std::packaged_task<RetType()> task(std::move(f));
            auto future = task.get_future();
            work_queue_.push(std::move(task));
            return future;
        }

        size_t size() const
        {
            return work_queue_.size();
        }

        void setSleepTime(int sleep_time)
        {
            sleep_time_ = sleep_time;
        }

    private:
        void run_loop()
        {
            std::packaged_task<RetType()> f;
            while(!done_)
            {
                if (work_queue_.try_pop(f))
                {
                    f();
                }
                else
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time_));
                }
            }
        }


        thread_safe_queue<std::packaged_task<RetType()> > work_queue_;
        std::atomic_bool done_;
        std::vector<std::thread> threads_;
        int sleep_time_;
    };
}


#endif
