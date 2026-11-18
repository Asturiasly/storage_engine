#include <storage_engine/DiskManager.hpp>
#include <storage_engine/Page.hpp>
#include <storage_engine/Errors/ErrorsHeader.hpp>
#include <gtest/gtest.h>
#include <type_traits>
#include <numeric>
#include <string>
#include <filesystem>

#include <unistd.h>

static_assert(!std::is_copy_constructible_v<StorageEngine::DiskManager>);
static_assert(!std::is_copy_assignable_v<StorageEngine::DiskManager>);
static_assert(!std::is_move_constructible_v<StorageEngine::DiskManager>);
static_assert(!std::is_move_assignable_v<StorageEngine::DiskManager>);
static_assert(std::is_const_v<std::remove_reference_t<decltype(std::declval<StorageEngine::DiskManager>().GetPath())>>);

std::filesystem::path GetTmpDir()
{
    return std::filesystem::temp_directory_path(); 
}

class DiskManagerFixture : public ::testing::Test
{
    public:
    void SetUp() override
    {
        const ::testing::TestInfo* test_info =
            ::testing::UnitTest::GetInstance()->current_test_info();
        pid_t pid = getpid(); // Get PID of current proc
        p_ = GetTmpDir().string() + "/" + std::string(test_info->name()) + "_" + std::to_string(pid);
    }

    void TearDown() override
    {
        std::error_code ec;
        std::filesystem::remove(p_, ec);
    }

    protected:
    const std::filesystem::path& GetPath() const noexcept
    {
        return p_;
    }

    private:
    std::filesystem::path p_;
};

template <typename Action>
void ExpectIOError(Action&& action, std::errc expected_code)
{
    try
    {
        action();
        FAIL() << "Expected IOError";
    }
    catch (const StorageEngine::IOError& e)
    {
        EXPECT_EQ(static_cast<std::errc>(e.GetErrorCode().value()), expected_code);
    }
    catch (...)
    {
        FAIL() << "Expected IOError, got something else";
    }
}

TEST_F(DiskManagerFixture, CreateNewFile)
{
    StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    EXPECT_TRUE(std::filesystem::is_regular_file(GetPath()));
    EXPECT_EQ(DM.GetCurrentPageCount(), 0);
}

TEST_F(DiskManagerFixture, CreateNewFileTwice)
{
    {
        StorageEngine::DiskManager DM0(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    }

    ExpectIOError(
        [this] 
        {
            StorageEngine::DiskManager DM1(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
        },
        std::errc::file_exists);
}

TEST_F(DiskManagerFixture, OpenUnexistingFile)
{
    ExpectIOError(
        [this] 
        {
            StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
        },
        std::errc::no_such_file_or_directory);
}


TEST_F(DiskManagerFixture, NewFileInUnexistingDir)
{
    ExpectIOError(
        [] 
        {
            StorageEngine::DiskManager DM(GetTmpDir().string() + "/unexisting_dir/some_file", StorageEngine::DiskManagerMode::CreateNew);
        },
        std::errc::no_such_file_or_directory);
}

TEST_F(DiskManagerFixture, OpenExistingEmptyFile)
{
    {
        StorageEngine::DiskManager DM0(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
        EXPECT_TRUE(std::filesystem::is_regular_file(GetPath()));
    }

    StorageEngine::DiskManager DM1(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
    EXPECT_EQ(DM1.GetCurrentPageCount(), 0);
}

TEST_F(DiskManagerFixture, OpenExistingNonEmptyFile)
{
    {
        StorageEngine::DiskManager DMToCreate(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    }

    std::filesystem::resize_file(GetPath(), StorageEngine::PageSize);

    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
        EXPECT_EQ(DM.GetCurrentPageCount(), 1);
    }

    std::filesystem::resize_file(GetPath(), StorageEngine::PageSize * 2);

    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
        EXPECT_EQ(DM.GetCurrentPageCount(), 2);
    }

    std::filesystem::resize_file(GetPath(), StorageEngine::PageSize * 50);

    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
        EXPECT_EQ(DM.GetCurrentPageCount(), 50);
    }

    std::filesystem::resize_file(GetPath(), StorageEngine::PageSize * 150);

    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
        EXPECT_EQ(DM.GetCurrentPageCount(), 150);
    }
}

TEST_F(DiskManagerFixture, PageBaseAllocation)
{
    StorageEngine::DiskManager DMToAllocate(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    EXPECT_EQ(DMToAllocate.GetCurrentPageCount(), 0);
    EXPECT_EQ(std::filesystem::file_size(GetPath()), 0);
    StorageEngine::PageId page = DMToAllocate.AllocatePage();
    EXPECT_EQ(DMToAllocate.GetCurrentPageCount(), 1);
    EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize);
    EXPECT_EQ(page.GetId(), 0);
}

TEST_F(DiskManagerFixture, PageAllocationReopen)
{
    {
        StorageEngine::DiskManager DMToAllocate(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
        EXPECT_EQ(DMToAllocate.GetCurrentPageCount(), 0);
        for (uintmax_t i = 0; i < 5; ++i)
        {
            StorageEngine::PageId page1 = DMToAllocate.AllocatePage();
            EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize * (i + 1));
            EXPECT_EQ(DMToAllocate.GetCurrentPageCount(), i + 1);
            EXPECT_EQ(page1.GetId(), i);
        }
    }

    StorageEngine::DiskManager DMReOpen(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
    EXPECT_EQ(DMReOpen.GetCurrentPageCount(), 5);
    EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize * 5);
    StorageEngine::PageId page2 = DMReOpen.AllocatePage();
    EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize * 6);
    EXPECT_EQ(DMReOpen.GetCurrentPageCount(), 6);
    EXPECT_EQ(page2.GetId(), 5);
}

TEST_F(DiskManagerFixture, WriteAndReadOperations)
{
    StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    EXPECT_EQ(DM.GetCurrentPageCount(), 0);

    StorageEngine::PageBuffer p1_buf_write;
    StorageEngine::PageBuffer p2_buf_write;
    StorageEngine::PageBuffer p3_buf_write;

    for (size_t i = 0; i < StorageEngine::PageSize; ++i)
    {
        p1_buf_write.View().data()[i] = static_cast<std::byte>(i + 1);
    }

    for (size_t i = StorageEngine::PageSize; i > 0; --i)
    {
        p2_buf_write.View().data()[i - 1] =  static_cast<std::byte>(i);
    }

    for (size_t i = 0; i < StorageEngine::PageSize; ++i)
    {
        p3_buf_write.View().data()[i] =  static_cast<std::byte>(i + 2);
    }

    StorageEngine::PageId page1 = DM.AllocatePage();
    EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize);
    DM.WritePage(page1, p1_buf_write.View());
    EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize);
    StorageEngine::PageId page2 = DM.AllocatePage();
    EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize * 2);
    DM.WritePage(page2, p2_buf_write.View());
    EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize * 2);
    StorageEngine::PageId page3 = DM.AllocatePage();
    EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize * 3);
    DM.WritePage(page3, p3_buf_write.View());
    EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize * 3);

    StorageEngine::PageBuffer p1_buf_read;
    StorageEngine::PageBuffer p2_buf_read;
    StorageEngine::PageBuffer p3_buf_read;

    DM.ReadPage(page1, p1_buf_read.View());

    for (size_t i = 0; i < StorageEngine::PageSize; ++i)
    {
        EXPECT_EQ(p1_buf_read.View().data()[i], static_cast<std::byte>(i + 1));
    }

    DM.ReadPage(page2, p2_buf_read.View());

    for (size_t i = StorageEngine::PageSize; i > 0; --i)
    {
        EXPECT_EQ(p2_buf_read.View().data()[i - 1], static_cast<std::byte>(i));
    }

    DM.ReadPage(page3, p3_buf_read.View());

    for (size_t i = 0; i < StorageEngine::PageSize; ++i)
    {
        EXPECT_EQ(p3_buf_read.View().data()[i], static_cast<std::byte>(i + 2));
    }
}

TEST_F(DiskManagerFixture, DataAfterReopen)
{
    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
        EXPECT_EQ(DM.GetCurrentPageCount(), 0);

        StorageEngine::PageBuffer p_buf_write;

        for (size_t i = 0; i < StorageEngine::PageSize; ++i)
        {
            p_buf_write.View().data()[i] = static_cast<std::byte>(i + 1);
        }

        EXPECT_EQ(p_buf_write.View().size(), StorageEngine::PageSize);

        StorageEngine::PageId page1 = DM.AllocatePage();
        EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize);
        DM.WritePage(page1, p_buf_write.View());
        EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize);
    }

    StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
    StorageEngine::PageBuffer p_buf_read;
    StorageEngine::PageId page{static_cast<StorageEngine::PageId::UnderlyingType>(0)};
    DM.ReadPage(page, p_buf_read.View());

    for (size_t i = 0; i < StorageEngine::PageSize; ++i)
    {
        EXPECT_EQ(p_buf_read.View().data()[i], static_cast<std::byte>(i + 1));
    }
}

TEST_F(DiskManagerFixture, WriteBeyondPages)
{
    StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    EXPECT_EQ(DM.GetCurrentPageCount(), 0);
    StorageEngine::PageId page{static_cast<StorageEngine::PageId::UnderlyingType>(0)};
    StorageEngine::PageBuffer p_buf_write;
    
    EXPECT_THROW(
        try
        {
            DM.WritePage(page, p_buf_write.View());
        }
        catch(const StorageEngine::CorruptedDataError& ex)
        {
            EXPECT_EQ(ex.GetPath(), GetPath());
            EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::WritePage);
            EXPECT_EQ(ex.GetReason(), StorageEngine::CorruptedDataError::Reason::PageOutOfRange);
            throw;
        }
        catch(...)
        {
            FAIL() << "Expected CorruptedDataError, got something else";
        }
    , StorageEngine::CorruptedDataError);
}

TEST_F(DiskManagerFixture, ReadBeyondPages)
{
    StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    EXPECT_EQ(DM.GetCurrentPageCount(), 0);
    StorageEngine::PageId page{static_cast<StorageEngine::PageId::UnderlyingType>(0)};
    StorageEngine::PageBuffer p_buf_read;
    
    EXPECT_THROW(
        try
        {
            DM.ReadPage(page, p_buf_read.View());
        }
        catch(const StorageEngine::CorruptedDataError& ex)
        {
            EXPECT_EQ(ex.GetPath(), GetPath());
            EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::ReadPage);
            EXPECT_EQ(ex.GetReason(), StorageEngine::CorruptedDataError::Reason::PageOutOfRange);
            throw;
        }
        catch(...)
        {
            FAIL() << "Expected CorruptedDataError, got something else";
        }
    , StorageEngine::CorruptedDataError);
}

TEST_F(DiskManagerFixture, GotInvalidPageForRead)
{
    StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    EXPECT_EQ(DM.GetCurrentPageCount(), 0);
    StorageEngine::PageId page{static_cast<StorageEngine::PageId::UnderlyingType>(StorageEngine::PageId::MaxPageCount)};
    StorageEngine::PageBuffer p_buf_read;
    EXPECT_DEBUG_DEATH(DM.ReadPage(page, p_buf_read.View()), "");
}

TEST_F(DiskManagerFixture, GotInvalidPageForWrite)
{
    StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    EXPECT_EQ(DM.GetCurrentPageCount(), 0);
    StorageEngine::PageId page{static_cast<StorageEngine::PageId::UnderlyingType>(StorageEngine::PageId::MaxPageCount)};
    StorageEngine::PageBuffer p_buf_write;
    EXPECT_DEBUG_DEATH(DM.WritePage(page, p_buf_write.View()), "");
}

TEST_F(DiskManagerFixture, SyncPages)
{
    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
        EXPECT_EQ(DM.GetCurrentPageCount(), 0);

        StorageEngine::PageBuffer p_buf_write;

        for (size_t i = 0; i < StorageEngine::PageSize; ++i)
        {
            p_buf_write.View().data()[i] = static_cast<std::byte>(i + 1);
        }

        EXPECT_EQ(p_buf_write.View().size(), StorageEngine::PageSize);

        StorageEngine::PageId page1 = DM.AllocatePage();
        EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize);
        DM.WritePage(page1, p_buf_write.View());
        EXPECT_EQ(std::filesystem::file_size(GetPath()), StorageEngine::PageSize);
        DM.Sync();
        //Two sincs is okay
        DM.Sync();
    }

    StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
    StorageEngine::PageBuffer p_buf_read;
    StorageEngine::PageId page{static_cast<StorageEngine::PageId::UnderlyingType>(0)};
    DM.ReadPage(page, p_buf_read.View());

    for (size_t i = 0; i < StorageEngine::PageSize; ++i)
    {
        EXPECT_EQ(p_buf_read.View().data()[i], static_cast<std::byte>(i + 1));
    }
}

TEST_F(DiskManagerFixture, NonMultipleOfPageSizeFile)
{
    {
        StorageEngine::DiskManager DMToCreate(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    }
    std::filesystem::resize_file(GetPath(), 3071);
    EXPECT_THROW(
        {
            try
            {
                StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
            }
            catch(const StorageEngine::CorruptedDataError& ex)
            {
                EXPECT_EQ(ex.GetPath(), GetPath());
                EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::Open);
                EXPECT_TRUE(!std::string(ex.what()).empty());
                throw;
            }
        }, StorageEngine::CorruptedDataError
    );
}

TEST_F(DiskManagerFixture, FileGrants)
{
    {
        StorageEngine::DiskManager DMToCreate(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    }

    const auto st = std::filesystem::status(GetPath());
    EXPECT_EQ(st.permissions(),
            std::filesystem::perms::owner_read |
            std::filesystem::perms::owner_write);
}