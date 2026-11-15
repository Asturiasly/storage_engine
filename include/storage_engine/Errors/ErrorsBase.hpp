#pragma once

#include <stdexcept>
#include <system_error>
#include <optional>
#include <string>
#include <memory>
#include <filesystem>
#include <utility>

#include <storage_engine/Common/Operations.hpp>
#include <storage_engine/Page.hpp>

namespace StorageEngine
{
    struct ErrorData
    {
        ErrorData() = delete;
        ErrorData(Operation operation, 
            std::filesystem::path path, 
            StorageEngine::PageId page)
        :
        operation_(operation),
        path_(std::move(path)),
        page_(page)
        {}

        ErrorData(Operation operation, 
            std::filesystem::path path)
        :
        operation_(operation),
        path_(std::move(path)),
        page_(PageId{})
        {}

        std::string err_txt_;
        Operation operation_;
        std::filesystem::path path_;
        StorageEngine::PageId page_;
    };

    class StorageError : public std::runtime_error
    {
        public:
            StorageError() = delete;

            [[nodiscard]] const std::filesystem::path& GetPath() const noexcept
            {
                return err_data_->path_;
            }

            [[nodiscard]] std::optional<StorageEngine::PageId> GetPage() const noexcept
            {
                return err_data_->page_.IsValid()
                    ? std::optional<StorageEngine::PageId>{err_data_->page_}
                    : std::optional<StorageEngine::PageId>{};
            }

            [[nodiscard]] Operation GetOperation() const noexcept
            {
                return err_data_->operation_;
            }

        protected:
            StorageError(ErrorData err_data) :
            std::runtime_error(BuildMessage(err_data)),
            err_data_(std::make_shared<const ErrorData>(std::move(err_data)))
            {}

            static std::string BuildMessage(const ErrorData& data)
            {
                return data.page_.IsValid() ?
                "ERROR: " + data.err_txt_ +
                " Path to file: " + data.path_.string() +
                " PageId: " + std::string(data.page_.ToString()) +
                " Operation proceeded: " + std::string(OperationToString(data.operation_))
                :
                "ERROR: " + data.err_txt_ +
                " Path to file: " + data.path_.string() +
                " Operation proceeded: " + std::string(OperationToString(data.operation_));
            }
        
        private:
            std::shared_ptr<const ErrorData> err_data_;
    };
}   //StorageEngine




