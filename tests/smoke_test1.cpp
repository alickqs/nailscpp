#include "photo_repository/personal_repo.h"
#include "photo_repository/shared_repo.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <string>
//тест на работу photo_repository - не учитавает tgbot и поиск (11.03)

namespace fs = std::filesystem;

namespace {
class SmokeTest1 : public ::testing::Test {
protected:
    void SetUp() override {
        fs::remove_all(workspace_);
        fs::create_directories(downloadsRoot_);
    }

    fs::path sourceRoot_{NAILSCPP_SOURCE_DIR};
    fs::path binaryRoot_{NAILSCPP_BINARY_DIR};
    fs::path workspace_{binaryRoot_ / "test_runs" / "smoke"};
    fs::path personalRoot_{workspace_ / "personal_repo"};
    fs::path sharedRoot_{workspace_ / "shared_repo"};
    fs::path downloadsRoot_{workspace_ / "downloads"};
    fs::path sourcePhoto_{sourceRoot_ / "test_data" / "source" / "photo1.jpg"};
    fs::path downloadedPhoto_{downloadsRoot_ / "from_shared.jpg"};
};

}

TEST_F(SmokeTest1, UploadExportAndDownloadFlowWorks) {
    PersonalRepository personal(personalRoot_, "smoke-user");
    SharedRepository shared(sharedRoot_);

    const PhotoId personalId = personal.uploadFromDevice(sourcePhoto_, "smoke test upload");
    EXPECT_TRUE(personal.contains(personalId));
    EXPECT_EQ(personal.size(), 1U);

    const PhotoId sharedId = personal.exportPhotoToShared(personalId, shared, false);
    EXPECT_TRUE(shared.contains(sharedId));
    EXPECT_EQ(shared.size(), 1U);

    shared.downloadToDevice(sharedId, downloadedPhoto_);
    EXPECT_TRUE(fs::exists(downloadedPhoto_));
    EXPECT_GT(fs::file_size(downloadedPhoto_), 0U);

    const auto sharedPhoto = shared.getPhotoInfo(sharedId);
    ASSERT_TRUE(sharedPhoto.has_value());
    EXPECT_EQ(sharedPhoto->description, "smoke test upload");
}
