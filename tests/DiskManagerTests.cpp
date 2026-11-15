#include <storage_engine/DiskManager.hpp>
#include <storage_engine/Page.hpp>
#include <storage_engine/Errors/ErrorsHeader.hpp>
#include <gtest/gtest.h>
#include <type_traits>
#include <filesystem>
#include <string>

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
    EXPECT_EQ(DM.GetPageCount(), 0);
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
    EXPECT_EQ(DM1.GetPageCount(), 0);
}

TEST_F(DiskManagerFixture, OpenExistingNonEmptyFile)
{
    {
        StorageEngine::DiskManager DMToCreate(GetPath(), StorageEngine::DiskManagerMode::CreateNew);
    }

    std::filesystem::resize_file(GetPath(), StorageEngine::PageSize);

    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
        EXPECT_EQ(DM.GetPageCount(), 1);
    }

    std::filesystem::resize_file(GetPath(), StorageEngine::PageSize * 2);

    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
        EXPECT_EQ(DM.GetPageCount(), 2);
    }

    std::filesystem::resize_file(GetPath(), StorageEngine::PageSize * 50);

    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
        EXPECT_EQ(DM.GetPageCount(), 50);
    }

    std::filesystem::resize_file(GetPath(), StorageEngine::PageSize * 150);

    {
        StorageEngine::DiskManager DM(GetPath(), StorageEngine::DiskManagerMode::OpenExisting);
        EXPECT_EQ(DM.GetPageCount(), 150);
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