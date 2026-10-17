#include <storage_engine/PageBuffer.hpp>
#include <gtest/gtest.h>

static_assert(StorageEngine::PageSize % StorageEngine::IOAlignSize == 0);
