#pragma once

#include <string>

namespace StorageEngine
{
    enum class Operation
    {
        Open,
        SyncPages,
        ReadPage,
        WritePage,
        AllocatePage
    };

    inline std::string_view OperationToString(Operation op) noexcept
    {
        switch (op)
        {
            case Operation::Open:
            {
                return "Open";
            }
            case Operation::SyncPages:
            {
                return "SyncPages";
            }
            case Operation::ReadPage:
            {
                return "ReadPage";
            }
            case Operation::WritePage:
            {
                return "WritePage";
            }
            case Operation::AllocatePage:
            {
                return "AllocatePage";
            }
        }
        __builtin_unreachable();
    }
}//StorageEngine