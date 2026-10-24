#pragma once

#include <storage_engine/FileDescriptor.hpp>
#include <storage_engine/Page.hpp>

#include <filesystem>

namespace StorageEngine 
{
    enum class DiskManagerMode
    {
        CreateNew,
        OpenExisting
    };

    class DiskManager
    { 
        public:
            explicit DiskManager(std::filesystem::path path, DiskManagerMode mode);
            DiskManager(const DiskManager&) = delete;
            DiskManager(DiskManager&&) = delete;
            DiskManager& operator=(const DiskManager&) = delete;

            [[nodiscard]] const std::filesystem::path& GetPath() const noexcept;
            [[nodiscard]] PageId::PageCount GetPageCount() const noexcept;

        private:
            [[nodiscard]] static int Open_(const std::filesystem::path& path, DiskManagerMode mode);
            [[nodiscard]] static int GetFlagsByDMMode_(DiskManagerMode mode);
            FileDescriptor fd_;
            PageId::PageCount page_count_;
            std::filesystem::path path_;
    };
} //StorageEngine