#include "word2vec/sampling/NegativeSampler.hpp"
#include <algorithm>
#include <cmath>

NegativeSampler::NegativeSampler(const std::vector<int>& frequencies, double power) {

    std::random_device rd;
    rng = std::make_unique<std::mt19937>(rd());

    std::vector<double> prob(frequencies.size());
    double sum = 0.0;

    for (size_t i = 0; i < frequencies.size(); ++i) {
        prob[i] = std::pow(frequencies[i], power);
        sum += prob[i];
    }

    for (double& p : prob)
        p /= sum;

    cumulative_table.resize(prob.size());

    cumulative_table[0] = prob[0];

    for (size_t i = 1; i < prob.size(); ++i)
        cumulative_table[i] = cumulative_table[i - 1] + prob[i];

    current_strategy = power;
}

int NegativeSampler::sample() {

    std::uniform_real_distribution<double> dist(0.0, 1.0);
    double r = dist(*rng);

    auto it = std::lower_bound(cumulative_table.begin(),
                               cumulative_table.end(),
                               r);

    return std::distance(cumulative_table.begin(), it);
}

std::string NegativeSampler::get_strategy_description() const {

    return std::visit([](auto&& arg) -> std::string {

        using T = std::decay_t<decltype(arg)>;

        if constexpr (std::is_same_v<T, std::monostate>)
            return "Default strategy";

        else if constexpr (std::is_same_v<T, std::string>)
            return "Named strategy: " + arg;

        else if constexpr (std::is_same_v<T, double>)
            return "Power strategy with factor: " + std::to_string(arg);

        return "Unknown";

    }, current_strategy);
}