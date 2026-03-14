#include "word2vec/model/word2vec.hpp"
#include "word2vec/core/exceptions.hpp"
#include <cmath>
#include <algorithm>
#include <fstream>
#include <iostream>

Word2VecModel::Word2VecModel(size_t dim)
    : vocab(dim),
      learning_rate(0.025)
{
}

Vocabulary& Word2VecModel::get_vocabulary()
{
    return vocab;
}

void Word2VecModel::initialize_sampler()
{
    sampler = std::make_unique<NegativeSampler>(
        vocab.get_frequencies());
}

void Word2VecModel::train(const std::vector<std::vector<int>>& data)
{
    if (!sampler)
        throw ModelException("Negative sampler not initialized");

    const int window = 2;

    for (const auto& sentence : data)
    {
        for (size_t i = 0; i < sentence.size(); ++i)
        {
            int target = sentence[i];

            int start = std::max<int>(0, i - window);
            int end   = std::min<int>(sentence.size(), i + window + 1);

            for (int j = start; j < end; ++j)
            {
                if (j == i) continue;

                int context = sentence[j];

                train_pair(target, context, 1.0);

                for (int k = 0; k < 5; ++k)
                {
                    int neg = sampler->sample();
                    train_pair(target, neg, 0.0);
                }
            }
        }
    }
}

void Word2VecModel::train_pair(int target,
                               int context,
                               double label)
{
    auto& v_target  = vocab.get_embedding(target);
    auto& v_context = vocab.get_embedding(context);

    double score = v_target.dot(v_context);
    double pred  = 1.0 / (1.0 + std::exp(-score));
    double grad  = learning_rate * (label - pred);

    for (size_t d = 0; d < v_target.size(); ++d)
    {
        double temp = v_target[d];
        v_target[d]  += grad * v_context[d];
        v_context[d] += grad * temp;
    }
}

std::optional<std::vector<std::pair<double,std::string>>>
Word2VecModel::find_similar(const std::string& word, int k)
{
    auto idOpt = vocab.get_id(word);
    if (!idOpt)
        return std::nullopt;

    int id = *idOpt;

    std::vector<std::pair<double,std::string>> result;

    const auto& vec = vocab.get_embedding(id);

    for (size_t i = 0; i < vocab.size(); ++i)
    {
        if ((int)i == id) continue;

        double sim =
            vec.dot(vocab.get_embedding(i)) /
            (vec.norm() * vocab.get_embedding(i).norm() + 1e-9);

        result.emplace_back(sim, vocab.get_word(i));
    }

    std::sort(result.begin(), result.end(),
              [](auto& a, auto& b)
              {
                  return a.first > b.first;
              });

    if ((int)result.size() > k)
        result.resize(k);

    return result;
}

void Word2VecModel::save(const std::string& path) const
{
    std::ofstream out(path, std::ios::binary);
    if (!out)
        throw FileIOException(path);

    size_t vs = vocab.size();
    out.write((char*)&vs, sizeof(vs));
}

void Word2VecModel::load(const std::string& path)
{
    std::ifstream in(path, std::ios::binary);
    if (!in)
        throw FileIOException(path);

    size_t vs;
    in.read((char*)&vs, sizeof(vs));
}

double Word2VecModel::evaluate(const std::vector<int>&,
                               const std::vector<int>&) const
{
    return 0.0;
}