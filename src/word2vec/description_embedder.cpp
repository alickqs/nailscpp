#include "word2vec/search/description_embedder.hpp"

EmbeddingVector DescriptionEmbedder::embed(const std::string& text,
                                           Vocabulary& vocab) {

    auto tokens = tokenizer.tokenize(text);

    EmbeddingVector vec(vocab.get_embedding(0).size());

    size_t count = 0;

    for (const auto& t : tokens) {

        auto emb = vocab.get_embedding(t);

        if (!emb) continue;

        vec.add(emb->get());
        count++;
    }

    if (count > 0)
        vec.scale(1.0 / count);

    return vec;
}