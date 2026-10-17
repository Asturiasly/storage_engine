#pragma once

#include <cstddef>
#include <cstdint>
#include <limits>
#include <compare>

namespace StorageEngine 
{
    // Corresponds to a state of minimal page presence in virtual OS memory (4KB, Filesystem ext4 = 4KB).
    //Reading 1 block is less operation per keys in b-tree nodes and using less resources while small changes.
    inline constexpr std::size_t PageSize = 4096;

    class PageId
    {
        //project 2pw32 = ~4.29 bil of pages with std::uint32_t, max file = 16TB.
        //2x memory usage for B-Tree is less keys in nodes
        //Changing uint32->uint64 = losing all previous db files.

        public:
            using UnderlyingType = std::uint32_t;

            //Deleted all implicit conversions
            template <typename T>
            constexpr explicit PageId(T) noexcept = delete;
            constexpr explicit PageId (UnderlyingType id) noexcept : id_(id) {}
            constexpr PageId() noexcept : id_(InvalidValue_) {}

            [[nodiscard]] constexpr bool IsValid() const noexcept { return id_ != InvalidValue_; }
            [[nodiscard]] constexpr UnderlyingType GetId() const noexcept { return id_; }
            constexpr auto operator<=>(const PageId& /*other*/) const = default;

        private:
            static constexpr UnderlyingType InvalidValue_ = std::numeric_limits<UnderlyingType>::max();
            UnderlyingType id_;   
    };
} //StorageEngine