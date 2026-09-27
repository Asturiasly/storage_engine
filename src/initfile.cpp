#include "initfile.hpp"

std::vector<int> test_init()
{
    std::vector<int> v;

    for (int i = 0; i < 5; ++i)
    {
        v.push_back(1);
    }

    return v;
}