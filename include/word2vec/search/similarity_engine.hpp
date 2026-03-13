#pragma once
#include <vector>
#include <string>
#include <utility>
#include "../embedding.hpp"

class SimilarityEngine {
private:
    std::vector<EmbeddingVector> description_vectors;
    std::vector<std::string> descriptions;

public:
    void add(const std::string& description, const EmbeddingVector& vec);

    std::vector<std::pair<double,std::string>>
    search(const EmbeddingVector& query, size_t k) const;

    size_t size() const;
};