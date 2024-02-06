#include "memmanager.h"
#include "xed/xed-iclass-enum.h"
#include <mach/mach_traps.h>
#include <mach/vm_map.h>
#include <mach/vm_prot.h>
#include <mach/mach_init.h>
#include <memory>
#include <unordered_map>
#include <sys/mman.h>

extern "C" {
    #include "xed/xed-encode.h"
}

thread_local __m128 ymmStorage[32];

__m128 *get_ymm_storage() {
    return ymmStorage;
}

uint8_t* alloc_executable(uint64_t size) {
    // mmap anonymous memory
    auto memory = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
    return memory;
}

static std::unordered_map<uint64_t, void*> executable_chunks_for_locations;

void jumptable_add_chunk(uint64_t location, void* chunk) {
    executable_chunks_for_locations[location] = chunk;
}

void* jumptable_get_chunk(uint64_t location) {
    return executable_chunks_for_locations[location];
}
