#include <storage_engine/DiskManager.hpp>
#include <storage_engine/Page.hpp>
#include <storage_engine/Errors/ErrorsHeader.hpp>

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
        { 
            throw IOError(ErrorData(Operation::Open, path), std::error_code(errno_buffer, std::system_category()));
        }

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

        if (res == -1) 
        {
            throw IOError(ErrorData(Operation::Open, path_), std::error_code(errno_buffer, std::system_category()));
        }

        if (st_buffer.st_size % static_cast<off_t>(PageSize) != 0)
        {
            throw CorruptedDataError(ErrorData(Operation::Open, path_), 
            CorruptedDataError::Reason::SizeNotMultipleOfPage);
        }

        off_t page_count_buf = st_buffer.st_size / static_cast<off_t>(PageSize);
        if (page_count_buf > PageId::MaxPageCount)
        {
            throw ResourceExhaustionError(ErrorData(Operation::Open, path_),
            ResourceExhaustionError::Reason::ExhaustedPages);
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