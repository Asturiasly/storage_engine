#pragma once

namespace StorageEngine 
{
    class FileDescriptor
    { 
        public:
            explicit FileDescriptor(int fd);
            FileDescriptor(const FileDescriptor&) = delete;
            FileDescriptor& operator=(const FileDescriptor&) = delete;
            FileDescriptor(FileDescriptor&& other) noexcept;
            FileDescriptor& operator=(FileDescriptor&& other) noexcept;
            [[nodiscard]] bool IsValidDescriptor() const noexcept;
            [[nodiscard]] int GetFd() const noexcept;
            ~FileDescriptor();
            static constexpr int InvalidDescriptor = -1;

        private:
            int fd_;
            static void CloseFd_(int fd) noexcept;
    };
} //StorageEngine