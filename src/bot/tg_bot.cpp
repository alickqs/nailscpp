#include "../include/bot/tg_bot.h"
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

TelegramBot::TelegramBot(const std::string& token): botToken(token), apiUrl("https://api.telegram.org/bot" + token) {}

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
    std::time_t now = std::time(nullptr);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S");
    // Создаем уникальный ID для записи
    std::string dataId = chatId + "_" + std::to_string(now);
    // Сохраняем данные
    ManicureData data;
    data.photoUrl = photoUrl;
    data.description = description;
    data.userId = chatId;
    data.timestamp = ss.str();

    manicureStorage[dataId] = data;

    std::cout << " Сохранен маникюр: " << dataId << std::endl;
    std::cout << " Фото: " << photoUrl << std::endl;
    std::cout << " Описание: " << description << std::endl;
}

void TelegramBot::showManicureData(const std::string& chatId, const std::string& dataId) {
    if (manicureStorage.find(dataId) != manicureStorage.end()) {
        ManicureData& data = manicureStorage[dataId];
        std::string message =
                " Маникюр успешно сохранен!\n\n"
                " Ссылка на фото:\n" + data.photoUrl + "\n\n"
                " Описание:\n" + data.description + "\n\n"
                " Время сохранения:\n" + data.timestamp + "\n\n"
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