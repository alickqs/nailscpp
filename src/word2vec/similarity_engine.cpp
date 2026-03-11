#include "word2vec/search/similarity_engine.hpp"
#include <algorithm>
#include <cmath>

void SimilarityEngine::add(const std::string& description,
                           const EmbeddingVector& vec) {

    description_vectors.push_back(vec);
    descriptions.push_back(description);
}

std::vector<std::pair<double,std::string>>
SimilarityEngine::search(const EmbeddingVector& query, size_t k) const {

    std::vector<std::pair<double,std::string>> results;

    for (size_t i = 0; i < description_vectors.size(); ++i) {

        const auto& v = description_vectors[i];

        double sim = query.dot(v) /
            (query.norm() * v.norm() + 1e-8);

        results.emplace_back(sim, descriptions[i]);
    }

    std::sort(results.begin(), results.end(),
              [](auto& a, auto& b) { return a.first > b.first; });

    if (results.size() > k)
        results.resize(k);

    return results;
}

size_t SimilarityEngine::size() const {
    return descriptions.size();
}