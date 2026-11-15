#pragma once

#include <storage_engine/Errors/ErrorsBase.hpp>

namespace StorageEngine
{
    class IOError : public StorageError
    {
        public:
            IOError(ErrorData err_data, std::error_code ec)
                : StorageError(BuildErrorText(err_data, ec))
                , err_c_(ec)
            {}

            [[nodiscard]] const std::error_code& GetErrorCode() const noexcept
            {
                return err_c_;
            }

        private:
            std::error_code err_c_;

            static ErrorData BuildErrorText(ErrorData err_data, const std::error_code& ec)
            {
                err_data.err_txt_ = ec.message();
                return err_data;
            }
    };
}//StorageEngine
