#pragma once
#include <vector>
#include <utility>
#include "word2vec/embedding/embedding_vector.hpp"

#include "photo_repository/maniqure_data_updated.h"

class SimilarityEngine {
private:
    std::vector<EmbeddingVector> vectors;
    std::vector<PhotoId> ids;

public:
    void add(PhotoId id, const EmbeddingVector& vec);

    std::vector<std::pair<double, PhotoId>>
    search(const EmbeddingVector& query, size_t k) const;

    size_t size() const;
};