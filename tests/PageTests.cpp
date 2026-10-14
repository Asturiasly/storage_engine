#include <bit>
#include <cstddef>
#include <limits>

#include <gtest/gtest.h>
#include <storage_engine/page.hpp>

constexpr StorageEngine::PageId p_max(std::numeric_limits<StorageEngine::PageId::UnderlyingType>::max());
constexpr StorageEngine::PageId p;

constexpr StorageEngine::PageId p0(static_cast<StorageEngine::PageId::UnderlyingType>(0));
constexpr StorageEngine::PageId p1(static_cast<StorageEngine::PageId::UnderlyingType>(0));
constexpr StorageEngine::PageId p2(static_cast<StorageEngine::PageId::UnderlyingType>(1));

static_assert(!p.IsValid(), "PageId default constructor of PageId object must leads to invalid state");
static_assert(p0.IsValid(), "PageId explicit initialization with value must leads to valid state");
static_assert(std::has_single_bit(StorageEngine::PageSize), "Page size must be a power of 2");
static_assert(p0 == p1, "PageId objects with same id's must be equal");
static_assert(p0 != p2, "PageId objects with different id's must be non equal");
static_assert(!(p0 != p1), "PageId objects with same id's must not be non equal");
static_assert(p0 < p2, "PageId with lesser id must be less than other");
static_assert(p2 > p0, "PageId with greater id must be greater than other");
static_assert(p0 >= p1, "The same PageId objects must be equal with ge operator");
static_assert(p0 <= p1, "The same PageId objects must be equal with le operator");
static_assert(p.GetId() == std::numeric_limits<StorageEngine::PageId::UnderlyingType>::max(), "Invalid PageId must return std::uint32_t::max() id");
static_assert(p0.GetId() == 0, "PageId with constructed by 0 value must return 0 id");
static_assert(p_max.GetId() == std::numeric_limits<StorageEngine::PageId::UnderlyingType>::max());
static_assert(!p_max.IsValid());

TEST(Pages, ComputeIdInRuntime)
{
    int x = 1;
    int y = 2;
    StorageEngine::PageId PIobj{static_cast<StorageEngine::PageId::UnderlyingType>(x + y)};
    EXPECT_TRUE(PIobj.GetId() == 3);
}