#include "memmanager.h"
#include "xed/xed-iclass-enum.h"
#include <dlfcn.h>
// #include <mach/mach_traps.h>
// #include <mach/vm_map.h>
// #include <mach/vm_prot.h>
// #include <mach/mach_init.h>
#include <memory>
#include <pthread.h>
// #include <sys/_pthread/_pthread_key_t.h>
#include <unordered_map>
#include <sys/mman.h>

extern "C" {
    #include "xed/xed-encode.h"
}

static thread_local __m128 gYmmStorage[32];

// static pthread_key_t tls_key;
// static bool initialized = false;

__m128 *get_ymm_storage() {
    uint64_t /*rax,*/ rbx, rcx, rdx, rsi, rdi, r8, r9, r10, r11, r12, r13, r14, r15;

    // Save registers
    //__asm__ volatile("mov %%rax, %0" : "=r"(rax));
    __asm__ volatile("mov %%rbx, %0" : "=r"(rbx));
    __asm__ volatile("mov %%rcx, %0" : "=r"(rcx));
    __asm__ volatile("mov %%rdx, %0" : "=r"(rdx));
    __asm__ volatile("mov %%rsi, %0" : "=r"(rsi));
    __asm__ volatile("mov %%rdi, %0" : "=r"(rdi));
    __asm__ volatile("mov %%r8, %0" : "=r"(r8));
    __asm__ volatile("mov %%r9, %0" : "=r"(r9));
    __asm__ volatile("mov %%r10, %0" : "=r"(r10));
    __asm__ volatile("mov %%r11, %0" : "=r"(r11));
    __asm__ volatile("mov %%r12, %0" : "=r"(r12));
    __asm__ volatile("mov %%r13, %0" : "=r"(r13));
    __asm__ volatile("mov %%r14, %0" : "=r"(r14));
    __asm__ volatile("mov %%r15, %0" : "=r"(r15));


    auto ymmStorage = gYmmStorage;

    // Restore registers
    //__asm__ volatile("mov %0, %%rax" : : "r"(rax));
    __asm__ volatile("mov %0, %%rbx" : : "r"(rbx));
    __asm__ volatile("mov %0, %%rcx" : : "r"(rcx));
    __asm__ volatile("mov %0, %%rdx" : : "r"(rdx));
    __asm__ volatile("mov %0, %%rsi" : : "r"(rsi));
    __asm__ volatile("mov %0, %%rdi" : : "r"(rdi));
    __asm__ volatile("mov %0, %%r8" : : "r"(r8));
    __asm__ volatile("mov %0, %%r9" : : "r"(r9));
    __asm__ volatile("mov %0, %%r10" : : "r"(r10));
    __asm__ volatile("mov %0, %%r11" : : "r"(r11));
    __asm__ volatile("mov %0, %%r12" : : "r"(r12));
    __asm__ volatile("mov %0, %%r13" : : "r"(r13));
    __asm__ volatile("mov %0, %%r14" : : "r"(r14));
    __asm__ volatile("mov %0, %%r15" : : "r"(r15));

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
