#pragma once

#include <string>

namespace StorageEngine
{
    enum class Operation
    {
        Open
    };

    static std::string_view OperationToString(Operation op) noexcept
    {
        switch (op)
        {
            case Operation::Open:
            {
                return "Open";
            }
        }

        __builtin_unreachable();
    }
}//StorageEngine