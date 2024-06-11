#pragma once

#include "Common.h"
#include <unordered_map>

namespace dm {

class MyHashMap {

private:
    const u32 mParallel = 24;

    std::unordered_map<std::string, u32>    *mData = nullptr;

public:
    MyHashMap(const u32 parallel = 24)
    :
    mParallel(parallel),
    mData(new std::unordered_map<std::string, u32>[mParallel])
    {
        for (u32 i = 0; i < mParallel; i++)
        {
            mData[i].reserve(10000000 * 1.25 / mParallel);
        }
    }

    void emplace(const u32 id, const std::string &key, const u32 value)
    {
        mData[id].emplace(key, value);
    }

    auto find(const u32 id, const std::string &key)
    {
        return mData[id].find(key);
    }

    u32 size()
    {
        u32 _size = 0;
        for (u32 i = 0; i < mParallel; i++) {
            _size += mData[i].size();
        }
        return _size;
    }
};

}