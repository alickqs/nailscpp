#include "bot/tg_bot.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace {
struct OutboundCall {
    std::string method;
    nlohmann::json payload;
};

class TelegramBotCommandsTest : public ::testing::Test {
protected:
    void SetUp() override {
        bot_.setRequestHandlerForTests([this](const std::string& method, const nlohmann::json& payload) {
            calls_.push_back({method, payload});
            return nlohmann::json{{"ok", true}, {"result", nlohmann::json::object()}};
        });
    }

    static nlohmann::json messageUpdate(int updateId, int chatId, const std::string& text) {
        return nlohmann::json::array({
            {
                {"update_id", updateId},
                {"message",
                    {
                        {"message_id", 1},
                        {"chat", {{"id", chatId}, {"type", "private"}}},
                        {"text", text}
                    }}
            }
        });
    }

    std::vector<OutboundCall> findCallsByMethod(const std::string& method) const {
        std::vector<OutboundCall> filtered;
        for (const auto& call : calls_) {
            if (call.method == method) {
                filtered.push_back(call);
            }
        }
        return filtered;
    }

    TelegramBot bot_{"test-token"};
    std::vector<OutboundCall> calls_;
};
}  // namespace

TEST_F(TelegramBotCommandsTest, StartCommandShowsMainMenu) {
    bot_.handleUpdates(messageUpdate(1, 101, "/start"));

    const auto sendMessageCalls = findCallsByMethod("sendMessage");
    ASSERT_FALSE(sendMessageCalls.empty());

    const auto& payload = sendMessageCalls.back().payload;
    ASSERT_TRUE(payload.contains("reply_markup"));
    ASSERT_TRUE(payload["reply_markup"].contains("inline_keyboard"));
    EXPECT_NE(payload["text"].get<std::string>().find("Главное меню"), std::string::npos);
}

TEST_F(TelegramBotCommandsTest, MenuCommandShowsMainMenu) {
    bot_.handleUpdates(messageUpdate(2, 101, "/menu"));

    const auto sendMessageCalls = findCallsByMethod("sendMessage");
    ASSERT_FALSE(sendMessageCalls.empty());
    EXPECT_NE(sendMessageCalls.back().payload["text"].get<std::string>().find("Главное меню"), std::string::npos);
}

TEST_F(TelegramBotCommandsTest, NewCommandRequestsPhotoAndSetsInlineCancelButton) {
    bot_.handleUpdates(messageUpdate(3, 101, "/new"));

    const auto sendMessageCalls = findCallsByMethod("sendMessage");
    ASSERT_FALSE(sendMessageCalls.empty());

    const auto& payload = sendMessageCalls.back().payload;
    EXPECT_NE(payload["text"].get<std::string>().find("Отправьте фотографию"), std::string::npos);
    ASSERT_TRUE(payload.contains("reply_markup"));
    const auto& keyboard = payload["reply_markup"]["inline_keyboard"];
    ASSERT_FALSE(keyboard.empty());
    ASSERT_FALSE(keyboard[0].empty());
    EXPECT_EQ(keyboard[0][0]["callback_data"].get<std::string>(), "cancel");
}

TEST_F(TelegramBotCommandsTest, HelpCommandReturnsCommandsList) {
    bot_.handleUpdates(messageUpdate(4, 101, "/help"));

    const auto sendMessageCalls = findCallsByMethod("sendMessage");
    ASSERT_FALSE(sendMessageCalls.empty());
    const std::string text = sendMessageCalls.back().payload["text"].get<std::string>();

    EXPECT_NE(text.find("/start"), std::string::npos);
    EXPECT_NE(text.find("/menu"), std::string::npos);
    EXPECT_NE(text.find("/new"), std::string::npos);
    EXPECT_NE(text.find("/cancel"), std::string::npos);
    EXPECT_NE(text.find("/help"), std::string::npos);
}

TEST_F(TelegramBotCommandsTest, CancelCommandClearsFlowAndShowsMainMenu) {
    bot_.handleUpdates(messageUpdate(5, 101, "/new"));
    bot_.handleUpdates(messageUpdate(6, 101, "/cancel"));

    const auto sendMessageCalls = findCallsByMethod("sendMessage");
    ASSERT_GE(sendMessageCalls.size(), 3U);

    bool hasCancelledMessage = false;
    bool hasMainMenuMessage = false;
    for (const auto& call : sendMessageCalls) {
        const std::string text = call.payload.value("text", "");
        if (text.find("Действие отменено") != std::string::npos) {
            hasCancelledMessage = true;
        }
        if (text.find("Главное меню") != std::string::npos) {
            hasMainMenuMessage = true;
        }
    }

    EXPECT_TRUE(hasCancelledMessage);
    EXPECT_TRUE(hasMainMenuMessage);
}

TEST_F(TelegramBotCommandsTest, UnknownCommandReturnsFallbackMessage) {
    bot_.handleUpdates(messageUpdate(7, 101, "/abracadabra"));

    const auto sendMessageCalls = findCallsByMethod("sendMessage");
    ASSERT_FALSE(sendMessageCalls.empty());
    EXPECT_NE(sendMessageCalls.back().payload["text"].get<std::string>().find("Неизвестная команда"),
              std::string::npos);
}
