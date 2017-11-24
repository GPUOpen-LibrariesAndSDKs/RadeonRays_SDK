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
#pragma once

#include "device_clw.h"
#include "event.h"
#include "CLW.h"


namespace Calc
{
    // Event implementation with CLW
    class EventClw : public Event
    {
    public:
        EventClw() {}
        EventClw(CLWEvent event);
        ~EventClw();

        void Wait() override;
        bool IsComplete() const override;

        void SetEvent(CLWEvent event);
        CLWEvent GetEvent() const { return m_event; }
    private:
        CLWEvent m_event;
    };

    inline EventClw::EventClw(CLWEvent event)
        : m_event(event)
    {
    }

    inline EventClw::~EventClw()
    {
    }

    inline void EventClw::Wait()
    {
        try
        {
            m_event.Wait();
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    inline bool EventClw::IsComplete() const
    {
        try
        {
            return m_event.GetCommandExecutionStatus() == CL_COMPLETE;
        }
        catch (CLWException& e)
        {
            throw ExceptionClw(e.what());
        }
    }

    inline void EventClw::SetEvent(CLWEvent event)
    {
        m_event = event;
    }
}