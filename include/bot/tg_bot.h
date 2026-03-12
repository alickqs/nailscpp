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
#include <nlohmann/json.hpp>

struct ManicureData {
    std::string id; // PhotoId
    std::string description;
    std::filesystem::path filePath;
    std::chrono::system_clock::time_point createdAt;
    std::string repoType;
    std::optional<std::string> ownerId;
};

// Репозиторий для работы с маникюрами
class ManicureRepository {
private:
    // Используем shared_ptr для автоматического управления памятью
    std::map<std::string, std::shared_ptr<ManicureData>> storage;

    std::shared_ptr<std::mutex> storageMutex;

public:
    ManicureRepository();

    ~ManicureRepository() = default;

    // Запрещаем копирование
    ManicureRepository(const ManicureRepository&) = delete;
    ManicureRepository& operator=(const ManicureRepository&) = delete;

    // Разрешаем перемещение
    ManicureRepository(ManicureRepository&&) = default;
    ManicureRepository& operator=(ManicureRepository&&) = default;

    // Добавление маникюра с перемещением данных
    void add(std::string id, std::shared_ptr<ManicureData> data);

    // Получение маникюра по ID (optional - может не найтись)
    std::optional<std::shared_ptr<ManicureData>> get(const std::string& id) const;

    // Получение последнего маникюра пользователя (optional)
    std::optional<std::shared_ptr<ManicureData>> getLastUserManicure(const std::string& userId) const;

    // Получение всех маникюров пользователя
    std::vector<std::shared_ptr<ManicureData>> getUserManicures(const std::string& userId) const;

    // Поиск маникюров по тексту в описании
    std::vector<std::shared_ptr<ManicureData>> search(const std::string& text) const;

    // Удаление маникюра
    std::optional<bool> remove(const std::string& id);

    // Получение количества маникюров
    [[nodiscard]] size_t size() const noexcept;

    // Очистка всех данных
    void clear() noexcept;

    // Сохранение в файл
    void saveToFile(const std::string& filename) const;

    // Загрузка из файла с обработкой исключений
    void loadFromFile(const std::string& filename);
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
    // Используем unique_ptr для репозитория (единоличное владение)
    std::unique_ptr<ManicureRepository> repository;

    // Logger с shared_ptr (может использоваться несколькими компонентами)
    std::shared_ptr<class Logger> logger;

public:
    TelegramBot(const std::string& token);
    void start();
    void handleUpdates(const nlohmann::json& updates);
    void sendMessage(const std::string& chatId, const std::string& text);
    nlohmann::json makeRequest(const std::string& method, const nlohmann::json& payload = {});
    // методы для работы с репозиторием
    void listUserManicures(const std::string& chatId);
    void searchManicures(const std::string& chatId, const std::string& query);
    void deleteManicure(const std::string& chatId, const std::string& manicureId);
};

#endif // TG_BOT_H