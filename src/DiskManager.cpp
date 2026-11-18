#include <storage_engine/DiskManager.hpp>
#include <storage_engine/Common/Operations.hpp>
#include <storage_engine/Page.hpp>
#include <storage_engine/PageBuffer.hpp>
#include <storage_engine/Errors/ErrorsHeader.hpp>

#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>

#include <cassert>
#include <stdexcept>
#include <string>
#include <system_error>
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
    //ENOSPC is obvious risk in future writing in file, but we do not guarantee that storage have enough space
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

    void DiskManager::WritePage(PageId page, PageBuffer::ConstPageView bytes)
    {
        //Expected valid page - got invalid
        assert(page.GetId().has_value());

        size_t written_bytes = 0;
        PageId::UnderlyingType page_id = page.GetId().value();

        if (page_id >= current_page_count_)
        {
            throw CorruptedDataError(ErrorData(Operation::WritePage, path_),
            CorruptedDataError::Reason::PageOutOfRange);
        }

        while(written_bytes < PageSize)
        {
            ::ssize_t ret = pwrite(fd_.GetFd(), bytes.data() + written_bytes, 
            PageSize - written_bytes, 
            static_cast<::off_t>(page_id * PageSize + written_bytes));

            if (ret == -1)
            {
                int pwrite_errno = errno;
                if (pwrite_errno == EINTR)
                    continue;
                else
                    throw IOError(ErrorData(Operation::WritePage, path_), 
                    std::error_code(pwrite_errno, std::system_category()));
            }

            if (ret == 0)
            {
                //Kernel gives no progress or error for write operation
                throw IOError(ErrorData(Operation::WritePage, path_), 
                std::error_code(EIO, std::system_category()));
            }

            written_bytes += static_cast<size_t>(ret);
        }
    }

    void DiskManager::ReadPage(PageId page, PageBuffer::PageView bytes)
    {
        assert(page.GetId().has_value());

        size_t read_bytes = 0;
        PageId::UnderlyingType page_id = page.GetId().value();

        if (page_id >= current_page_count_)
        {
            throw CorruptedDataError(ErrorData(Operation::ReadPage, path_),
            CorruptedDataError::Reason::PageOutOfRange);
        }

        while(read_bytes < PageSize)
        {
            ::ssize_t ret = pread(fd_.GetFd(), bytes.data() + read_bytes, 
            PageSize - read_bytes, 
            static_cast<::off_t>(page_id * PageSize + read_bytes));

            if (ret == -1)
            {
                int pread_errno = errno;
                if (pread_errno == EINTR)
                    continue;

                throw IOError(ErrorData(Operation::ReadPage, path_), 
                std::error_code(pread_errno, std::system_category()));
            }

            if (ret == 0)
            {
                //Read nothing.
                throw CorruptedDataError(ErrorData(Operation::ReadPage, path_), 
                CorruptedDataError::Reason::UnexpectedShortRead);
            }

            read_bytes += static_cast<size_t>(ret);
        }
    }

    //Buffer pool will call it later
    //TODO Sync directory. Limitation now
    void DiskManager::Sync()
    {
        while (true)
        {
            //full metadata writing
            //No need in faster fdatasync.
            int ret = fsync(fd_.GetFd());

            if (ret == -1)
            {
                int fsync_errno = errno;
                if (fsync_errno == EINTR)
                    continue;

                throw IOError(ErrorData(Operation::SyncPages, path_), 
                std::error_code(fsync_errno, std::system_category()));
            }

            break;
        }
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