
#include "word2vec/data/tokenizer.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

std::vector<std::string> Tokenizer::tokenize(const std::string& text) const {

    std::string cleaned = text;

    std::transform(cleaned.begin(), cleaned.end(), cleaned.begin(),
        [](unsigned char c) {
            if (std::ispunct(c)) return ' ';
            return static_cast<char>(std::tolower(c));
        });

    std::stringstream ss(cleaned);
    std::vector<std::string> tokens;
    std::string word;

    while (ss >> word)
        tokens.push_back(word);

    return tokens;
}