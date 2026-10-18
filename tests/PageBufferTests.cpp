#include <storage_engine/PageBuffer.hpp>
#include <gtest/gtest.h>
#include <algorithm>
#include <cstdint>
#include <type_traits>
#include <memory>
#include <utility>

static_assert(!std::is_copy_constructible_v<StorageEngine::PageBuffer>);
static_assert(!std::is_copy_assignable_v<StorageEngine::PageBuffer>);
static_assert(std::is_move_constructible_v<StorageEngine::PageBuffer>);
static_assert(std::is_move_assignable_v<StorageEngine::PageBuffer>);
static_assert(std::is_nothrow_move_constructible_v<StorageEngine::PageBuffer>);
static_assert(std::is_nothrow_move_assignable_v<StorageEngine::PageBuffer>);
static_assert(decltype(std::declval<StorageEngine::PageBuffer>().View())::extent == StorageEngine::PageSize);
static_assert(!std::is_const_v<decltype(std::declval<StorageEngine::PageBuffer>().View())::element_type>);
static_assert(decltype(std::declval<const StorageEngine::PageBuffer&>().View())::extent == StorageEngine::PageSize);
static_assert(std::is_const_v<decltype(std::declval<const StorageEngine::PageBuffer&>().View())::element_type>);


TEST(PageBuffer, Alignment)
{
    StorageEngine::PageBuffer Buffer{};
    EXPECT_EQ(reinterpret_cast<std::uintptr_t>(Buffer.View().data()) % StorageEngine::IOAlignSize, 0)
    << "buffer address = " << Buffer.View().data()
    << ", IOAlignSize = " << StorageEngine::IOAlignSize;
}

TEST(PageBuffer, NullBytesOnDefault)
{
    StorageEngine::PageBuffer Buffer{};
    auto BufView = Buffer.View();
    EXPECT_TRUE(std::all_of(BufView.begin(), BufView.end(),
        [](std::byte b)
        {
            return b == std::byte{0};
        })
    );
}

TEST(PageBuffer, RWView)
{
    StorageEngine::PageBuffer Buffer{};
    Buffer.View().data()[0] = std::byte{1};
    EXPECT_EQ(Buffer.View().data()[0], std::byte{1});
    EXPECT_EQ(Buffer.View().data()[1], std::byte{0});
}

TEST(PageBuffer, MoveableConstrAssign)
{
    StorageEngine::PageBuffer OldBuffer{};
    StorageEngine::PageBuffer NewBuffer{};
    OldBuffer.View().data()[0] = std::byte{1};
    NewBuffer = std::move(OldBuffer);
    EXPECT_EQ(NewBuffer.View()[0], std::byte{1});
    EXPECT_DEBUG_DEATH((void)OldBuffer.View().data(), "");
}

TEST(PageBuffer, MoveableConstrCreation)
{
    StorageEngine::PageBuffer OldBuffer{};
    OldBuffer.View().data()[0] = std::byte{1};
    StorageEngine::PageBuffer NewBuffer(std::move(OldBuffer));
    EXPECT_EQ(NewBuffer.View()[0], std::byte{1});
    EXPECT_DEBUG_DEATH((void)OldBuffer.View().data(), "");
}

TEST(PageBuffer, NonEmptyTargetPageBuffer)
{
    StorageEngine::PageBuffer OldBuffer{};
    StorageEngine::PageBuffer NewBuffer{};
    NewBuffer.View().data()[0] = std::byte{1};
    NewBuffer = std::move(OldBuffer);
    EXPECT_EQ(NewBuffer.View().data()[0], std::byte{0});
}