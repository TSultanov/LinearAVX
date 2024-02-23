#pragma once

#include <cstdint>
#include <vector>

struct CacheRecord {
    CacheRecord(const uint64_t rip, const uint64_t origLength, const uint8_t* chunk, const uint64_t chunkLength)
    : rip(rip)
    , origLength(origLength)
    , chunk(chunk)
    , chunkLength(chunkLength)
    {}

    const uint64_t rip;
    const uint64_t origLength;
    const uint8_t* chunk;
    const uint64_t chunkLength;
};

class Cache {
    const uint64_t cacheVersion = 0;

public:
    void store(CacheRecord const& record);
    CacheRecord get(const uint64_t rip) const;
    std::vector<uint64_t> getReplacementPoints() const;
};
