#include "word2vec/search/similarity_engine.hpp"
#include <algorithm>

void SimilarityEngine::add(PhotoId id, const EmbeddingVector& vec) {
    vectors.push_back(vec);
    ids.push_back(id);
}

std::vector<std::pair<double, PhotoId>>
SimilarityEngine::search(const EmbeddingVector& q, size_t k) const {

    std::vector<std::pair<double, PhotoId>> res;

    for (size_t i = 0; i < vectors.size(); ++i) {

        double sim =
            q.dot(vectors[i]) /
            (q.norm() * vectors[i].norm() + 1e-8);

        res.emplace_back(sim, ids[i]);
    }

    std::sort(res.begin(), res.end(),
              [](auto& a, auto& b){ return a.first > b.first; });

    if (res.size() > k) res.resize(k);
    return res;
}

size_t SimilarityEngine::size() const {
    return ids.size();
}