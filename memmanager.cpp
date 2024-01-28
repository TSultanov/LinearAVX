#include "memmanager.h"
#include <mach/mach_traps.h>
#include <mach/vm_map.h>
#include <mach/vm_prot.h>
#include <mach/mach_init.h>
#include <memory>
#include <unordered_map>

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

struct instruction {
    uint8_t buffer[15];
    uint32_t olen;
};

uint8_t* encode_requests(std::vector<xed_encoder_request_t>& requests) {
    std::vector<instruction> encoded_instructions;
    for (auto& req : requests) {
        instruction instr;
        xed_error_enum_t err = xed_encode(&req, instr.buffer, 15, &instr.olen);
        if (err != XED_ERROR_NONE) {
            printf("Encoder error: %s\n", xed_error_enum_t2str(err));
            exit(1);
        }

        encoded_instructions.push_back(instr);
    }

    uint64_t total_olen = 0;
    for (auto const& instr : encoded_instructions) {
        total_olen += instr.olen;
    }

    uint8_t *stencil = new uint8_t[total_olen]; // Yes, a memory leak
    uint64_t offset = 0;
    for (auto const& instr : encoded_instructions) {
        memcpy(stencil + offset, instr.buffer, instr.olen);
        offset += instr.olen;
    }

    vm_protect(current_task(), (vm_address_t)stencil, total_olen, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE);

    return stencil;
}
