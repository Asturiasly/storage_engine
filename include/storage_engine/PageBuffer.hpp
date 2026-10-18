#pragma once

#include <storage_engine/Page.hpp>
#include <bit>
#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
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
        using PageView = std::span<std::byte, StorageEngine::PageSize>;
        using ConstPageView = std::span<const std::byte, StorageEngine::PageSize>;
        //std::aligned_alloc static requirements
        static_assert(std::has_single_bit(StorageEngine::IOAlignSize), "IOAlignSize must be power of two");
        static_assert(StorageEngine::PageSize % StorageEngine::IOAlignSize == 0, "The value PageSize must be a multiple of the value IOAlignSize.");

        PageBuffer() : ptr_data_
        (static_cast<std::byte*>(std::aligned_alloc(StorageEngine::IOAlignSize, StorageEngine::PageSize)))
        {
            if (ptr_data_.get() == nullptr) { throw std::bad_alloc(); }

            //to avoid previous data of this process to be written in db file
            std::memset(ptr_data_.get(), 0, StorageEngine::PageSize);
        }

        [[nodiscard]] ConstPageView View() const noexcept
        {
            //mustn't be call after moving
            assert(ptr_data_.get() != nullptr);
            return ConstPageView(ptr_data_.get(), StorageEngine::PageSize);
        }

        [[nodiscard]] PageView View() noexcept
        {
            //mustn't be call after moving
            assert(ptr_data_.get() != nullptr);
            return PageView(ptr_data_.get(), StorageEngine::PageSize);
        }

        private:
        struct FreeDeleter 
        {
            void operator()(std::byte* p) const noexcept { std::free(p); }
        };
        std::unique_ptr<std::byte[], FreeDeleter> ptr_data_;
    };
} //StorageEngine