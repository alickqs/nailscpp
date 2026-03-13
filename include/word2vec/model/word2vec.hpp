#pragma once

#include <vector>
#include <string>
#include <memory>
#include <optional>

#include "word2vec/core/ITrainable.hpp"
#include "word2vec/data/vocabulary.hpp"
#include "word2vec/sampling/NegativeSampler.hpp"

class Word2VecModel : public ITrainable {

private:
    Vocabulary vocab;
    std::unique_ptr<NegativeSampler> sampler;
    double learning_rate;

public:
    explicit Word2VecModel(size_t dim);

    Vocabulary& get_vocabulary();

    void initialize_sampler();

    void train(const std::vector<std::vector<int>>& data) override;

    void train_pair(int target, int context, double label);

    std::optional<std::vector<std::pair<double,std::string>>>
    find_similar(const std::string& word, int k);

    void save(const std::string& path) const override;
    void load(const std::string& path) override;
    double evaluate(const std::vector<int>&,
                    const std::vector<int>&) const override;
};
