#pragma once
#include <string>
#include "../data/tokenizer.hpp"
#include "../data/vocabulary.hpp"
#include "../embedding.hpp"

class DescriptionEmbedder {
private:
    Tokenizer tokenizer;

public:
    EmbeddingVector embed(const std::string& text, Vocabulary& vocab);
};