#pragma once

#include <unordered_map>
#include <vector>
#include <string>
#include <optional>
#include <functional>

#include "word2vec/embedding/embedding_vector.hpp"

class Vocabulary {

private:
    size_t embedding_dim;

    std::unordered_map<std::string,int> word_to_id;
    std::vector<std::string> id_to_word;
    std::vector<int> frequencies;
    std::vector<EmbeddingVector> embeddings;

public:
    explicit Vocabulary(size_t dim);

    int add_word(const std::string& word);

    std::optional<int> get_id(const std::string& word) const;

    const std::string& get_word(size_t id) const;

    EmbeddingVector& get_embedding(size_t id);
    const EmbeddingVector& get_embedding(size_t id) const;

    std::optional<std::reference_wrapper<EmbeddingVector>>
    get_embedding(const std::string& word);

    const std::vector<int>& get_frequencies() const;

    size_t size() const;
};