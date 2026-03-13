#include "bot/tg_bot.h"

#include <chrono>
#include <iostream>
#include <memory>

int main() {
    ManicureRepository repo;

    const int N = 5000;

    // ---------- ADD benchmark ----------
    auto startAdd = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < N; ++i) {
        auto data = std::make_shared<ManicureData>();
        data->id = std::to_string(i);
        data->description = "red glossy manicure with design";
        data->repoType = "memory";
        data->ownerId = "user1";
        data->createdAt = std::chrono::system_clock::now();

        repo.add(data->id, data);
    }

    auto endAdd = std::chrono::high_resolution_clock::now();

    // ---------- SEARCH benchmark ----------
    auto startSearch = std::chrono::high_resolution_clock::now();

    auto results = repo.search("red");

    auto endSearch = std::chrono::high_resolution_clock::now();

    // ---------- RECOMMENDATION benchmark ----------
    auto sample = results.front();

    auto startRec = std::chrono::high_resolution_clock::now();

    auto rec = generateNextManicureRecommendation(*sample);

    auto endRec = std::chrono::high_resolution_clock::now();

    auto addTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(endAdd - startAdd);

    auto searchTime =
        std::chrono::duration_cast<std::chrono::milliseconds>(endSearch - startSearch);

    auto recTime =
        std::chrono::duration_cast<std::chrono::microseconds>(endRec - startRec);

    std::cout << "Add " << N << " manicures: "
              << addTime.count() << " ms\n";

    std::cout << "Search results: " << results.size()
              << " in " << searchTime.count() << " ms\n";

    std::cout << "Recommendation generation: "
              << recTime.count() << " us\n";
}