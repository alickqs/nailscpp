#include "word2vec/data/dataset_loader.hpp"
#include "word2vec/core/exceptions.hpp"
#include <fstream>
#include <sstream>

std::optional<std::vector<std::vector<int>>>
DatasetLoader::load(const std::string& path, Vocabulary& vocab) {

    std::ifstream file(path);

    if (!file.is_open())
        throw FileIOException(path);

    std::vector<std::vector<int>> data;
    std::string line;

    while (std::getline(file, line)) {

        auto tokens = tokenizer.tokenize(line);

        std::vector<int> sentence;

        for (const auto& token : tokens) {
            int id = vocab.add_word(token);
            sentence.push_back(id);
        }

        if (!sentence.empty())
            data.push_back(sentence);
    }

    if (data.empty())
        return std::nullopt;

    return data;
}