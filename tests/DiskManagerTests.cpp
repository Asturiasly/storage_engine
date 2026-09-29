#include <gtest/gtest.h>
#include "initfile.hpp"

TEST(TestSuite, DiskManager)
{
    int v = do_nothing();
    EXPECT_EQ(v, 1);
}