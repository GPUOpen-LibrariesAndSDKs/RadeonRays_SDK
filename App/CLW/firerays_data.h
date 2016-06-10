#pragma once

#include "firerays.h"

#include <vector>

namespace Baikal
{
    struct IntersectorData
    {
        std::vector<FireRays::Shape*> m_shapes;
        FireRays::IntersectionApi* m_api;

        IntersectionData()
        : m_api (nullptr)
        {
        }

        ~IntersectorData()
        {
            if (m_api)
            {
                for (auto& s : m_shapes)
                {
                    m_api->DeleteShape(s);
                }
            }
        }
    };
}
