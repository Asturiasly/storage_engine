#pragma once

#include <storage_engine/Page.hpp>
#include <cstddef>
#include <cstdlib>
#include <span>
#include <memory>
#include <new>

namespace StorageEngine
{
    // Requirements to alignment for O_DIRECT depends on OS and filesystem 
    // and 4KB is enough for almost every system. It fits for current boundaries and 
    // widespread size of memory pages.
    inline constexpr std::size_t IOAlignSize = 4096;

    class PageBuffer
    {
        public:
        PageBuffer() : ptr_data_
        (static_cast<std::byte*>(std::aligned_alloc(StorageEngine::IOAlignSize, StorageEngine::PageSize)))
        {
            if (ptr_data_.get() == nullptr) { throw std::bad_alloc(); }
        }

        private:
        struct FreeDeleter 
        {
            void operator()(std::byte* p) const noexcept { std::free(p); }
        };
        std::unique_ptr<std::byte[], FreeDeleter> ptr_data_;
    };
} //StorageEngine