#pragma once
#include <vector>
#include <string>

class ITrainable {
public:
    virtual ~ITrainable() = default;
    virtual void train(const std::vector<std::vector<int>>& data) = 0;
    virtual void save(const std::string& path) const = 0;
    virtual void load(const std::string& path) = 0;
    virtual double evaluate(const std::vector<int>& target,
                           const std::vector<int>& context) const = 0;
};