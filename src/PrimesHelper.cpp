#include <limits>
#include <cmath>

#include "PrimesHelper.h"

namespace {
    constexpr size_t Primes[] = {
            3,
            7,
            17,
            23,
            29,
            37,
            47,
            59,
            71,
            89,
            107,
            131,
            163,
            197,
            239,
            293,
            353,
            431,
            521,
            631,
            761,
            919,
            1103,
            1327,
            1597,
            1931,
            2333,
            2801,
            3371,
            4049,
            4861,
            5839,
            7013,
            8419,
            10103,
            12143,
            14591,
            17519,
            21023,
            25229,
            30293,
            36353,
            43627,
            52361,
            62851,
            75431,
            90523,
            108631,
            130363,
            156437,
            187751,
            225307,
            270371,
            324449,
            389357,
            467237,
            560689,
            672827,
            807403,
            968897,
            1162687,
            1395263,
            1674319,
            2009191,
            2411033,
            2893249,
            3471899,
            4166287,
            4999559,
            5999471,
            7199369
    };

    constexpr size_t MaxPrimeFromList = Primes[sizeof(Primes) / sizeof(size_t) - 1];

    constexpr bool IsPrime(size_t n) {
        if ((n & 1) == 0) {
            return n == 2;
        }

        const auto bound = static_cast<size_t>(std::sqrt(n));

        for (size_t i = 3; i <= bound; i += 2) {
            if (n % i == 0) {
                return false;
            }
        }

        return true;
    }

    constexpr size_t MaxPrime = 2146435069;
}

size_t PrimesHelper::ExpandPrime(size_t n) {
    const auto min = n * 2;

    return n < MaxPrime && MaxPrime < min ? MaxPrime : GetPrime(min);
}

size_t PrimesHelper::GetPrime(size_t min) {
    if (min <= MaxPrimeFromList) {
        for (auto prime : Primes) {
            if (prime >= min) {
                return prime;
            }
        }
    }

    for (auto num = min | 1; num < std::numeric_limits<size_t>::max(); num += 2) {
        if (IsPrime(num)) {
            return num;
        }
    }

    throw PrimesError("Cant find big enough prime");
}
