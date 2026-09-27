#include <gtest/gtest.h>
#include "initfile.hpp"
#include <iostream>

TEST(TestSuite, AsanTest)
{
    auto v = test_init();
    std::cout << v[5] << std::endl;
}