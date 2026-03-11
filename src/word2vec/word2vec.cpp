#include "word2vec/model/word2vec.hpp"
#include "word2vec/core/exceptions.hpp"
#include <fstream>
#include <algorithm>
#include <cmath>

namespace {

double sigmoid(double x) {
    return 1.0 / (1.0 + std::exp(-x));
}

}

Word2VecModel::Word2VecModel(size_t embedding_dim)
    : vocab(embedding_dim), learning_rate(0.025) {

    shared_parameters = std::make_shared<std::vector<double>>(
        std::initializer_list<double>{0.025, 0.95, 5});
}

void Word2VecModel::set_training_data(const std::vector<std::vector<int>>& data) {
    training_data = data;
}

Vocabulary& Word2VecModel::get_vocabulary() {
    return vocab;
}

void Word2VecModel::initialize_sampler() {
    sampler = std::make_unique<NegativeSampler>(vocab.get_frequencies());
}

void Word2VecModel::train(const std::vector<std::vector<int>>& data) {

    double lr = learning_rate;
    const int epochs = 5;
    const int negative_samples = 5;

    for (int epoch = 0; epoch < epochs; ++epoch) {

        for (const auto& sentence : data) {

            for (size_t i = 0; i < sentence.size(); ++i) {

                int target = sentence[i];

                for (size_t j = 0; j < sentence.size(); ++j) {

                    if (i == j) continue;

                    int context = sentence[j];

                    train_pair(target, context, 1.0, lr);

                    for (int k = 0; k < negative_samples; ++k) {
                        int neg = sampler->sample();
                        train_pair(target, neg, 0.0, lr);
                    }
                }
            }
        }

        lr *= 0.95;
    }
}

void Word2VecModel::train_pair(int target, int context, double label, double lr) {

    auto& target_vec = vocab.get_embedding(target);
    auto& context_vec = vocab.get_embedding(context);

    double dot = target_vec.dot(context_vec);
    double pred = sigmoid(dot);
    double grad = pred - label;

    EmbeddingVector grad_t(target_vec.size());
    EmbeddingVector grad_c(context_vec.size());

    for (size_t i = 0; i < target_vec.size(); ++i) {
        grad_t[i] = grad * context_vec[i];
        grad_c[i] = grad * target_vec[i];
    }

    target_vec.sgd_update(grad_t, lr);
    context_vec.sgd_update(grad_c, lr);
}

void Word2VecModel::save(const std::string& path) const {

    std::ofstream file(path);

    if (!file.is_open())
        throw FileIOException(path);

    file << vocab.size() << "\n";

    for (size_t i = 0; i < vocab.size(); ++i) {

        file << vocab.get_word(i);

        const auto& emb = const_cast<Vocabulary&>(vocab).get_embedding(i);

        for (size_t d = 0; d < emb.size(); ++d)
            file << " " << emb[d];

        file << "\n";
    }
}

void Word2VecModel::load(const std::string& path) {
    throw ModelException("Load not implemented");
}

double Word2VecModel::evaluate(const std::vector<int>&,
                               const std::vector<int>&) const {
    return 0.0;
}

std::optional<std::vector<std::pair<double,std::string>>>
Word2VecModel::find_similar(const std::string& word, int k) {

    auto emb_opt = vocab.get_embedding(word);

    if (!emb_opt)
        return std::nullopt;

    auto& target = emb_opt->get();

    std::vector<std::pair<double,std::string>> result;

    for (size_t i = 0; i < vocab.size(); ++i) {

        auto& other = vocab.get_embedding(i);

        double sim = target.dot(other) /
            (target.norm() * other.norm() + 1e-8);

        result.emplace_back(sim, vocab.get_word(i));
    }

    std::sort(result.begin(), result.end(),
              [](auto& a, auto& b) { return a.first > b.first; });

    if (result.size() > static_cast<size_t>(k))
        result.resize(k);

    return result;
}