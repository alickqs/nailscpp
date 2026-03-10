#pragma once
#include <memory>
#include <stdexcept>

class EmbeddingVector {
private:
    std::unique_ptr<double[]> data;
    size_t dimension;

public:
    explicit EmbeddingVector(size_t dim);
    EmbeddingVector(const EmbeddingVector& other);
    EmbeddingVector& operator=(const EmbeddingVector& other);
    EmbeddingVector(EmbeddingVector&& other) noexcept;
    EmbeddingVector& operator=(EmbeddingVector&& other) noexcept;
    ~EmbeddingVector() = default;

    double& operator[](size_t index);
    const double& operator[](size_t index) const;
    size_t size() const { return dimension; }

    double dot(const EmbeddingVector& other) const;
    double norm() const;
    void sgd_update(const EmbeddingVector& gradient, double learning_rate);
};