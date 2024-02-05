#include "encoder.h"
#include "Instructions/Instruction.h"
#include "decoder.h"
#include "memmanager.h"
#include "printinstr.h"
#include "xed/xed-build-defines.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-encode-direct.h"
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-error-enum.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-iform-enum.h"
#include "xed/xed-inst.h"
#include "xed/xed-operand-enum.h"
#include "xed/xed-operand-values-interface.h"
#include "xed/xed-reg-enum.h"
#include <assert.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "Instructions/Instructions.h"

const xed_state_t dstate = {.mmode = XED_MACHINE_MODE_LONG_64,
                            .stack_addr_width = XED_ADDRESS_WIDTH_64b};

struct instruction {
    uint8_t buffer[15];
    uint32_t olen;
};

void decode_instruction3(unsigned char *inst, xed_decoded_inst_t *xedd, uint32_t *olen) {
    xed_machine_mode_enum_t mmode = XED_MACHINE_MODE_LONG_64;
    xed_address_width_enum_t stack_addr_width = XED_ADDRESS_WIDTH_64b;

    const uint32_t instruction_length = 15;

    xed_error_enum_t xed_error;
    xed_decoded_inst_zero(xedd);
    xed_decoded_inst_set_mode(xedd, mmode, stack_addr_width);
    xed_error = xed_decode(xedd, 
                            XED_STATIC_CAST(const xed_uint8_t*,inst),
                            instruction_length);
    printf("Length: %d, Error: %s\n",(int)xed_decoded_inst_get_length(xedd), xed_error_enum_t2str(xed_error));
    *olen = xed_decoded_inst_get_length(xedd);
    print_instr(xedd);
}

uint8_t *encode_requests(std::vector<xed_encoder_request_t> &requests, uint64_t *length) {
    std::vector<instruction> encoded_instructions;
    for (uint32_t i = 0; i < requests.size(); i++) {
        xed_encoder_request_t &req = requests[i];
        printf("%d: Encoding %s\n", i,
               xed_iclass_enum_t2str(xed_encoder_request_get_iclass(&req)));
        instruction instr;
        xed_error_enum_t err = xed_encode(&req, instr.buffer, 15, &instr.olen);
        if (err != XED_ERROR_NONE) {
            printf("%d: Encoder error: %s\n", i, xed_error_enum_t2str(err));
            exit(1);
        }

        // xed_decoded_inst_t xedd;
        // uint32_t olen;
        // decode_instruction3(instr.buffer, &xedd, &olen);

        encoded_instructions.push_back(instr);
    }

    uint64_t total_olen = 0;
    for (auto const &instr : encoded_instructions) {
        total_olen += instr.olen;
    }

    uint8_t *stencil = alloc_executable(total_olen); // Yes, a memory leak TODO
    uint64_t offset = 0;
    for (auto const &instr : encoded_instructions) {
        memcpy(stencil + offset, instr.buffer, instr.olen);
        offset += instr.olen;
    }

    *length = offset;
    return stencil;
}

void encode_instruction(xed_decoded_inst_t *xedd, uint8_t *buffer,
                        const unsigned int ilen, unsigned int *olen,
                        uint64_t tid, uint64_t rbp_value, uint64_t rip_value, uint64_t rsp_value) {
    xed_iclass_enum_t iclass = xed_decoded_inst_get_iclass(xedd);

    if (!iclassMapping.contains(iclass)) {
        printf("Unsupported iclass %s\n", xed_iclass_enum_t2str(iclass));
        exit(1);
    }

    auto instrFactory = iclassMapping.at(iclass);
    std::shared_ptr<Instruction> instr = instrFactory(rip_value, ilen, xedd);

    ymm_t *ymm = get_ymm_for_thread(tid);
    // printf("YMM bank at %p for thread %llu\n", ymm, tid);
    auto requests = instr->compile(ymm, CompilationStrategy::DirectCall);

    uint64_t chunk_length = 0;
    uint8_t *chunk = encode_requests(requests, &chunk_length);
    const uint64_t retLength = 1;
    printf("chunk_length = %llu\n", chunk_length);

    bool inline_compiled = false;
    // if (chunk_length - 1 <= ilen) {
    //     printf("JITted instructions should fit inline, recompiling for inline\n");
    //     requests = instr->compile(ymm, CompilationStrategy::Inline);

    //     chunk_length = 0;
    //     chunk = encode_requests(requests, &chunk_length);
    //     inline_compiled = true;
    // }

    if (inline_compiled) {
        memcpy(buffer, chunk, chunk_length);
        *olen = chunk_length;
    } else {
        uint64_t chunk_addr = (uint64_t)chunk;
        int64_t displacement = chunk_addr - rip_value;
        int32_t displacement32 = (int32_t)displacement - 5;
        if (displacement32 != displacement - 5) {
            printf("rip displacement too big\n");
            printf("rip displacement: 0x%llx (%lld)\n", displacement, displacement);
            printf("rip displacement32: 0x%x (%d)\n", displacement32, displacement32);
            printf("Emitting INT3\n");
            buffer[0] = 0xcc;
            *olen = 1;

            // printf("Recompiling for exception call\n");

            // requests = instr->compile(ymm, CompilationStrategy::ExceptionCall, rip_value+1);

            // chunk_length = 0;
            // chunk = encode_requests(requests, &chunk_length);
            // *chunk = 0xcc;

            jumptable_add_chunk(rip_value + (ilen - 1), chunk);
            return;
        }

        buffer[0] = 0xe8;
        buffer[1] = displacement32 & 0xff;
        buffer[2] = (displacement32 & 0xff00) >> 8;
        buffer[3] = (displacement32 & 0xff0000) >> 16;
        buffer[4] = (displacement32 & 0xff000000) >> 24;
        *olen = 5;
    }
}