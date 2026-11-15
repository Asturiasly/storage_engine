#pragma once

#include <storage_engine/Errors/ErrorsBase.hpp>
    
#include <string>

namespace StorageEngine
{
    class CorruptedDataError : public StorageError
    {
        public:

            enum class Reason
            {
                SizeNotMultipleOfPage,
            };

            CorruptedDataError(ErrorData err_data, Reason reason) :
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
                    case Reason::SizeNotMultipleOfPage:
                    {
                        err_data.err_txt_ = "Expected multiple of size of page " + std::to_string(StorageEngine::PageSize);
                        break;
                    }
                }
                return err_data;
            }
    };
}//StorageEngine