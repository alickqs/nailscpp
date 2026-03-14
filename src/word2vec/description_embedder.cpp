#include "word2vec/search/description_embedder.hpp"

EmbeddingVector DescriptionEmbedder::embed(const std::string& text,
                                           Vocabulary& vocab) {

    auto tokens = tokenizer.tokenize(text);

    EmbeddingVector result(dim);

    int count = 0;

    for (auto& t : tokens) {
        auto id = vocab.get_id(t);
        if (!id) continue;

        auto& w = vocab.get_embedding(*id);

        for (size_t i = 0; i < dim; ++i)
            result[i] += w[i];

        count++;
    }

    if (count == 0) return result;

    for (size_t i = 0; i < dim; ++i)
        result[i] /= count;

    return result;
}