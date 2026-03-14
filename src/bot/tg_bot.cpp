#include "bot/tg_bot.h"
#include <iostream>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <mutex>
#include <thread>
#include <random>
#include <filesystem>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>
#include "bot/tg_bot.h"
#include "photo_repository/maniqure_data_updated.h"

#include <shared_mutex>
#include <algorithm>
#include <nlohmann/json.hpp>
using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;
namespace fs = std::filesystem;

//функция для генерации рекомендации
RecommendationData generateNextManicureRecommendation(const ManicureData& lastManicure) {
    RecommendationData recommendation;
    //девчоки вставьте сюда код для поиска нужного маникюра с id от 1 до 30 и его описание по описанию присланного
    //вот пример для std::string id = "1";
    //все почти готово, доделайте свою часть!!!!!!!!!!
    std::string photoPath = "data/photos_nails++/1.jpg";

    std::ifstream file(photoPath, std::ios::binary);

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    recommendation.imageData.resize(size);
    file.read(recommendation.imageData.data(), size);
    file.close();

    recommendation.imageFormat = "jpg";
    recommendation.description = "✨ Вот пример маникюра:";
    std::cout << "✅ Фото загружено: " << photoPath << " (" << size << " байт)" << std::endl;
    return recommendation;
}

//ConnectionPool
struct TelegramBot::ConnectionPool {
    std::unique_ptr<boost::asio::io_context> ioc;
    std::unique_ptr<boost::asio::ssl::context> ssl_ctx;
    std::chrono::steady_clock::time_point lastUsed;

    ConnectionPool() : ioc(std::make_unique<boost::asio::io_context>()),
                       ssl_ctx(std::make_unique<boost::asio::ssl::context>(boost::asio::ssl::context::tlsv12_client)) {
        ssl_ctx->set_default_verify_paths();
    }
};

//Logger
class Logger {
private:
    std::unique_ptr<std::ofstream> logFile;

public:
    explicit Logger(const std::string& filename) {
        try {
            logFile = std::make_unique<std::ofstream>(filename, std::ios::app);
            if (!logFile->is_open()) {
                throw std::runtime_error("Cannot open log file: " + filename);
            }
        } catch (const std::exception& e) {
            std::cerr << "Logger initialization failed: " << e.what() << std::endl;
            throw;
        }
    }

    ~Logger() {
        if (logFile && logFile->is_open()) {
            *logFile << "=== Logger shutting down ===" << std::endl;
            logFile->close();
        }
    }

    void log(const std::string& message) {
        if (logFile && logFile->is_open()) {
            auto now = std::chrono::system_clock::now();
            auto now_time_t = std::chrono::system_clock::to_time_t(now);

            *logFile << "[" << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S") << "] "
                     << message << std::endl;
        }
    }
};

//конструктор ManicureRepository
ManicureRepository::ManicureRepository() : storageMutex(std::make_shared<std::mutex>()) {}

// добавяем в репозиторий
void ManicureRepository::add(std::string id, std::shared_ptr<ManicureData> data) {
    std::lock_guard<std::mutex> lock(*storageMutex);
    if (!data) {
        throw std::invalid_argument("Cannot add null data to repository");
    }
    storage[std::move(id)] = std::move(data);
}

// берем из репозитория
std::optional<std::shared_ptr<ManicureData>> ManicureRepository::get(const std::string& id) const {
    std::lock_guard<std::mutex> lock(*storageMutex);
    auto it = storage.find(id);
    if (it == storage.end()) {
        return std::nullopt;
    }
    return it->second;
}

// берем последний по времени из репозитория
std::optional<std::shared_ptr<ManicureData>> ManicureRepository::getLastUserManicure(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(*storageMutex);
    std::shared_ptr<ManicureData> latest = nullptr;
    for (const auto& [id, data] : storage) {
        if (data->ownerId && *data->ownerId == userId) {
            if (!latest || data->createdAt > latest->createdAt) {
                latest = data;
            }
        }
    }
    if (!latest) {
        return std::nullopt;
    }
    return latest;
}

// берем все из репозитория
std::vector<std::shared_ptr<ManicureData>> ManicureRepository::getUserManicures(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(*storageMutex);
    std::vector<std::shared_ptr<ManicureData>> result;
    result.reserve(storage.size());
    for (const auto& [id, data] : storage) {
        if (data->ownerId && *data->ownerId == userId) {
            result.push_back(data);
        }
    }
    return result;
}

// ищем в репозитории
std::vector<std::shared_ptr<ManicureData>> ManicureRepository::search(const std::string& text) const {
    std::lock_guard<std::mutex> lock(*storageMutex);
    std::vector<std::shared_ptr<ManicureData>> result;
    for (const auto& [id, data] : storage) {
        if (data->description.find(text) != std::string::npos) {
            result.push_back(data);
        }
    }
    return result;
}

//удаляем из репозитория
std::optional<bool> ManicureRepository::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(*storageMutex);
    auto it = storage.find(id);
    if (it == storage.end()) {
        return std::nullopt;
    }

    //удаляем файл с диска
    try {
        if (it->second->filePath != fs::path() && fs::exists(it->second->filePath)) {
            fs::remove(it->second->filePath);
        }
    } catch (const std::exception& e) {
        // Логируем ошибку, но продолжаем удаление из хранилища
        std::cerr << "Failed to delete file: " << e.what() << std::endl;
    }

    storage.erase(it);
    return true;
}

//размер репозитория
size_t ManicureRepository::size() const noexcept {
    std::lock_guard<std::mutex> lock(*storageMutex);
    return storage.size();
}

//очищаем репозиторий
void ManicureRepository::clear() noexcept {
    std::lock_guard<std::mutex> lock(*storageMutex);
    //удаляем все файлы
    for (const auto& [id, data] : storage) {
        try {
            if (data->filePath != fs::path() && fs::exists(data->filePath)) {
                fs::remove(data->filePath);
            }
        } catch (const std::exception& e) {
            std::cerr << "Failed to delete file: " << e.what() << std::endl;
        }
    }
    storage.clear();
}

//загружаем в файл
void ManicureRepository::saveToFile(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(*storageMutex);
    std::ofstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for writing: " + filename);
    }
    for (const auto& [id, data] : storage) {
        nlohmann::json j;
        j["id"] = data->id;
        j["description"] = data->description;
        j["filePath"] = data->filePath.string();
        j["repoType"] = data->repoType;
        if (data->ownerId) {
            j["ownerId"] = *data->ownerId;
        }
        auto time_t = std::chrono::system_clock::to_time_t(data->createdAt);
        j["createdAt"] = time_t;
        file << j.dump() << std::endl;
    }
}

//выгружаем из файла
void ManicureRepository::loadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(*storageMutex);
    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file for reading: " + filename);
    }
    storage.clear();
    std::string line;
    while (std::getline(file, line)) {
        try {
            auto j = nlohmann::json::parse(line);
            auto data = std::make_shared<ManicureData>();
            data->id = j["id"].get<std::string>();
            data->description = j["description"].get<std::string>();
            data->filePath = j["filePath"].get<std::string>();
            data->repoType = j["repoType"].get<std::string>();
            if (j.contains("ownerId") && !j["ownerId"].is_null()) {
                data->ownerId = j["ownerId"].get<std::string>();
            }
            if (j.contains("createdAt")) {
                std::time_t time_t = j["createdAt"].get<std::time_t>();
                data->createdAt = std::chrono::system_clock::from_time_t(time_t);
            } else {
                data->createdAt = std::chrono::system_clock::now();
            }
            storage[data->id] = std::move(data);
        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("Error parsing JSON: " + std::string(e.what()));
        }
    }
}

//ActionVisitor
void ActionVisitor::operator()(const ActionShowAll&) const {
    bot->showManicureList(chatId, bot->repository->getUserManicures(chatId));
}

//показать последний
void ActionVisitor::operator()(const ActionShowLast&) const {
    auto last = bot->repository->getLastUserManicure(chatId);
    if (last) {
        bot->showManicureDetails(chatId, (*last)->id);
    } else {
        bot->sendMessage(chatId, "😕 У вас пока нет сохраненных маникюров.");
    }
}

//поиск
void ActionVisitor::operator()(const ActionSearch&) const {
    bot->sendMessage(chatId, "🔍 Введите текст для поиска в описаниях маникюров:");
    bot->userStates[chatId] = "searching";
}

//удалить
void ActionVisitor::operator()(const ActionDelete& action) const {
    bot->confirmDelete(chatId, action.manicureId);
}

//рекомендация
void ActionVisitor::operator()(const ActionRecommend& action) const {
    bot->showRecommendation(chatId, action.manicureId);
}

//назад
void ActionVisitor::operator()(const ActionBack&) const {
    bot->showMainMenu(chatId);
}

// конструктор бота
TelegramBot::TelegramBot(const std::string& token)
        : botToken(token),
          apiUrl("https://api.telegram.org/bot" + token),
          repository(std::make_unique<ManicureRepository>()),
          logger(std::make_shared<Logger>("bot.log")) {

    try {
        //создаем директорию для фотографий
        fs::create_directories("photos");
        repository->loadFromFile("manicures.dat");
        logger->log("Loaded " + std::to_string(repository->size()) + " manicures from file");
    } catch (const std::exception& e) {
        logger->log(std::string("Failed to load from file: ") + e.what());
    }

    for (size_t i = 0; i < POOL_SIZE; ++i) {
        connectionPool.push_back(std::make_unique<ConnectionPool>());
    }
}

// деструктор бота
TelegramBot::~TelegramBot() {
    try {
        repository->saveToFile("manicures.dat");
        logger->log("Repository saved to file on shutdown");
    } catch (const std::exception& e) {
        logger->log("Failed to save repository on shutdown: " + std::string(e.what()));
    }
}

std::string TelegramBot::savePhotoToDisk(const std::vector<char>& photoData, const std::string& chatId) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << "photos/" << chatId << "_" << now_time_t << "_" << (rand() % 1000) << ".jpg";
    std::string filePath = ss.str();

    std::ofstream file(filePath, std::ios::binary);
    if (file.is_open()) {
        file.write(photoData.data(), photoData.size());
        file.close();
        logger->log("Photo saved to disk: " + filePath);
        return filePath;
    }

    throw std::runtime_error("Failed to save photo to disk");
}

std::vector<char> TelegramBot::getFileData(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filePath);
    }

    file.seekg(0, std::ios::end);
    size_t size = file.tellg();
    file.seekg(0, std::ios::beg);

    std::vector<char> data(size);
    file.read(data.data(), size);
    file.close();

    return data;
}

std::string TelegramBot::downloadPhoto(const std::string& fileId) {
    try {
        nlohmann::json payload;
        payload["file_id"] = fileId;

        auto response = makeRequest("getFile", payload);

        if (!response.contains("result") || !response["result"].contains("file_path")) {
            throw std::runtime_error("Failed to get file path");
        }

        std::string filePath = response["result"]["file_path"].get<std::string>();
        std::string downloadUrl = "https://api.telegram.org/file/bot" + botToken + "/" + filePath;

        auto* conn = getConnection();

        tcp::resolver resolver(*conn->ioc);
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream(*conn->ioc, *conn->ssl_ctx);

        auto const results = resolver.resolve("api.telegram.org", "443");
        boost::beast::get_lowest_layer(stream).connect(results);
        stream.handshake(boost::asio::ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::get, "/file/bot" + botToken + "/" + filePath, 11};
        req.set(http::field::host, "api.telegram.org");

        http::write(stream, req);

        boost::beast::flat_buffer buffer;
        http::response<http::vector_body<char>> res;
        http::read(stream, buffer, res);

        if (res.result() != http::status::ok) {
            throw std::runtime_error("Failed to download file");
        }

        return savePhotoToDisk(res.body(), "temp");

    } catch (const std::exception& e) {
        logger->log("Failed to download photo: " + std::string(e.what()));
        throw;
    }
}

//обрабатываем полученное фото
void TelegramBot::handlePhoto(const std::string& chatId, const std::string& fileId, const std::string& fileUniqueId) {
    try {
        std::string localPath = downloadPhoto(fileId);
        std::vector<char> photoData = getFileData(localPath);

        if (userStates[chatId] == "waiting_for_photo") {
            tempPhotos[chatId] = photoData;
            userStates[chatId] = "waiting_for_description";
            sendMessage(chatId, "✅ Фото получено! Теперь отправьте описание маникюра:");
        } else {
            sendMessage(chatId, "❓ Чтобы добавить маникюр, используйте /new");
        }
    } catch (const std::exception& e) {
        logger->log("Error handling photo: " + std::string(e.what()));
        sendMessage(chatId, "❌ Не удалось обработать фото. Попробуйте еще раз.");
    }
}

//генерируем уникальный id
std::string TelegramBot::generateUniqueId(const std::string& chatId) {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    auto now_us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()).count();
    int counter = ++userManicureCounters[chatId];
    std::stringstream ss;
    ss << chatId << "_" << now_time_t << "_" << counter << "_" << (now_us % 10000);
    return ss.str();
}

TelegramBot::ConnectionPool* TelegramBot::getConnection() {
    currentConnection = (currentConnection + 1) % POOL_SIZE;
    auto* conn = connectionPool[currentConnection].get();
    conn->lastUsed = std::chrono::steady_clock::now();
    return conn;
}

//очищаем кеш
void TelegramBot::clearExpiredCache() {
    auto now = std::chrono::steady_clock::now();
    for (auto it = responseCache.begin(); it != responseCache.end();) {
        if (now - it->second.timestamp > CACHE_TTL) {
            it = responseCache.erase(it);
        } else {
            ++it;
        }
    }
}

//старт
void TelegramBot::start() {
    constexpr int POLLING_INTERVAL_MS = 300;
    constexpr int CACHE_CLEANUP_INTERVAL = 100;
    int iteration = 0;

    while (true) {
        try {
            nlohmann::json payload;
            if (!lastUpdateId.empty()) {
                payload["offset"] = std::stoi(lastUpdateId) + 1;
            }
            payload["timeout"] = 30;

            auto response = makeRequest("getUpdates", payload);

            if (response.contains("result") && !response["result"].empty()) {
                handleUpdates(response["result"]);
            }

            if (++iteration % CACHE_CLEANUP_INTERVAL == 0) {
                clearExpiredCache();
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(POLLING_INTERVAL_MS));

        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
            logger->log(std::string("Error in start: ") + e.what());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}

//алгоритм запросов в бот
void TelegramBot::handleUpdates(const nlohmann::json& updates) {
    for (const auto& update : updates) {
        if (update.contains("update_id")) {
            lastUpdateId = std::to_string(update["update_id"].get<int>());
        }

        if (update.contains("callback_query")) {
            handleCallbackQuery(update["callback_query"]);
            continue;
        }

        if (update.contains("message")) {
            std::string chatId = std::to_string(update["message"]["chat"]["id"].get<int>());

            if (update["message"].contains("photo")) {
                const auto& photos = update["message"]["photo"];
                std::string fileId = photos.back()["file_id"].get<std::string>();
                std::string fileUniqueId = photos.back()["file_unique_id"].get<std::string>();

                handlePhoto(chatId, fileId, fileUniqueId);
                continue;
            }

            if (update["message"].contains("text")) {
                std::string text = update["message"]["text"].get<std::string>();

                if (text == "/cancel") {
                    userStates.erase(chatId);
                    searchQueries.erase(chatId);
                    tempPhotos.erase(chatId);
                    sendMessage(chatId, "❌ Действие отменено.");
                    showMainMenu(chatId);
                    continue;
                }

                if (userStates[chatId] == "searching") {
                    handleSearchQuery(chatId, text);
                    userStates.erase(chatId);
                }
                else if (userStates[chatId] == "waiting_for_description") {
                    if (tempPhotos.find(chatId) != tempPhotos.end()) {
                        std::vector<char> photoData = tempPhotos[chatId];
                        std::string description = text;
                        saveManicureData(chatId, photoData, description);
                        tempPhotos.erase(chatId);
                        userStates.erase(chatId);
                    } else {
                        sendMessage(chatId, "❌ Ошибка: фото не найдено. Начните заново с /new");
                        userStates.erase(chatId);
                    }
                }
                else if (text == "/start" || text == "/menu") {
                    showMainMenu(chatId);
                }
                else if (text == "/new") {
                    handleManicureRequest(chatId);
                }
                else if (text == "/help") {
                    std::string helpText =
                            "📖 Доступные команды:\n"
                            "/start - Показать главное меню\n"
                            "/menu - Главное меню\n"
                            "/new - Добавить новый маникюр (отправьте фото)\n"
                            "/cancel - Отменить текущее действие\n"
                            "/help - Это сообщение";
                    sendMessage(chatId, helpText);
                }
                else {
                    sendMessage(chatId, "❓ Неизвестная команда. Используйте /menu для навигации.");
                }
            }
        }
    }
}

//метод для отправки фото
void TelegramBot::sendPhoto(const std::string& chatId, const std::vector<char>& imageData, const std::string& caption) {
    try {
        std::string boundary = "----WebKitFormBoundary" + std::to_string(std::time(nullptr));

        std::string body;
        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"chat_id\"\r\n\r\n";
        body += chatId + "\r\n";

        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"caption\"\r\n\r\n";
        body += caption + "\r\n";

        body += "--" + boundary + "\r\n";
        body += "Content-Disposition: form-data; name=\"photo\"; filename=\"recommendation.png\"\r\n";
        body += "Content-Type: image/png\r\n\r\n";

        std::vector<char> requestBody;
        requestBody.insert(requestBody.end(), body.begin(), body.end());
        requestBody.insert(requestBody.end(), imageData.begin(), imageData.end());

        std::string footer = "\r\n--" + boundary + "--\r\n";
        requestBody.insert(requestBody.end(), footer.begin(), footer.end());

        auto* conn = getConnection();

        tcp::resolver resolver(*conn->ioc);
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream(*conn->ioc, *conn->ssl_ctx);

        auto const results = resolver.resolve("api.telegram.org", "443");
        boost::beast::get_lowest_layer(stream).connect(results);
        stream.handshake(boost::asio::ssl::stream_base::client);

        http::request<http::vector_body<char>> req{http::verb::post, "/bot" + botToken + "/sendPhoto", 11};
        req.set(http::field::host, "api.telegram.org");
        req.set(http::field::content_type, "multipart/form-data; boundary=" + boundary);
        req.set(http::field::content_length, std::to_string(requestBody.size()));
        req.body() = std::move(requestBody);
        req.prepare_payload();

        http::write(stream, req);

        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        boost::system::error_code ec;
        stream.shutdown(ec);

        if (ec && ec != boost::asio::ssl::error::stream_truncated) {
            throw boost::system::system_error(ec);
        }

        logger->log("Photo sent to " + chatId + ", size: " + std::to_string(imageData.size()) + " bytes");

    } catch (const std::exception& e) {
        logger->log("Failed to send photo: " + std::string(e.what()));
        sendMessage(chatId, caption + "\n\n(Не удалось отправить изображение)");
    }
}

//отправляем сохраненное фото
void TelegramBot::sendStoredPhoto(const std::string& chatId, const std::string& filePath, const std::string& caption) {
    try {
        std::vector<char> photoData = getFileData(filePath);
        sendPhoto(chatId, photoData, caption);
    } catch (const std::exception& e) {
        logger->log("Failed to send stored photo: " + std::string(e.what()));
        sendMessage(chatId, caption + "\n\n(Фото недоступно)");
    }
}

//алгоритм действий бота
void TelegramBot::handleCallbackQuery(const nlohmann::json& callbackQuery) {
    std::string chatId = std::to_string(callbackQuery["message"]["chat"]["id"].get<int>());
    std::string data = callbackQuery["data"].get<std::string>();

    logger->log("Callback query from " + chatId + ": " + data);

    nlohmann::json answerPayload;
    answerPayload["callback_query_id"] = callbackQuery["id"].get<std::string>();
    makeRequest("answerCallbackQuery", answerPayload);

    if (data == "show_all") {
        handleUserAction(chatId, ActionShowAll{});
    }
    else if (data == "show_last") {
        handleUserAction(chatId, ActionShowLast{});
    }
    else if (data == "search") {
        handleUserAction(chatId, ActionSearch{});
    }
    else if (data == "back") {
        handleUserAction(chatId, ActionBack{});
    }
    else if (data == "help") {
        std::string helpText =
                "📖 *Помощь*\n\n"
                "Бот для хранения маникюров позволяет:\n"
                "• Сохранять фото и описания маникюров\n"
                "• Просматривать все свои записи\n"
                "• Искать по описанию\n"
                "• Удалять записи\n"
                "• Получать рекомендации с PNG изображениями\n\n"
                "Используйте меню для навигации!";
        sendMessage(chatId, helpText);
    }
    else if (data == "new_manicure") {
        handleManicureRequest(chatId);
    }
    else if (data.substr(0, 5) == "view_") {
        std::string manicureId = data.substr(5);
        showManicureDetails(chatId, manicureId);
    }
    else if (data.substr(0, 7) == "delete_") {
        std::string manicureId = data.substr(7);
        confirmDelete(chatId, manicureId);
    }
    else if (data.substr(0, 15) == "confirm_delete_") {
        std::string manicureId = data.substr(15);
        auto result = repository->remove(manicureId);
        if (result && *result) {
            sendMessage(chatId, "✅ Маникюр успешно удален!");
            logger->log("User " + chatId + " deleted manicure: " + manicureId);
        } else {
            sendMessage(chatId, "❌ Не удалось удалить маникюр.");
        }
        showMainMenu(chatId);
    }
    else if (data.substr(0, 10) == "recommend_") {
        std::string manicureId = data.substr(10);
        handleUserAction(chatId, ActionRecommend{manicureId});
    }
    else if (data.substr(0, 5) == "page_") {
        auto manicures = repository->getUserManicures(chatId);
        showManicureList(chatId, manicures);
    }
    else if (data == "back_to_list") {
        auto manicures = repository->getUserManicures(chatId);
        showManicureList(chatId, manicures);
    }
    else if (data == "cancel") {
        userStates.erase(chatId);
        searchQueries.erase(chatId);
        tempPhotos.erase(chatId);
        sendMessage(chatId, "❌ Действие отменено.");
        showMainMenu(chatId);
    }
    else {
        sendMessage(chatId, "❓ Неизвестная команда.");
    }
}

//ждем фото
void TelegramBot::handleManicureRequest(const std::string& chatId) {
    std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"❌ Отмена", "cancel"}}
    };

    auto keyboard = createInlineKeyboard(buttons);
    userStates[chatId] = "waiting_for_photo";
    sendMessageWithKeyboard(chatId, "📸 Отправьте фотографию маникюра:", keyboard);
}

//сохраняем маникюр
void TelegramBot::saveManicureData(const std::string& chatId, const std::vector<char>& photoData, const std::string& description) {
    auto now = std::chrono::system_clock::now();
    std::string dataId = generateUniqueId(chatId);

    std::string filePath = savePhotoToDisk(photoData, chatId);

    auto data = std::make_shared<ManicureData>();
    data->id = dataId;
    data->description = description;
    data->filePath = fs::path(filePath);
    data->createdAt = now;
    data->repoType = "memory";
    data->ownerId = chatId;

    repository->add(dataId, std::move(data));
    logger->log("User " + chatId + " saved manicure: " + dataId);

    std::thread([this]() {
        try {
            repository->saveToFile("manicures.dat");
            logger->log("Repository saved to file");
        } catch (const std::exception& e) {
            logger->log("Failed to save repository: " + std::string(e.what()));
        }
    }).detach();

    showManicureDetails(chatId, dataId);
}

//показываем маникюр
void TelegramBot::showManicureData(const std::string& chatId, const std::string& dataId) {
    showManicureDetails(chatId, dataId);
}

//меню
void TelegramBot::showMainMenu(const std::string& chatId) {
    std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"📋 Все мои маникюры", "show_all"}, {"⭐ Последний", "show_last"}},
            {{"🔍 Поиск", "search"}, {"❓ Помощь", "help"}},
            {{"➕ Новый маникюр", "new_manicure"}}
    };

    auto keyboard = createInlineKeyboard(buttons);
    sendMessageWithKeyboard(chatId,
                            "🗂 *Главное меню*\n\n"
                            "Выберите действие:", keyboard);
}

//список маникюров
void TelegramBot::showManicureList(const std::string& chatId,
                                   const std::vector<std::shared_ptr<ManicureData>>& manicures) {
    if (manicures.empty()) {
        sendMessage(chatId, "😕 У вас пока нет сохраненных маникюров.");
        showMainMenu(chatId);
        return;
    }

    constexpr int ITEMS_PER_PAGE = 5;
    int totalPages = (manicures.size() + ITEMS_PER_PAGE - 1) / ITEMS_PER_PAGE;
    int currentPage = 0;

    std::string message = "📚 *Ваши маникюры (всего: " + std::to_string(manicures.size()) + ")*\n\n";

    size_t start = currentPage * ITEMS_PER_PAGE;
    size_t end = std::min(start + ITEMS_PER_PAGE, manicures.size());

    for (size_t i = start; i < end; ++i) {
        const auto& m = manicures[i];
        auto time_t = std::chrono::system_clock::to_time_t(m->createdAt);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%d.%m.%Y %H:%M");

        std::string shortId = m->id.length() > 8 ? m->id.substr(m->id.length() - 8) : m->id;

        message += "🔹 *" + shortId + "*\n";
        message += "   📅 " + ss.str() + "\n";
        message += "   📝 " + m->description.substr(0, 30) +
                   (m->description.length() > 30 ? "..." : "") + "\n\n";
    }

    std::vector<std::vector<std::pair<std::string, std::string>>> buttons;

    for (size_t i = start; i < end; ++i) {
        const auto& m = manicures[i];
        std::string callbackData = "view_" + m->id;
        std::string shortId = m->id.length() > 8 ? m->id.substr(m->id.length() - 8) : m->id;
        buttons.push_back({{"🔍 " + shortId, callbackData}});
    }

    buttons.push_back({});
    if (currentPage > 0) {
        buttons.back().push_back({"◀️ Пред.", "page_" + std::to_string(currentPage - 1)});
    }
    buttons.back().push_back({"🏠 Главное меню", "back"});
    if (currentPage < totalPages - 1) {
        buttons.back().push_back({"▶️ След.", "page_" + std::to_string(currentPage + 1)});
    }

    auto keyboard = createInlineKeyboard(buttons);
    sendMessageWithKeyboard(chatId, message, keyboard);
}

//показываем информацию о маникюре
void TelegramBot::showManicureDetails(const std::string& chatId, const std::string& manicureId) {
    logger->log("Showing details for manicure ID: " + manicureId);

    auto dataOpt = repository->get(manicureId);
    if (!dataOpt) {
        logger->log("ERROR: Manicure not found with ID: " + manicureId);
        sendMessage(chatId, "❌ Маникюр не найден.");
        showMainMenu(chatId);
        return;
    }

    auto data = *dataOpt;

    if (!data->ownerId || *data->ownerId != chatId) {
        logger->log("ERROR: Access denied for user " + chatId + " to manicure " + manicureId);
        sendMessage(chatId, "❌ У вас нет доступа к этому маникюру.");
        showMainMenu(chatId);
        return;
    }

    auto time_t = std::chrono::system_clock::to_time_t(data->createdAt);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%d.%m.%Y %H:%M:%S");

    std::string caption =
            "💅 *Детали маникюра*\n\n"
            "🆔 ID: `" + data->id + "`\n"
                                   "📅 Дата: " + ss.str() + "\n"
                                                           "📝 Описание:\n" + data->description;

    // Отправляем фото с подписью
    sendStoredPhoto(chatId, data->filePath.string(), caption);

    std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"💡 Получить рекомендацию", "recommend_" + manicureId}, {"🗑 Удалить", "delete_" + manicureId}},
            {{"◀️ Назад к списку", "back_to_list"}, {"🏠 Главное меню", "back"}}
    };

    auto keyboard = createInlineKeyboard(buttons);
    sendMessageWithKeyboard(chatId, "👆 *Фото выше*\n\nВыберите действие:", keyboard);
}

//обрабатываем кнопку поиск
void TelegramBot::handleSearchQuery(const std::string& chatId, const std::string& query) {
    auto results = repository->search(query);

    if (results.empty()) {
        sendMessage(chatId, "😕 Ничего не найдено по запросу: \"" + query + "\"");
        showMainMenu(chatId);
        return;
    }

    std::string message = "🔍 *Результаты поиска*\n\n";
    message += "По запросу \"" + query + "\" найдено " +
               std::to_string(results.size()) + " маникюров:\n\n";

    std::vector<std::vector<std::pair<std::string, std::string>>> buttons;
    int count = 0;

    for (const auto& data : results) {
        if (data->ownerId && *data->ownerId == chatId) {
            auto time_t = std::chrono::system_clock::to_time_t(data->createdAt);
            std::stringstream ss;
            ss << std::put_time(std::localtime(&time_t), "%d.%m.%Y");

            std::string shortId = data->id.length() > 8 ? data->id.substr(data->id.length() - 8) : data->id;

            message += "🔹 *" + shortId + "* (" + ss.str() + ")\n";
            message += "   " + data->description.substr(0, 50) +
                       (data->description.length() > 50 ? "..." : "") + "\n\n";

            buttons.push_back({{"🔍 Просмотр " + std::to_string(++count), "view_" + data->id}});
        }
    }

    buttons.push_back({{"🏠 Главное меню", "back"}});

    auto keyboard = createInlineKeyboard(buttons);
    sendMessageWithKeyboard(chatId, message, keyboard);
}

//удаление маникюра
void TelegramBot::confirmDelete(const std::string& chatId, const std::string& manicureId) {
    std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"✅ Да, удалить", "confirm_delete_" + manicureId},
             {"❌ Нет, отмена", "back"}}
    };

    auto keyboard = createInlineKeyboard(buttons);
    sendMessageWithKeyboard(chatId,
                            "⚠️ *Подтверждение удаления*\n\n"
                            "Вы уверены, что хотите удалить этот маникюр?",
                            keyboard);
}

//делаем рекомендацию
void TelegramBot::showRecommendation(const std::string& chatId, const std::string& manicureId) {
    logger->log("Showing recommendation for manicure ID: " + manicureId);

    auto dataOpt = repository->get(manicureId);
    if (!dataOpt) {
        logger->log("ERROR: Manicure not found for recommendation: " + manicureId);
        sendMessage(chatId, "❌ Маникюр не найден.");
        showMainMenu(chatId);
        return;
    }

    auto data = *dataOpt;

    RecommendationData recommendation = generateNextManicureRecommendation(*data);
    sendPhoto(chatId, recommendation.imageData, recommendation.description);
    std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"◀️ Назад к маникюру", "view_" + manicureId}, {"🏠 Главное меню", "back"}}
    };

    auto keyboard = createInlineKeyboard(buttons);
    sendMessageWithKeyboard(chatId, "✨ *Рекомендация отправлена выше*", keyboard);
}

void TelegramBot::handleUserAction(const std::string& chatId, const UserAction& action) {
    ActionVisitor visitor(this, chatId);
    std::visit(visitor, action);
}

//делаем кнопочки
nlohmann::json TelegramBot::createInlineKeyboard(const std::vector<std::vector<std::pair<std::string, std::string>>>& buttons) {
    nlohmann::json keyboard = nlohmann::json::array();
    for (const auto& row : buttons) {
        nlohmann::json keyboardRow = nlohmann::json::array();
        for (const auto& button : row) {
            nlohmann::json btn;
            btn["text"] = button.first;
            btn["callback_data"] = button.second;
            keyboardRow.push_back(btn);
        }
        keyboard.push_back(keyboardRow);
    }

    nlohmann::json replyMarkup;
    replyMarkup["inline_keyboard"] = keyboard;
    return replyMarkup;
}

//отправляем сообщение с клавиатурой
void TelegramBot::sendMessageWithKeyboard(const std::string& chatId, const std::string& text, const nlohmann::json& keyboard) {
    nlohmann::json payload;
    payload["chat_id"] = chatId;
    payload["text"] = text;
    payload["parse_mode"] = "Markdown";
    payload["reply_markup"] = keyboard;
    makeRequest("sendMessage", payload);
}

//отправляем сообщение
void TelegramBot::sendMessage(const std::string& chatId, const std::string& text) {
    nlohmann::json payload;
    payload["chat_id"] = chatId;
    payload["text"] = text;
    payload["parse_mode"] = "Markdown";
    makeRequest("sendMessage", payload);
}

nlohmann::json TelegramBot::makeRequest(const std::string& method, const nlohmann::json& payload) {
    if (method == "getUpdates") {
        std::string cacheKey = method + payload.dump();
        auto it = responseCache.find(cacheKey);
        auto now = std::chrono::steady_clock::now();

        if (it != responseCache.end() && now - it->second.timestamp < CACHE_TTL) {
            return it->second.data;
        }
    }

    try {
        auto* conn = getConnection();

        tcp::resolver resolver(*conn->ioc);
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream(*conn->ioc, *conn->ssl_ctx);

        auto const results = resolver.resolve("api.telegram.org", "443");
        boost::beast::get_lowest_layer(stream).connect(results);
        stream.handshake(boost::asio::ssl::stream_base::client);

        http::request<http::string_body> req{http::verb::post, "/bot" + botToken + "/" + method, 11};
        req.set(http::field::host, "api.telegram.org");
        req.set(http::field::content_type, "application/json");
        req.set(http::field::connection, "keep-alive");

        if (!payload.empty()) {
            req.body() = payload.dump();
        }
        req.prepare_payload();

        http::write(stream, req);

        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);

        boost::system::error_code ec;
        stream.shutdown(ec);

        if (ec && ec != boost::asio::ssl::error::stream_truncated) {
            throw boost::system::system_error(ec);
        }

        auto result = nlohmann::json::parse(res.body());

        if (method == "getUpdates") {
            std::string cacheKey = method + payload.dump();
            responseCache[cacheKey] = {result, std::chrono::steady_clock::now()};
        }

        return result;

    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Request failed: ") + e.what());
    }
}