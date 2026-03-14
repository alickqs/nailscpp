#include "bot/tg_bot.h"
#include "photo_repository/personal_repo.h"
#include "photo_repository/shared_repo.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <memory>
#include <string>

namespace fs = std::filesystem;

namespace {
class TgBotPhotoRepositoryIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs::remove_all(workspace_);
        fs::create_directories(workspace_);
        fs::create_directories(downloadsRoot_);
    }

    fs::path sourceRoot_{NAILSCPP_SOURCE_DIR};
    fs::path binaryRoot_{NAILSCPP_BINARY_DIR};
    fs::path workspace_{binaryRoot_ / "test_runs" / "tg_bot_photo_integration"};
    fs::path personalRoot_{workspace_ / "personal_repo"};
    fs::path sharedRoot_{workspace_ / "shared_repo"};
    fs::path downloadsRoot_{workspace_ / "downloads"};
    fs::path sourcePhoto_{sourceRoot_ / "test_data" / "source" / "photo1.jpg"};
    fs::path botStorageFile_{workspace_ / "bot_manicures.dat"};
};
} // namespace

TEST_F(TgBotPhotoRepositoryIntegrationTest, PhotoRepositoryDataCanBeStoredByBotRepository) {
    const std::string ownerId = "integration-user";

    PersonalRepository personal(personalRoot_, ownerId);
    SharedRepository shared(sharedRoot_);

    const PhotoId personalId = personal.uploadFromDevice(sourcePhoto_, "integration manicure");
    const PhotoId sharedId = personal.exportPhotoToShared(personalId, shared, false);

    const auto sharedInfo = shared.getPhotoInfo(sharedId);
    ASSERT_TRUE(sharedInfo.has_value());
    ASSERT_TRUE(fs::exists(sharedInfo->filePath));

    ManicureRepository botRepo;
    auto manicure = std::make_shared<ManicureData>();
    manicure->id = "shared_" + std::to_string(sharedId);
    manicure->description = sharedInfo->description;
    manicure->filePath = sharedInfo->filePath;
    manicure->createdAt = sharedInfo->createdAt;
    manicure->repoType = "shared";
    manicure->ownerId = ownerId;
    botRepo.add(manicure->id, manicure);

    const auto userItems = botRepo.getUserManicures(ownerId);
    ASSERT_EQ(userItems.size(), 1U);
    EXPECT_EQ(userItems.front()->description, "integration manicure");
    EXPECT_EQ(userItems.front()->filePath, sharedInfo->filePath);

    botRepo.saveToFile(botStorageFile_.string());
    ASSERT_TRUE(fs::exists(botStorageFile_));

    ManicureRepository restored;
    restored.loadFromFile(botStorageFile_.string());
    const auto restoredItem = restored.get(manicure->id);
    ASSERT_TRUE(restoredItem.has_value());
    EXPECT_EQ((*restoredItem)->description, "integration manicure");
    EXPECT_EQ((*restoredItem)->filePath, sharedInfo->filePath);
    ASSERT_TRUE((*restoredItem)->ownerId.has_value());
    EXPECT_EQ(*(*restoredItem)->ownerId, ownerId);
}
