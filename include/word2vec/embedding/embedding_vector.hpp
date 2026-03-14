#pragma once
#include <memory>
#include <cstddef>

class EmbeddingVector
{
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

    double& operator[](size_t i);
    const double& operator[](size_t i) const;

    size_t size() const;

    double dot(const EmbeddingVector& other) const;
    double norm() const;

    void sgd_update(const EmbeddingVector& grad, double lr);
};