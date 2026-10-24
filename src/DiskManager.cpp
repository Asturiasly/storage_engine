#include <storage_engine/DiskManager.hpp>
#include <storage_engine/Page.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>

#include <stdexcept>
#include <string>
#include <system_error>
#include <limits>
#include <utility>

namespace StorageEngine
{
    namespace Constants
    {
        static constexpr mode_t DefaultOSMode = 0600; //Random user can't get access to storage file.
    }

    int DiskManager::Open_(const std::filesystem::path& path, DiskManagerMode mode)
    {
        int fd = ::open(path.c_str(), GetFlagsByDMMode_(mode), Constants::DefaultOSMode);
        int errno_buffer = errno; //a lot of operations (malloc/std::string constr etc) could rewrite this. Safer with buffer 
        if (fd == FileDescriptor::InvalidDescriptor)
        { throw std::system_error(errno_buffer, std::system_category(), "Failed to open file via path: " + path.string()); }

        return fd;
    }

    int DiskManager::GetFlagsByDMMode_(DiskManagerMode mode)
    {
        switch (mode) 
        {
            case DiskManagerMode::CreateNew:
            {
                return O_CREAT | O_EXCL | O_RDWR | O_CLOEXEC;
            } 
            case DiskManagerMode::OpenExisting:
            {
                return O_RDWR | O_CLOEXEC;
            }
        }

        __builtin_unreachable();
    }

    DiskManager::DiskManager(std::filesystem::path path, DiskManagerMode mode) :
    fd_(Open_(path, mode)),
    page_count_(0),
    path_(std::move(path))
    {
        struct stat st_buffer;
        int res = ::fstat(fd_.GetFd(), &st_buffer);
        int errno_buffer = errno;
        //TODO: own exception system, no std::exc
        if (res == -1) {throw std::system_error(errno_buffer, std::system_category(), 
            "Failed to get stat for file via path: " + path_.string()); }

        if (st_buffer.st_size % static_cast<off_t>(PageSize) != 0)
        {
            //TODO: own exception system, no std::exc
            throw std::runtime_error("Path to storage file: " + path_.string() + ", expected multiple of: " + std::to_string(PageSize) + " bytes. "
            + "Current size is: " + std::to_string(st_buffer.st_size));
        }

        off_t page_count_buf = st_buffer.st_size / static_cast<off_t>(PageSize);
        if (page_count_buf < 0 || page_count_buf > PageId::MaxPageCount)
        {
            //TODO: own exception system, no std::exc
            throw std::runtime_error("Path to storage file: " + path_.string() + ", page count mustn't be less than 0 and greater than "
            + std::to_string(PageId::MaxPageCount));
        }

        page_count_ = static_cast<PageId::PageCount>(page_count_buf);
    }

    PageId::PageCount DiskManager::GetPageCount() const noexcept
    {
        return page_count_;
    }

    const std::filesystem::path& DiskManager::GetPath() const noexcept
    {
        return path_;
    }
} //StorageEngine