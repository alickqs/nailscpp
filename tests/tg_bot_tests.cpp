#include "bot/tg_bot.h"

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {
class TgBotRepositoryTest : public ::testing::Test {
protected:
    void SetUp() override {
        fs::remove_all(workspace_);
        fs::create_directories(workspace_);
        storageFile_ = workspace_ / "manicures_test.dat";
    }

    std::shared_ptr<ManicureData> makeData(const std::string& id,
                                           const std::string& owner,
                                           const std::string& description,
                                           std::chrono::system_clock::time_point createdAt) {
        auto data = std::make_shared<ManicureData>();
        data->id = id;
        data->description = description;
        data->filePath = workspace_ / (id + ".jpg");
        data->createdAt = createdAt;
        data->repoType = "memory";
        data->ownerId = owner;
        return data;
    }

    static void writeBytes(const fs::path& filePath, const std::vector<char>& bytes) {
        std::ofstream out(filePath, std::ios::binary);
        ASSERT_TRUE(out.is_open());
        out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
        out.close();
    }

    fs::path binaryRoot_{NAILSCPP_BINARY_DIR};
    fs::path sourceRoot_{NAILSCPP_SOURCE_DIR};
    fs::path workspace_{binaryRoot_ / "test_runs" / "tg_bot_repo"};
    fs::path storageFile_;
};
}

TEST_F(TgBotRepositoryTest, AddGetAndSizeWork) {
    ManicureRepository repo;
    const auto now = std::chrono::system_clock::now();
    repo.add("m1", makeData("m1", "u1", "red nails", now));

    EXPECT_EQ(repo.size(), 1U);
    const auto item = repo.get("m1");
    ASSERT_TRUE(item.has_value());
    EXPECT_EQ((*item)->description, "red nails");
    ASSERT_TRUE((*item)->ownerId.has_value());
    EXPECT_EQ(*(*item)->ownerId, "u1");
}

TEST_F(TgBotRepositoryTest, AddRejectsNullData) {
    ManicureRepository repo;
    EXPECT_THROW(repo.add("bad", nullptr), std::invalid_argument);
}

TEST_F(TgBotRepositoryTest, GetLastUserManicureReturnsLatestByTimestamp) {
    ManicureRepository repo;
    const auto base = std::chrono::system_clock::now();
    repo.add("a1", makeData("a1", "alice", "old", base - std::chrono::minutes(10)));
    repo.add("a2", makeData("a2", "alice", "new", base + std::chrono::minutes(5)));
    repo.add("b1", makeData("b1", "bob", "other user", base + std::chrono::minutes(20)));

    const auto latest = repo.getLastUserManicure("alice");
    ASSERT_TRUE(latest.has_value());
    EXPECT_EQ((*latest)->id, "a2");
    EXPECT_EQ((*latest)->description, "new");
}

TEST_F(TgBotRepositoryTest, GetLastUserManicureReturnsNulloptWhenUserMissing) {
    ManicureRepository repo;
    repo.add("m1", makeData("m1", "u1", "red", std::chrono::system_clock::now()));

    const auto latest = repo.getLastUserManicure("u2");
    EXPECT_FALSE(latest.has_value());
}

TEST_F(TgBotRepositoryTest, GetUserManicuresReturnsOnlyRequestedOwner) {
    ManicureRepository repo;
    const auto now = std::chrono::system_clock::now();
    repo.add("u1_1", makeData("u1_1", "u1", "first", now));
    repo.add("u2_1", makeData("u2_1", "u2", "second", now));
    repo.add("u1_2", makeData("u1_2", "u1", "third", now));

    const auto userItems = repo.getUserManicures("u1");
    ASSERT_EQ(userItems.size(), 2U);
    for (const auto& item : userItems) {
        ASSERT_TRUE(item->ownerId.has_value());
        EXPECT_EQ(*item->ownerId, "u1");
    }
}

TEST_F(TgBotRepositoryTest, SearchAndRemoveBehaveCorrectly) {
    ManicureRepository repo;
    const auto now = std::chrono::system_clock::now();
    repo.add("m1", makeData("m1", "u1", "nude with glitter", now));
    repo.add("m2", makeData("m2", "u1", "french style", now));

    const auto found = repo.search("glitter");
    ASSERT_EQ(found.size(), 1U);
    EXPECT_EQ(found[0]->id, "m1");

    const auto missingRemove = repo.remove("unknown");
    EXPECT_FALSE(missingRemove.has_value());

    const auto okRemove = repo.remove("m1");
    ASSERT_TRUE(okRemove.has_value());
    EXPECT_TRUE(*okRemove);
    EXPECT_EQ(repo.size(), 1U);
}

TEST_F(TgBotRepositoryTest, SearchIsCaseSensitive) {
    ManicureRepository repo;
    const auto now = std::chrono::system_clock::now();
    repo.add("m1", makeData("m1", "u1", "Glitter French", now));

    EXPECT_TRUE(repo.search("Glitter").size() == 1U);
    EXPECT_TRUE(repo.search("glitter").empty());
}

TEST_F(TgBotRepositoryTest, RemoveDeletesFileFromDiskWhenPresent) {
    ManicureRepository repo;
    const auto now = std::chrono::system_clock::now();
    auto item = makeData("m1", "u1", "with file", now);

    writeBytes(item->filePath, {'a', 'b', 'c'});
    ASSERT_TRUE(fs::exists(item->filePath));

    repo.add("m1", item);
    const auto removed = repo.remove("m1");

    ASSERT_TRUE(removed.has_value());
    EXPECT_TRUE(*removed);
    EXPECT_FALSE(fs::exists(item->filePath));
    EXPECT_EQ(repo.size(), 0U);
}

TEST_F(TgBotRepositoryTest, ClearRemovesAllEntriesAndDeletesFiles) {
    ManicureRepository repo;
    const auto now = std::chrono::system_clock::now();

    auto first = makeData("m1", "u1", "first", now);
    auto second = makeData("m2", "u1", "second", now);

    writeBytes(first->filePath, {'1'});
    writeBytes(second->filePath, {'2'});

    repo.add("m1", first);
    repo.add("m2", second);
    ASSERT_EQ(repo.size(), 2U);

    repo.clear();

    EXPECT_EQ(repo.size(), 0U);
    EXPECT_FALSE(fs::exists(first->filePath));
    EXPECT_FALSE(fs::exists(second->filePath));
}

TEST_F(TgBotRepositoryTest, SaveAndLoadRoundTripPreservesData) {
    ManicureRepository repo;
    const auto now = std::chrono::system_clock::now();
    repo.add("m1", makeData("m1", "u1", "cat eye", now));
    repo.add("m2", makeData("m2", "u2", "pastel", now + std::chrono::minutes(1)));

    repo.saveToFile(storageFile_.string());

    ManicureRepository restored;
    restored.loadFromFile(storageFile_.string());

    EXPECT_EQ(restored.size(), 2U);
    const auto m1 = restored.get("m1");
    const auto m2 = restored.get("m2");
    ASSERT_TRUE(m1.has_value());
    ASSERT_TRUE(m2.has_value());
    EXPECT_EQ((*m1)->description, "cat eye");
    EXPECT_EQ((*m2)->description, "pastel");
    ASSERT_TRUE((*m2)->ownerId.has_value());
    EXPECT_EQ(*(*m2)->ownerId, "u2");
}

TEST_F(TgBotRepositoryTest, SaveAndLoadPreservesMissingOwnerId) {
    ManicureRepository repo;
    auto item = std::make_shared<ManicureData>();
    item->id = "no_owner";
    item->description = "ownerless";
    item->filePath = workspace_ / "no_owner.jpg";
    item->createdAt = std::chrono::system_clock::now();
    item->repoType = "memory";
    item->ownerId = std::nullopt;

    repo.add(item->id, item);
    repo.saveToFile(storageFile_.string());

    ManicureRepository restored;
    restored.loadFromFile(storageFile_.string());

    const auto restoredItem = restored.get(item->id);
    ASSERT_TRUE(restoredItem.has_value());
    EXPECT_FALSE((*restoredItem)->ownerId.has_value());
}

TEST_F(TgBotRepositoryTest, LoadFromMalformedFileThrowsRuntimeError) {
    const fs::path malformed = workspace_ / "malformed.dat";
    {
        std::ofstream out(malformed);
        ASSERT_TRUE(out.is_open());
        out << "{invalid_json}" << std::endl;
    }

    ManicureRepository repo;
    EXPECT_THROW(repo.loadFromFile(malformed.string()), std::runtime_error);
}

TEST_F(TgBotRepositoryTest, LoadFromMissingFileThrows) {
    ManicureRepository repo;
    EXPECT_THROW(repo.loadFromFile((workspace_ / "does_not_exist.dat").string()), std::runtime_error);
}

TEST_F(TgBotRepositoryTest, GenerateRecommendationReturnsImageAndText) {
    ManicureData lastManicure;
    lastManicure.id = "seed";
    lastManicure.description = "френч с блестками";
    lastManicure.filePath = sourceRoot_ / "test_data" / "source" / "photo1.jpg";
    lastManicure.createdAt = std::chrono::system_clock::now();
    lastManicure.repoType = "memory";
    lastManicure.ownerId = "u1";

    RecommendationData recommendation = generateNextManicureRecommendation(lastManicure);

    EXPECT_FALSE(recommendation.description.empty());
    EXPECT_NE(recommendation.description.find("Похожий маникюр"), std::string::npos);
    EXPECT_FALSE(recommendation.imageData.empty());
    EXPECT_TRUE(recommendation.imageFormat == "jpg" || recommendation.imageFormat == "png");
}
