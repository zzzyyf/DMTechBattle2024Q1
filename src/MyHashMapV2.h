/**
 * We need to decrease the cost of memory allocate.
 * However, since String's memory is separately allocated, we need a specific map impl
 * to manage it.
 * So we plan to use a very simple hash map impl to give it a try.
 *
 * After the simplest impl, we achieved ~640ms insert and ~60ms search cost, more than 2x faster.
 * According to profiling and stats, collision is very heavy (above 40%),
 * and allocating collided entries costs a lot of time.
 *
 * After using buffer list for collided entry allocation, we achieved ~110ms insert and ~90ms search cost, another 3.5x faster.
 * I guess by using some better hash algorithm like murmur3 can further reduce cost, but let's stop here.
 */
#pragma once

#include "Common.h"
#include "MyHashMap.h"
#include <cassert>
#include <cstring>
#include <list>
#include <type_traits>
#include <vector>


namespace dm {

template <class T>
class LinkBuffer
{
private:
    struct Buffer
    {
        u32         mCapacity = 0;
        T           *mData = nullptr;

        Buffer() = default;

        Buffer(const u32 capacity)
        :
        mCapacity(capacity),
        mData(new T[capacity])
        {

        }

        bool operator==(const Buffer &rhs) const
        {
            return mData == rhs.mData;
        }

        ~Buffer()
        {
            delete[] mData;
        }
    };

    std::list<Buffer>   mBuffers;

    std::list<Buffer>::iterator mItr = mBuffers.end();
    u32                         mItrOffset = 0;

public:
    LinkBuffer(u32 capacity)
    {
        init(capacity);
    }

    void append(u32 capacity)
    {
        auto &ref = mBuffers.emplace_back(capacity);
        if (mItrOffset == mItr->mCapacity)
        {
            auto itr = mItr;
            ++itr;
            assert(itr != mBuffers.end());
            if (*itr == ref)
            {
                mItr++;
                mItrOffset = 0;
            }
        }
    }

    T   *getData(u32 count = 1)
    {
        if (mItrOffset + count >= mItr->mCapacity)
        {
            mItrOffset = mItr->mCapacity;
            return nullptr;
        }

        T *data = mItr->mData + mItrOffset;
        mItrOffset += count;
        return data;
    }

    void init(u32 capacity)
    {
        mBuffers.emplace_back(capacity);
        mItr = mBuffers.begin();
        assert(mItr != mBuffers.end());
        mItrOffset = 0;
    }

};


// 1. manage String data separately
    // convert to StringRef, and store data sequentially
    // use LinkBuffer to contain added spaces
// 2. try to avoid rehash, or at least make rehash fast
    // make very large map and not rehash as a dirty solution
// 3. solve hash conflict efficiently
    // try simple link first
    // use LinkBuffer to contain collided entries
class MapV2
{
private:
    using ValueType = std::pair<StringRef, u32>;
    struct Value
    {
        ValueType   mValue;
        Value       *mNext = nullptr;
    };

    u32     mCapacity;
    Value   *mEntries = nullptr;

    u32     mCollisionCapacity;
    LinkBuffer<Value>   mCollidedEntries;

    u32     mStrDataSize;
    char    *mStrData = nullptr;
    char    *mStrPos = nullptr;

    u32     mSize = 0;
    u32     mCollisionCount = 0;

    std::hash<String>   mHasher;

public:
    MapV2(const u32 capacity)
    :
    mCapacity(capacity * 1.2),  // make bigger map to reduce collision
    mEntries(new Value[mCapacity]),

    mCollisionCapacity(capacity * 0.2),
    mCollidedEntries(mCollisionCapacity),

    mStrDataSize(35 * mCapacity * 1.5),
    mStrData(static_cast<char *>(malloc(mStrDataSize))),
    mStrPos(mStrData)
    {
    }

    __attribute__ ((noinline)) ~MapV2()
    {
        std::cout << mSize << ", collision: " << mCollisionCount << std::endl;
        destroyEntryLists();
        delete[] mEntries;
        std::cout << this << " str data size: " << mStrDataSize << std::endl;
        free(mStrData);
    }

    void emplace(const std::string &key, const u32 value)
    {
        u32 pos = mHasher(key) % mCapacity;
        Value *entry = &mEntries[pos];

        if (!entry->mValue.first.empty())
        {
            Value *collided_entry = mCollidedEntries.getData();
            if (!collided_entry) [[unlikely]]
            {
                // std::cout << this << " collided data appended!" << std::endl;
                mCollidedEntries.append(mCollisionCapacity);
                collided_entry = mCollidedEntries.getData();
                assert(collided_entry);
            }

            while (entry->mNext != nullptr) {
                entry = entry->mNext;
            }

            Value *prior = entry;
            entry = collided_entry;
            prior->mNext = entry;

            ++mCollisionCount;
        }

        if (mStrPos + key.size() > mStrData + mStrDataSize) [[unlikely]]
        {
            // std::cout << this << " str data reallocated!" << std::endl;
            u32 used = mStrPos - mStrData;
            mStrDataSize *= 1.2;
            mStrData = static_cast<char *>(realloc(static_cast<void *>(mStrData), mStrDataSize));
            mStrPos = mStrData + used;
        }

        ::memcpy(mStrPos, key.data(), key.size());
        entry->mValue.first = StringRef(mStrPos, key.size());
        entry->mValue.second = value;

        mStrPos += key.size();
        ++mSize;
    }

    auto find(const std::string &key)
    {
        u32 pos = mHasher(key) % mCapacity;
        Value *entry = &mEntries[pos];

        do {
#if !defined(NDEBUG)
            assert(!entry->mValue.first.empty());
#endif
            // see if we can filter lots of different entries by first char
            if (key[0] == entry->mValue.first[0] && key == entry->mValue.first) {
                return entry->mValue.second;
            }

            entry = entry->mNext;
        } while (entry);

        __builtin_unreachable();
    }

    u32 size() { return mSize; }

private:
    void destroyEntryLists()
    {
    }
};

class MyHashMapV2
{
private:
    const u32 mParallel = 24;

    std::vector<MapV2*>   mData;

public:
    MyHashMapV2(const u32 parallel = 24)
    :
    mParallel(parallel),
    mData(mParallel)
    {
        for (u32 i = 0; i < mParallel; i++)
        {
            mData[i] = new MapV2(10000000 * 1.25 / mParallel);
        }
    }

    void emplace(const u32 id, const std::string &key, const u32 value)
    {
        mData[id]->emplace(key, value);
    }

    auto find(const u32 id, const std::string &key)
    {
        return mData[id]->find(key);
    }

    u32 size()
    {
        u32 _size = 0;
        for (u32 i = 0; i < mParallel; i++) {
            _size += mData[i]->size();
        }
        return _size;
    }
};

}