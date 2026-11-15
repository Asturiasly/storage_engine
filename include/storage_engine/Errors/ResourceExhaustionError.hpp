#pragma once

#include <storage_engine/Errors/ErrorsBase.hpp>

#include <string>

namespace StorageEngine
{
    class ResourceExhaustionError : public StorageError
    {
        public:

            enum class Reason
            {
                ExhaustedPages,
            };

            ResourceExhaustionError(ErrorData err_data, Reason reason) :
            StorageError(BuildErrorText(err_data, reason)),
            reason_(reason)
            {}

            [[nodiscard]] Reason GetReason() const noexcept
            {
                return reason_;
            }

        private:
            Reason reason_;
            static ErrorData BuildErrorText(ErrorData err_data, Reason reason)
            {
                switch(reason)
                {
                    case Reason::ExhaustedPages:
                    {
                        err_data.err_txt_ = "Reached max of pages(" + std::to_string(PageId::MaxPageCount) + ")";
                        break;
                    }
                }
                return err_data;
            }
    };
}//StorageEngine

