#include "photo_repository/personal_repo.h"
#include "photo_repository/shared_repo.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>

namespace fs = std::filesystem;

namespace {
class PhotoRepositorySanityTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs::remove_all(workspace_);
        fs::create_directories(workspace_);
        fs::create_directories(downloadsRoot_);
    }

    fs::path sourceRoot_{NAILSCPP_SOURCE_DIR};
    fs::path binaryRoot_{NAILSCPP_BINARY_DIR};
    fs::path workspace_{binaryRoot_ / "test_runs" / "sanity"};
    fs::path personalRoot_{workspace_ / "personal_repo"};
    fs::path sharedRoot_{workspace_ / "shared_repo"};
    fs::path downloadsRoot_{workspace_ / "downloads"};
    fs::path sourcePhoto_{sourceRoot_ / "test_data" / "source" / "photo1.jpg"};
    fs::path missingPhoto_{sourceRoot_ / "test_data" / "source" / "does_not_exist.jpg"};
};
}

TEST_F(PhotoRepositorySanityTest, UploadRejectsEmptyDescription) {
    PersonalRepository personal(personalRoot_, "sanity-user");

    EXPECT_THROW(personal.uploadFromDevice(sourcePhoto_, ""), InvalidPhotoError);
}

TEST_F(PhotoRepositorySanityTest, UploadRejectsMissingSourceFile) {
    PersonalRepository personal(personalRoot_, "sanity-user");

    EXPECT_THROW(personal.uploadFromDevice(missingPhoto_, "desc"), FileOperationError);
}

TEST_F(PhotoRepositorySanityTest, ListPhotosSupportsPagination) {
    PersonalRepository personal(personalRoot_, "sanity-user");

    const PhotoId firstId = personal.uploadFromDevice(sourcePhoto_, "first");
    const PhotoId secondId = personal.uploadFromDevice(sourcePhoto_, "second");
    const PhotoId thirdId = personal.uploadFromDevice(sourcePhoto_, "third");

    const auto page0 = personal.listPhotos(0, 2);
    const auto page1 = personal.listPhotos(1, 2);
    const auto page2 = personal.listPhotos(2, 2);

    ASSERT_EQ(page0.size(), 2U);
    ASSERT_EQ(page1.size(), 1U);
    ASSERT_TRUE(page2.empty());

    EXPECT_EQ(page0[0].id, firstId);
    EXPECT_EQ(page0[1].id, secondId);
    EXPECT_EQ(page1[0].id, thirdId);
}

TEST_F(PhotoRepositorySanityTest, MetadataPersistsBetweenRepositoryInstances) {
    PhotoId uploadedId{};
    {
        PersonalRepository personal(personalRoot_, "sanity-user");
        uploadedId = personal.uploadFromDevice(sourcePhoto_, "persist me");
        ASSERT_TRUE(personal.contains(uploadedId));
        ASSERT_EQ(personal.size(), 1U);
    }

    {
        PersonalRepository reopened(personalRoot_, "sanity-user");
        EXPECT_TRUE(reopened.contains(uploadedId));
        ASSERT_EQ(reopened.size(), 1U);

        const auto info = reopened.getPhotoInfo(uploadedId);
        ASSERT_TRUE(info.has_value());
        EXPECT_EQ(info->description, "persist me");
        EXPECT_FALSE(info->filePath.empty());
    }
}

TEST_F(PhotoRepositorySanityTest, ExportToSharedCanRemoveOriginalFromPersonal) {
    PersonalRepository personal(personalRoot_, "sanity-user");
    SharedRepository shared(sharedRoot_);

    const PhotoId personalId = personal.uploadFromDevice(sourcePhoto_, "to shared");
    const PhotoId sharedId = personal.exportPhotoToShared(personalId, shared, true);

    EXPECT_FALSE(personal.contains(personalId));
    EXPECT_EQ(personal.size(), 0U);
    EXPECT_TRUE(shared.contains(sharedId));
    EXPECT_EQ(shared.size(), 1U);

    const auto sharedInfo = shared.getPhotoInfo(sharedId);
    ASSERT_TRUE(sharedInfo.has_value());
    EXPECT_EQ(sharedInfo->description, "to shared");
}

TEST_F(PhotoRepositorySanityTest, DownloadThrowsForUnknownPhoto) {
    SharedRepository shared(sharedRoot_);

    EXPECT_THROW(shared.downloadToDevice(9999, downloadsRoot_ / "missing.jpg"), PhotoNotFoundError);
}
