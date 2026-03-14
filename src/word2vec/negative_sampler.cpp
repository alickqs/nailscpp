#include "word2vec/sampling/negative_sampler.hpp"
#include <numeric>
#include <algorithm>
#include <random>
#include <stdexcept>

NegativeSampler::NegativeSampler(const std::vector<int>& frequencies)
{
    if (frequencies.empty())
        throw std::runtime_error("NegativeSampler: empty frequency table");

    cumulative.resize(frequencies.size());

    double sum = 0.0;
    for (size_t i = 0; i < frequencies.size(); ++i)
    {
        sum += std::pow(frequencies[i], 0.75);
        cumulative[i] = sum;
    }

    for (double& v : cumulative)
        v /= sum;

    rng = std::make_unique<std::mt19937>(
        std::random_device{}());
}

int NegativeSampler::sample()
{
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double x = dist(*rng);

    auto it = std::lower_bound(
        cumulative.begin(),
        cumulative.end(),
        x);

    if (it == cumulative.end())
        return cumulative.size() - 1;

    return std::distance(cumulative.begin(), it);
}

std::string NegativeSampler::strategy_name() const
{
    return "word2vec_0.75_power";
}