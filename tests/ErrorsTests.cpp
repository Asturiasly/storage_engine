#include <storage_engine/Common/Operations.hpp>
#include <storage_engine/Errors/ErrorsHeader.hpp>
#include <storage_engine/Page.hpp>

#include <gtest/gtest.h>

#include <optional>
#include <string>
#include <filesystem>

static_assert(std::is_nothrow_copy_constructible_v<StorageEngine::CorruptedDataError>);
static_assert(std::is_nothrow_copy_constructible_v<StorageEngine::ResourceExhaustionError>);
static_assert(std::is_nothrow_copy_constructible_v<StorageEngine::IOError>);



TEST(ErrorsTests, CorruptedData)
{
    std::filesystem::path path("/unexisting_path");
    EXPECT_THROW(
        {
            try
            {
                throw StorageEngine::CorruptedDataError(StorageEngine::ErrorData(StorageEngine::Operation::Open, path), 
                StorageEngine::CorruptedDataError::Reason::SizeNotMultipleOfPage);
            }
            catch(const StorageEngine::CorruptedDataError& ex)
            {
                EXPECT_TRUE(!std::string(ex.what()).empty());
                EXPECT_EQ(ex.GetPath(), path);
                EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::Open);
                EXPECT_EQ(ex.GetReason(), StorageEngine::CorruptedDataError::Reason::SizeNotMultipleOfPage);
                EXPECT_EQ(ex.GetPage(), std::nullopt);
                throw;
            }
        }, StorageEngine::CorruptedDataError 
    );
}

TEST(ErrorsTests, ResourceExhaustion)
{
    std::filesystem::path path("/unexisting_path");
    EXPECT_THROW(
        {
            try
            {
                throw StorageEngine::ResourceExhaustionError(StorageEngine::ErrorData(StorageEngine::Operation::Open, path), 
                StorageEngine::ResourceExhaustionError::Reason::ExhaustedPages);
            }
            catch(const StorageEngine::ResourceExhaustionError& ex)
            {
                EXPECT_TRUE(!std::string(ex.what()).empty());
                EXPECT_EQ(ex.GetPath(), path);
                EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::Open);
                EXPECT_EQ(ex.GetReason(), StorageEngine::ResourceExhaustionError::Reason::ExhaustedPages);
                EXPECT_EQ(ex.GetPage(), std::nullopt);
                throw;
            }
        }, StorageEngine::ResourceExhaustionError
    );
}

TEST(ErrorsTests, IOError)
{
    std::filesystem::path path("/unexisting_path");
    EXPECT_THROW(
        {
            try
            {
                throw StorageEngine::IOError(StorageEngine::ErrorData(StorageEngine::Operation::Open, path), 
                std::make_error_code(std::errc::file_exists));
            }
            catch(const StorageEngine::IOError& ex)
            {
                EXPECT_TRUE(!std::string(ex.what()).empty());
                EXPECT_EQ(ex.GetPath(), path);
                EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::Open);
                EXPECT_EQ(ex.GetErrorCode(), std::make_error_code(std::errc::file_exists));
                EXPECT_EQ(ex.GetPage(), std::nullopt);
                throw;
            }
        }, StorageEngine::IOError 
    );
}

TEST(ErrorsTests, AnyExceptionFromBase)
{
    std::filesystem::path path("/unexisting_path");
    EXPECT_THROW(
        {
            try
            {
                throw StorageEngine::CorruptedDataError(StorageEngine::ErrorData(StorageEngine::Operation::Open, path), 
                StorageEngine::CorruptedDataError::Reason::SizeNotMultipleOfPage);
            }
            catch(const StorageEngine::StorageError& ex)
            {
                EXPECT_TRUE(!std::string(ex.what()).empty());
                EXPECT_EQ(ex.GetPath(), path);
                EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::Open);
                EXPECT_EQ(ex.GetPage(), std::nullopt);
                throw;
            }
        }, StorageEngine::StorageError
    );

    EXPECT_THROW(
        {
            try
            {
                throw StorageEngine::ResourceExhaustionError(StorageEngine::ErrorData(StorageEngine::Operation::Open, path), 
                StorageEngine::ResourceExhaustionError::Reason::ExhaustedPages);
            }
            catch(const StorageEngine::StorageError& ex)
            {
                EXPECT_TRUE(!std::string(ex.what()).empty());
                EXPECT_EQ(ex.GetPath(), path);
                EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::Open);
                EXPECT_EQ(ex.GetPage(), std::nullopt);
                throw;
            }
        }, StorageEngine::StorageError
    );

    EXPECT_THROW(
        {
            try
            {
                throw StorageEngine::IOError(StorageEngine::ErrorData(StorageEngine::Operation::Open, path), 
                std::make_error_code(std::errc::file_exists));
            }
            catch(const StorageEngine::StorageError& ex)
            {
                EXPECT_TRUE(!std::string(ex.what()).empty());
                EXPECT_EQ(ex.GetPath(), path);
                EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::Open);
                EXPECT_EQ(ex.GetPage(), std::nullopt);
                throw;
            }
        }, StorageEngine::StorageError
    );
}

TEST(ErrorsTests, ExplicitPageInConstructor)
{
    std::filesystem::path path("/unexisting_path");
    StorageEngine::PageId page(static_cast<StorageEngine::PageId::UnderlyingType>(5));
    EXPECT_THROW(
        {
            try
            {
                throw StorageEngine::CorruptedDataError(StorageEngine::ErrorData(StorageEngine::Operation::Open, path, page), 
                StorageEngine::CorruptedDataError::Reason::SizeNotMultipleOfPage);
            }
            catch(const StorageEngine::CorruptedDataError& ex)
            {
                EXPECT_TRUE(!std::string(ex.what()).empty());
                EXPECT_EQ(ex.GetPath(), path);
                EXPECT_EQ(ex.GetOperation(), StorageEngine::Operation::Open);
                EXPECT_EQ(ex.GetReason(), StorageEngine::CorruptedDataError::Reason::SizeNotMultipleOfPage);
                EXPECT_EQ(page, ex.GetPage().value());
                throw;
            }
        }, StorageEngine::CorruptedDataError 
    );
}