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

static std::unordered_map<uint64_t, ymm_t*> ymm_register_file;

ymm_t* get_ymm_for_thread(uint64_t tid) {
    if (!ymm_register_file.contains(tid)) {
        ymm_register_file[tid] = new ymm_t {0};
    }

    return ymm_register_file[tid];
}

uint8_t* alloc_executable(uint64_t size) {
    // mmap anonymous memory
    auto memory = (uint8_t*)mmap(NULL, size, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, -1, 0);
    return memory;
}
