#include "Compiler.h"
#include "xed/xed-iform-enum.h"
#include "xed/xed-reg-enum.h"
#include <__format/formatter.h>
#include <memory>

extern "C" {
#include "xed/xed-decode.h"
#include "xed/xed-decoded-inst-api.h"
}

#include "../printinstr.h"

#include "../memmanager.h"
#include <vector>
#include <iostream>
#include <format>

#include "../utils.h"

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

Compiler::Compiler() {}

void Compiler::addInstruction(std::shared_ptr<Instruction> const& instr) {
    instructions.push_back(instr);
}

std::vector<Compiler::instruction> Compiler::compile(CompilationStrategy compilationStrategy, uint64_t returnAddress) {
    std::vector<instruction> encodedInstructions;

    const xed_state_t dstate = {.mmode = XED_MACHINE_MODE_LONG_64,
            .stack_addr_width = XED_ADDRESS_WIDTH_64b};

    if (compilationStrategy == CompilationStrategy::FarJump) {
        // pop RAX
        instruction instr = {
            .buffer = {0x58},
            .olen = 1,
        };
        encodedInstructions.push_back(instr); // this restore RAX state to pre-jump
    }

    for (auto& instr : instructions) {
        auto requests = instr->compile(compilationStrategy);

        printf("Compiling %s...\n", xed_iform_enum_t2str(instr->getIform()));

        for (uint32_t i = 0; i < requests.size(); i++) {
            xed_encoder_request_t &req = requests[i];
            instruction instr;
            xed_error_enum_t err = xed_encode(&req, instr.buffer, 15, &instr.olen);
            if (err != XED_ERROR_NONE) {
                printf("%d: Encoder error: %s\n", i, xed_error_enum_t2str(err));
                printf("Instruction: %s\n", xed_iclass_enum_t2str(xed_encoder_request_get_iclass(&req)));
                exit(1);
            }

            // xed_decoded_inst_t xedd;
            // uint32_t olen;
            // decode_instruction3(instr.buffer, &xedd, &olen);

            encodedInstructions.push_back(instr);
        }
    }

    if (compilationStrategy == CompilationStrategy::FarJump) {
        // PUSH RAX
        instruction instr = {
            .buffer = {0x50},
            .olen = 1,
        };
        encodedInstructions.push_back(instr); // this save RAX state for later

        // MOV RAX, returnAddress
        instruction instr2 = {
            .buffer = {0x48, 0xb8},
           .olen = 10,
        };
        *(uint64_t*)(instr2.buffer + 2) = returnAddress;
        encodedInstructions.push_back(instr2);

        // JMP RAX
        instruction instr3 = {
            .buffer = {0xff, 0xe0},
           .olen = 2,
        };
        encodedInstructions.push_back(instr3);
    }

    if (compilationStrategy == CompilationStrategy::DirectCall) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;
        xed_inst0(&enc_inst, 
            dstate, 
            XED_ICLASS_RET_NEAR, 
            64);
        xed_convert_to_encoder_request(&req, &enc_inst);
        instruction instr;
        xed_error_enum_t err = xed_encode(&req, instr.buffer, 15, &instr.olen);
        if (err != XED_ERROR_NONE) {
            printf("Encoder error: %s\n", xed_error_enum_t2str(err));
            exit(1);
        }

        encodedInstructions.push_back(instr);
    }

    return encodedInstructions;
}

uint8_t* Compiler::encode(CompilationStrategy compilationStrategy, uint32_t *length, uint64_t returnAddress) {
    auto const& encodedInstructions = compile(compilationStrategy, returnAddress);

    uint32_t total_olen = 0;
    for (auto const &instr : encodedInstructions) {
        total_olen += instr.olen;
    }

    uint8_t *stencil = alloc_executable(total_olen); // Yes, a memory leak TODO
    uint32_t offset = 0;
    for (auto const &instr : encodedInstructions) {
        memcpy(stencil + offset, instr.buffer, instr.olen);
        offset += instr.olen;
    }

    *length = offset;
    return stencil;
}
