#include "bot/tg_bot.h"
#include <iostream>
#include <csignal>
#include <fstream>
#include <atomic>
#include <csignal>
#include <cstdlib>

void signalHandler(int) {
    std::cout << "\n Остановка бота..." << std::endl;
    std::exit(0);
}

int main() {
    //установка обработчика сигнала для graceful shutdown
    std::signal(SIGINT, signalHandler);
    std::signal(SIGTERM, signalHandler);

    std::string token = "8787953583:AAHKAbvyVVRJIFIn_-rvIGL5xc6TMhs0crY";
    try {
        TelegramBot bot(token);

        std::cout << "=====================================" << std::endl;
        std::cout << " Бот для маникюра запущен!" << std::endl;
        std::cout << " Команды: /start, /new, /help" << std::endl;
        std::cout << "=====================================" << std::endl;
        std::cout << "Press Ctrl+C to stop" << std::endl;

        bot.start();
    } catch (const std::exception& e) {
        std::cerr << "❌ Ошибка: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
