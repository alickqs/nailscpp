#include "word2vec/data/vocabulary.hpp"
#include "word2vec/core/exceptions.hpp"
#include <random>

Vocabulary::Vocabulary(size_t dim) : embedding_dim(dim) {}

int Vocabulary::add_word(const std::string& word) {

    auto it = word_to_id.find(word);

    if (it != word_to_id.end()) {
        frequencies[it->second]++;
        return it->second;
    }

    int id = static_cast<int>(id_to_word.size());

    word_to_id[word] = id;
    id_to_word.push_back(word);
    frequencies.push_back(1);

    embeddings.emplace_back(embedding_dim);

    return id;
}

std::optional<int> Vocabulary::get_id(const std::string& word) const {

    auto it = word_to_id.find(word);

    if (it == word_to_id.end())
        return std::nullopt;

    return it->second;
}

const std::string& Vocabulary::get_word(size_t id) const {

    if (id >= id_to_word.size())
        throw VocabularyException("Invalid word id");

    return id_to_word[id];
}

EmbeddingVector& Vocabulary::get_embedding(size_t id) {

    if (id >= embeddings.size())
        throw VocabularyException("Invalid embedding id");

    return embeddings[id];
}

std::optional<std::reference_wrapper<EmbeddingVector>>
Vocabulary::get_embedding(const std::string& word) {

    auto id = get_id(word);

    if (!id)
        return std::nullopt;

    return embeddings[*id];
}

const std::vector<int>& Vocabulary::get_frequencies() const {
    return frequencies;
}

size_t Vocabulary::size() const {
    return id_to_word.size();
}