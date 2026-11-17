#include <storage_engine/DiskManager.hpp>
#include <storage_engine/Common/Operations.hpp>
#include <storage_engine/Page.hpp>
#include <storage_engine/PageBuffer.hpp>
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
    current_page_count_(0),
    path_(std::move(path))
    {
        struct stat st;
        int fstat_res = ::fstat(fd_.GetFd(), &st);
        int fstat_errno = errno;

        if (fstat_res == -1) 
        {
            throw IOError(ErrorData(Operation::Open, path_), std::error_code(fstat_errno, std::system_category()));
        }

        ::off_t page_count_buf = st.st_size / static_cast<off_t>(PageSize);

        if (st.st_size % static_cast<off_t>(PageSize) != 0)
        {
            throw CorruptedDataError(ErrorData(Operation::Open, path_), 
            CorruptedDataError::Reason::SizeNotMultipleOfPage);
        }

        if (page_count_buf > PageId::MaxPageCount)
        {
            throw ResourceExhaustionError(ErrorData(Operation::Open, path_),
            ResourceExhaustionError::Reason::ExhaustedPages);
        }

        current_page_count_ = static_cast<PageId::PageCount>(page_count_buf);
    }

    //Binary condition. No sense to use EINTR cycles.
    //ENOSPC is obvious risk in future writing in db file, but we do not guarantee that storage have enough space
    //for this data after page allocation.
    PageId DiskManager::AllocatePage()
    {
        if (current_page_count_ >= PageId::MaxPageCount)
        {
            throw ResourceExhaustionError(ErrorData(Operation::AllocatePage, path_), 
            ResourceExhaustionError::Reason::ExhaustedPages);
        }

        if (::ftruncate(fd_.GetFd(), static_cast<::off_t>(current_page_count_* PageSize + PageSize)) == -1)
        {
            int ftruncate_errno = errno;
            throw IOError(ErrorData(Operation::AllocatePage, path_), std::error_code(ftruncate_errno, std::system_category()));
        }

        PageId new_page(static_cast<PageId::UnderlyingType>(current_page_count_++));
        return new_page;
    }

    PageId::PageCount DiskManager::GetCurrentPageCount() const noexcept
    {
        return current_page_count_;
    }

    const std::filesystem::path& DiskManager::GetPath() const noexcept
    {
        return path_;
    }
} //StorageEngine