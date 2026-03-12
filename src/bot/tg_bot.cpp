//#include "tg_bot.h"
#include <iostream>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <nlohmann/json.hpp>

using tcp = boost::asio::ip::tcp;
namespace http = boost::beast::http;

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
            throw; // Пробрасываем исключение дальше
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

// Реализация ManicureRepository

ManicureRepository::ManicureRepository()
        : storageMutex(std::make_shared<std::mutex>()) {
    // Инициализация пустого репозитория
}

void ManicureRepository::add(std::string id, std::shared_ptr<ManicureData> data) {
    std::lock_guard<std::mutex> lock(*storageMutex); // RAII для мьютекса

    if (!data) {
        throw std::invalid_argument("Cannot add null data to repository");
    }

    // Используем move для id
    storage[std::move(id)] = std::move(data);
}

std::optional<std::shared_ptr<ManicureData>> ManicureRepository::get(const std::string& id) const {
    std::lock_guard<std::mutex> lock(*storageMutex);

    auto it = storage.find(id);
    if (it == storage.end()) {
        return std::nullopt; // optional без значения
    }

    return it->second; // optional с shared_ptr
}

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

std::vector<std::shared_ptr<ManicureData>> ManicureRepository::getUserManicures(const std::string& userId) const {
    std::lock_guard<std::mutex> lock(*storageMutex);

    std::vector<std::shared_ptr<ManicureData>> result;
    result.reserve(storage.size()); // Предварительное выделение памяти

    for (const auto& [id, data] : storage) {
        if (data->ownerId && *data->ownerId == userId) {
            result.push_back(data);
        }
    }

    return result;
}

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

std::optional<bool> ManicureRepository::remove(const std::string& id) {
    std::lock_guard<std::mutex> lock(*storageMutex);

    auto it = storage.find(id);
    if (it == storage.end()) {
        return std::nullopt; // Не нашли - возвращаем nullopt
    }

    storage.erase(it);
    return true; // Успешно удалили
}

size_t ManicureRepository::size() const noexcept {
    std::lock_guard<std::mutex> lock(*storageMutex);
    return storage.size();
}

void ManicureRepository::clear() noexcept {
    std::lock_guard<std::mutex> lock(*storageMutex);
    storage.clear();
}

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

        file << j.dump() << std::endl;
    }
}

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

            data->createdAt = std::chrono::system_clock::now(); // В реальности сохраняли бы timestamp

            storage[data->id] = std::move(data);

        } catch (const nlohmann::json::exception& e) {
            throw std::runtime_error("Error parsing JSON: " + std::string(e.what()));
        }
    }
}

// Обновленный конструктор TelegramBot
TelegramBot::TelegramBot(const std::string& token)
        : botToken(token),
          apiUrl("https://api.telegram.org/bot" + token),
          repository(std::make_unique<ManicureRepository>()),
          logger(std::make_shared<Logger>("bot.log")) {

    try {
        // Пытаемся загрузить сохраненные данные
        repository->loadFromFile("manicures.dat");
        logger->log("Loaded " + std::to_string(repository->size()) + " manicures from file");
    } catch (const std::exception& e) {
        logger->log(std::string("Failed to load from file: ") + e.what());
        // Не фатально, продолжаем с пустым репозиторием
    }
}

void TelegramBot::start() {
    while (true) {
        try {
            nlohmann::json payload;
            if (!lastUpdateId.empty()) {
                payload["offset"] = std::stoi(lastUpdateId) + 1;
            }

            auto response = makeRequest("getUpdates", payload);
            handleUpdates(response["result"]);
        } catch (const std::exception& e) {
            std::cerr << "Error: " << e.what() << std::endl;
        }
    }
}

void TelegramBot::handleUpdates(const nlohmann::json& updates) {
    for (const auto& update : updates) {
        if (update.contains("update_id")) {
            lastUpdateId = std::to_string(update["update_id"].get<int>());
        }
        //обработка нажатия на кнопочку
        if (update.contains("callback_query")) {
            handleCallbackQuery(update["callback_query"]);
            continue;
        }
        //обработка описания
        if (update.contains("message") && update["message"].contains("text")) {
            std::string chatId = std::to_string(update["message"]["chat"]["id"].get<int>());
            std::string text = update["message"]["text"].get<std::string>();
            //проверка состояния
            if (userStates[chatId] == "waiting_for_photo") { //прислали ссылку
                tempPhotoUrls[chatId] = text;
                userStates[chatId] = "waiting_for_description";
                sendMessage(chatId, "✅ Ссылка получена! Теперь отправьте описание маникюра:");
            }
            else if (userStates[chatId] == "waiting_for_description") { //прислали описание
                std::string photoUrl = tempPhotoUrls[chatId];
                std::string description = text;
                saveManicureData(chatId, photoUrl, description);
                //очищащем ненужную фигню
                tempPhotoUrls.erase(chatId);
                userStates.erase(chatId);
                showManicureData(chatId, chatId + "_" + std::to_string(std::time(nullptr)));
            }
            else if (text == "/start") {
                std::string welcomeText =
                        " Добро пожаловать в бот для маникюра!\n\n"
                        "Я помогу вам сохранить информацию о маникюре.\n"
                        "Просто отправьте /new чтобы добавить новый маникюр.";
                sendMessage(chatId, welcomeText);
            }
            else if (text == "/new") {
                handleManicureRequest(chatId);
            }
            else if (text == "/help") {
                std::string helpText =
                        " Доступные команды:\n"
                        "/start - Приветствие\n"
                        "/new - Добавить новый маникюр\n"
                        "/help - Это сообщение";
                sendMessage(chatId, helpText);
            }
            else {
                sendMessage(chatId, "❓ Неизвестная команда. Используйте /help для списка команд.");
            }
        }
    }
}

void TelegramBot::handleCallbackQuery(const nlohmann::json& callbackQuery) {
    std::string chatId = std::to_string(callbackQuery["message"]["chat"]["id"].get<int>());
    std::string data = callbackQuery["data"].get<std::string>();
    //ответ на callback
    nlohmann::json answerPayload;
    answerPayload["callback_query_id"] = callbackQuery["id"].get<std::string>();
    makeRequest("answerCallbackQuery", answerPayload);

    if (data == "new_manicure") {
        handleManicureRequest(chatId);
    }
    else if (data == "cancel") {
        userStates.erase(chatId);
        tempPhotoUrls.erase(chatId);
        sendMessage(chatId, "❌ Действие отменено. Используйте /new чтобы начать заново.");
    }
}

void TelegramBot::handleManicureRequest(const std::string& chatId) {
    // Создаем кнопку отмены
    std::vector<std::vector<std::pair<std::string, std::string>>> buttons = {
            {{"❌ Отмена", "cancel"}}
    };

    auto keyboard = createInlineKeyboard(buttons);
    userStates[chatId] = "waiting_for_photo";
    sendMessageWithKeyboard(chatId, "📸 Отправьте ссылку на фотографию маникюра:", keyboard);
}

void TelegramBot::saveManicureData(const std::string& chatId, const std::string& photoUrl, const std::string& description) {
    // Получаем текущее время
    auto now = std::chrono::system_clock::now();
    std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y-%m-%d %H:%M:%S");

    // Создаем уникальный ID для записи
    std::string dataId = chatId + "_" + std::to_string(now_time_t);

    // Сохраняем данные
    ManicureData data;
    data.id = dataId;  // PhotoId
    data.description = description;
    data.filePath = std::filesystem::path(photoUrl);  // filePath вместо photoUrl
    data.createdAt = now;  // createdAt вместо timestamp
    data.repoType = "memory";  // repoType
    data.ownerId = chatId;  // ownerId

    manicureStorage[dataId] = data;

    std::cout << " Сохранен маникюр: " << dataId << std::endl;
    std::cout << " Фото: " << photoUrl << std::endl;
    std::cout << " Описание: " << description << std::endl;
}

void TelegramBot::showManicureData(const std::string& chatId, const std::string& dataId) {
    if (manicureStorage.find(dataId) != manicureStorage.end()) {
        ManicureData& data = manicureStorage[dataId];
        // Преобразуем время для отображения
        std::time_t createdAt_time_t = std::chrono::system_clock::to_time_t(data.createdAt);
        std::stringstream ss;
        ss << std::put_time(std::localtime(&createdAt_time_t), "%Y-%m-%d %H:%M:%S");

        std::string message =
                " Маникюр успешно сохранен!\n\n"
                " Ссылка на фото:\n" + data.filePath.string() + "\n\n"
                " Описание:\n" + data.description + "\n\n"
                " Время сохранения:\n" + ss.str() + "\n\n"
                "Используйте /new чтобы добавить еще один маникюр.";
        sendMessage(chatId, message);
        //сюда допишем отправку нового маникюра
    } else {
        sendMessage(chatId, " Ошибка: данные не найдены.");
    }
}

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

void TelegramBot::sendMessageWithKeyboard(const std::string& chatId, const std::string& text, const nlohmann::json& keyboard) {
    nlohmann::json payload;
    payload["chat_id"] = chatId;
    payload["text"] = text;
    payload["reply_markup"] = keyboard;
    makeRequest("sendMessage", payload);
}

void TelegramBot::sendMessage(const std::string& chatId, const std::string& text) {
    nlohmann::json payload;
    payload["chat_id"] = chatId;
    payload["text"] = text;
    makeRequest("sendMessage", payload);
}

nlohmann::json TelegramBot::makeRequest(const std::string& method, const nlohmann::json& payload) {
    try {
        boost::asio::io_context ioc;
        boost::asio::ssl::context ssl_ctx(boost::asio::ssl::context::tlsv12_client);  //включаем ssl context???
        ssl_ctx.set_default_verify_paths();
        tcp::resolver resolver(ioc);
        boost::beast::ssl_stream<boost::beast::tcp_stream> stream(ioc, ssl_ctx);
        //resolve host and connect
        auto const results = resolver.resolve("api.telegram.org", "443");
        boost::beast::get_lowest_layer(stream).connect(results);
        stream.handshake(boost::asio::ssl::stream_base::client); //ssl handshake
        http::request<http::string_body> req{http::verb::post, "/bot" + botToken + "/" + method, 11};
        req.set(http::field::host, "api.telegram.org");
        req.set(http::field::content_type, "application/json");
        req.body() = payload.dump();
        req.prepare_payload();
        //пишем
        http::write(stream, req);
        //получаем ответ
        boost::beast::flat_buffer buffer;
        http::response<http::string_body> res;
        http::read(stream, buffer, res);
        //вырубаем ssl stream
        boost::system::error_code ec;
        stream.shutdown(ec);

        if (ec == boost::asio::ssl::error::stream_truncated) {
            ec.assign(0, ec.category());
        } else if (ec) {
            throw boost::system::system_error(ec);
        }

        return nlohmann::json::parse(res.body());
    } catch (const std::exception& e) {
        throw std::runtime_error(std::string("Request failed: ") + e.what());
    }
}