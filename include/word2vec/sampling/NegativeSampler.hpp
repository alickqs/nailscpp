#pragma once

#include <vector>
#include <memory>
#include <random>
#include <variant>
#include <string>

class NegativeSampler {
private:
    std::vector<double> cumulative;
    std::unique_ptr<std::mt19937> rng;

public:
    explicit NegativeSampler(const std::vector<int>& frequencies);

    int sample();

    std::string strategy_name() const;
};
