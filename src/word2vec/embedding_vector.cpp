#include "word2vec/embedding/embedding_vector.hpp"
#include <cmath>
#include <algorithm>

EmbeddingVector::EmbeddingVector(size_t dim)
    : data(std::make_unique<double[]>(dim)),
      dimension(dim)
{
    std::fill(data.get(), data.get() + dim, 0.0);
}

EmbeddingVector::EmbeddingVector(const EmbeddingVector& other)
    : data(std::make_unique<double[]>(other.dimension)),
      dimension(other.dimension)
{
    std::copy(other.data.get(),
              other.data.get() + dimension,
              data.get());
}

EmbeddingVector& EmbeddingVector::operator=(const EmbeddingVector& other)
{
    if (this == &other) return *this;

    dimension = other.dimension;
    data = std::make_unique<double[]>(dimension);

    std::copy(other.data.get(),
              other.data.get() + dimension,
              data.get());

    return *this;
}

EmbeddingVector::EmbeddingVector(EmbeddingVector&& other) noexcept
    : data(std::move(other.data)),
      dimension(other.dimension)
{
    other.dimension = 0;
}

EmbeddingVector& EmbeddingVector::operator=(EmbeddingVector&& other) noexcept
{
    data = std::move(other.data);
    dimension = other.dimension;
    other.dimension = 0;
    return *this;
}

double& EmbeddingVector::operator[](size_t i)
{
    return data[i];
}

const double& EmbeddingVector::operator[](size_t i) const
{
    return data[i];
}

size_t EmbeddingVector::size() const
{
    return dimension;
}

double EmbeddingVector::dot(const EmbeddingVector& other) const
{
    double s = 0.0;
    for (size_t i = 0; i < dimension; ++i)
        s += data[i] * other.data[i];
    return s;
}

double EmbeddingVector::norm() const
{
    return std::sqrt(dot(*this));
}

void EmbeddingVector::sgd_update(const EmbeddingVector& grad, double lr)
{
    for (size_t i = 0; i < dimension; ++i)
        data[i] -= lr * grad.data[i];
}