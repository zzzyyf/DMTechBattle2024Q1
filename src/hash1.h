#pragma once

#include "Common.h"

#include <cassert>
#include <mutex>
#include <unordered_map>

namespace dm {

static u64 primes[60];

class Hash1
{
public:
    std::size_t operator()(const std::string &key) const
    {
        u64 hash = 0;
        for (u32 i = 0, n = key.size(); i < n; i++)
        {
            hash += (key[i] - 'a') * primes[i] % 1'000'000'009; 
        }
        return hash;
    }
};

inline void initHash1()
{
    primes[0] = 1;

    for (u32 i = 1; i < 60; i++)
    {
        primes[i] = primes[i-1] * 31;
    }
}

};