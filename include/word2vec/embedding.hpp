#pragma once

#include <vector>

using EmbeddingVector = std::vector<double>;

double dot(const EmbeddingVector& a, const EmbeddingVector& b);

double cosine_similarity(const EmbeddingVector& a,
                         const EmbeddingVector& b);