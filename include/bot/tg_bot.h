#ifndef TG_BOT_H
#define TG_BOT_H

#include <string>
#include <functional>
#include <vector>
#include <utility>
#include <map>
#include <optional>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <variant>
#include <thread>
#include <nlohmann/json.hpp>

// Структура данных маникюра
struct ManicureData {
    std::string id;  // PhotoId
    std::string description;
    std::filesystem::path filePath;
    std::chrono::system_clock::time_point createdAt;
    std::string repoType;
    std::optional<std::string> ownerId;
};

// Структура для рекомендации
struct RecommendationData {
    std::string description;           // Текстовое описание рекомендации
    std::vector<char> imageData;        // Данные PNG изображения
    std::string imageFormat;
};

// Функция для генерации рекомендации
RecommendationData generateNextManicureRecommendation(const ManicureData& lastManicure);

// Репозиторий для работы с маникюрами
class ManicureRepository {
private:
    std::map<std::string, std::shared_ptr<ManicureData>> storage;
    std::shared_ptr<std::mutex> storageMutex;

public:
    // конструктор и деструктор
    ManicureRepository();
    ~ManicureRepository() = default;

    //запрещаем копирование и присваивание, разрешаем перемещение
    ManicureRepository(const ManicureRepository&) = delete;
    ManicureRepository& operator=(const ManicureRepository&) = delete;
    ManicureRepository(ManicureRepository&&) = default;
    ManicureRepository& operator=(ManicureRepository&&) = default;

    //методы для работы с репозиторием
    void add(std::string id, std::shared_ptr<ManicureData> data);
    std::optional<std::shared_ptr<ManicureData>> get(const std::string& id) const;
    std::optional<std::shared_ptr<ManicureData>> getLastUserManicure(const std::string& userId) const;
    std::vector<std::shared_ptr<ManicureData>> getUserManicures(const std::string& userId) const;
    std::vector<std::shared_ptr<ManicureData>> search(const std::string& text) const;
    std::optional<bool> remove(const std::string& id);
    [[nodiscard]] size_t size() const noexcept;
    void clear() noexcept;
    void saveToFile(const std::string& filename) const;
    void loadFromFile(const std::string& filename);
};

// типы действий пользователя
struct ActionShowAll {};
struct ActionShowLast {};
struct ActionSearch {};
struct ActionDelete {
    std::string manicureId;
};
struct ActionRecommend {
    std::string manicureId;
};
struct ActionBack {};

using UserAction = std::variant<ActionShowAll, ActionShowLast, ActionSearch,
        ActionDelete, ActionRecommend, ActionBack>;

class TelegramBot;

class ActionVisitor {
private:
    TelegramBot* bot;
    std::string chatId;

public:
    // конструктор
    ActionVisitor(TelegramBot* b, const std::string& id) : bot(b), chatId(id) {}

    // методы для обработки действий пользователя
    void operator()(const ActionShowAll&) const;
    void operator()(const ActionShowLast&) const;
    void operator()(const ActionSearch&) const;
    void operator()(const ActionDelete& action) const;
    void operator()(const ActionRecommend& action) const;
    void operator()(const ActionBack&) const;
};

class TelegramBot {
private:
    std::string botToken;
    std::string apiUrl;
    std::string lastUpdateId;

    std::unique_ptr<ManicureRepository> repository;
    std::shared_ptr<class Logger> logger;

    std::map<std::string, std::string> userStates;
    std::map<std::string, std::string> tempPhotoUrls;
    std::map<std::string, std::string> searchQueries;
    std::map<std::string, int> userManicureCounters;

    struct CacheEntry {
        nlohmann::json data;
        std::chrono::steady_clock::time_point timestamp;
    };
    std::map<std::string, CacheEntry> responseCache;
    const std::chrono::seconds CACHE_TTL{30};

    struct ConnectionPool;
    std::vector<std::unique_ptr<ConnectionPool>> connectionPool;
    const size_t POOL_SIZE = 5;
    size_t currentConnection = 0;

    //обработка запросов
    nlohmann::json createInlineKeyboard(const std::vector<std::vector<std::pair<std::string, std::string>>>& buttons);
    void sendMessageWithKeyboard(const std::string& chatId, const std::string& text, const nlohmann::json& keyboard);
    void handleCallbackQuery(const nlohmann::json& callbackQuery);

    //методы для работы с маникюром
    void handleManicureRequest(const std::string& chatId);
    void saveManicureData(const std::string& chatId, const std::string& photoUrl, const std::string& description);
    void showManicureData(const std::string& chatId, const std::string& dataId);

    // методы для работы с кнопочками
    void showMainMenu(const std::string& chatId);
    void showManicureList(const std::string& chatId, const std::vector<std::shared_ptr<ManicureData>>& manicures);
    void showManicureDetails(const std::string& chatId, const std::string& manicureId);
    void handleSearchQuery(const std::string& chatId, const std::string& query);
    void confirmDelete(const std::string& chatId, const std::string& manicureId);
    void showRecommendation(const std::string& chatId, const std::string& manicureId);

    // Новый метод для отправки фото
    void sendPhoto(const std::string& chatId, const std::vector<char>& imageData, const std::string& caption);

    void handleUserAction(const std::string& chatId, const UserAction& action);

    std::string generateUniqueId(const std::string& chatId);
    ConnectionPool* getConnection();
    void clearExpiredCache();

public:
    //конструктор и деструктор
    TelegramBot(const std::string& token);
    ~TelegramBot();

    //старт
    void start();
    void handleUpdates(const nlohmann::json& updates);
    void sendMessage(const std::string& chatId, const std::string& text);
    nlohmann::json makeRequest(const std::string& method, const nlohmann::json& payload = {});

    friend class ActionVisitor;
};

#endif // TG_BOT_H