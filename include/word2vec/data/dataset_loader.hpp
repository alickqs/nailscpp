#pragma once
#include <vector>
#include <string>
#include <optional>
#include "vocabulary.hpp"
#include "tokenizer.hpp"

class DatasetLoader {
private:
    Tokenizer tokenizer;

public:
    std::optional<std::vector<std::vector<int>>>
    load(const std::string& path, Vocabulary& vocab);
};
