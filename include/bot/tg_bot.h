#ifndef TG_BOT_H
#define TG_BOT_H

#include <string>
#include <functional>
#include <vector>
#include <utility>
#include <map>
#include <nlohmann/json.hpp>

struct ManicureData {
    std::string photoUrl;
    std::string description;
    std::string userId;
    std::string timestamp;
};

class TelegramBot {
private:
    std::string botToken;
    std::string apiUrl;
    std::string lastUpdateId;
    std::map<std::string, ManicureData> manicureStorage; //ключ-id, значение-структура маникюра
    std::map<std::string, std::string> userStates; //ключ-id, значение-текущее состояние(ожидание ссылки или описания)
    std::map<std::string, std::string> tempPhotoUrls; //временное хранение ссылки перед получением описания

    //работа с кнопочками
    nlohmann::json createInlineKeyboard(const std::vector<std::vector<std::pair<std::string, std::string>>>& buttons);
    void sendMessageWithKeyboard(const std::string& chatId, const std::string& text, const nlohmann::json& keyboard);
    void handleCallbackQuery(const nlohmann::json& callbackQuery);

    //работа с самим маникюром
    void handleManicureRequest(const std::string& chatId);
    void saveManicureData(const std::string& chatId, const std::string& photoUrl, const std::string& description);
    void showManicureData(const std::string& chatId, const std::string& dataId);

public:
    TelegramBot(const std::string& token);
    void start();
    void handleUpdates(const nlohmann::json& updates);
    void sendMessage(const std::string& chatId, const std::string& text);
    nlohmann::json makeRequest(const std::string& method, const nlohmann::json& payload = {});
};

#endif // TG_BOT_H