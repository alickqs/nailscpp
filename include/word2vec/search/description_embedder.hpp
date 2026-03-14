#pragma once
#include <string>
#include "word2vec/data/tokenizer.hpp"
#include "word2vec/data/vocabulary.hpp"
#include "word2vec/embedding/embedding_vector.hpp"

class DescriptionEmbedder {
private:
    Tokenizer tokenizer;
    size_t dim;

public:
    explicit DescriptionEmbedder(size_t d) : dim(d) {}

    EmbeddingVector embed(const std::string& text, Vocabulary& vocab);
};