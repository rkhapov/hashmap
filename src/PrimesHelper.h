#pragma once

#include <stdexcept>

class PrimesError : std::runtime_error {
public:
    explicit PrimesError(const std::string &message) : std::runtime_error(message) {
    }
};

class PrimesHelper {
public:
    static size_t ExpandPrime(size_t n);

    static size_t GetPrime(size_t min);
};
