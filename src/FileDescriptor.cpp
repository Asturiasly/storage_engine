#include <storage_engine/FileDescriptor.hpp>

#include <unistd.h>
#include <utility>

namespace StorageEngine
{
    FileDescriptor::FileDescriptor(int fd) : fd_(fd) {}
    FileDescriptor::FileDescriptor(FileDescriptor&& other) noexcept : fd_(std::exchange(other.fd_, InvalidDescriptor)) {}
    FileDescriptor& FileDescriptor::operator=(FileDescriptor&& other) noexcept
    {
        if (this != &other)          
        {
            if (fd_ != InvalidDescriptor)           
                CloseFd(fd_);       
            fd_ = std::exchange(other.fd_, InvalidDescriptor);
        }
        return *this;
    }

    int FileDescriptor::GetFd() const noexcept
    {
        return fd_;
    }

    FileDescriptor::~FileDescriptor()
    {
        if (fd_ != InvalidDescriptor) 
        {   
            CloseFd(fd_);
        }
    }

    bool FileDescriptor::IsValidDescriptor() const noexcept
    {
        return fd_ != InvalidDescriptor;
    }

    void FileDescriptor::CloseFd(int fd) noexcept
    {
        //Can't close anyway twice if close func returned error
        ::close(fd);
    }
} //StorageEngine
