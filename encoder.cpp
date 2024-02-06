#include "encoder.h"
#include "Compiler/Compiler.h"
#include "Instructions/Instruction.h"
#include "decoder.h"
#include "memmanager.h"
#include "printinstr.h"
#include "utils.h"
#include "xed/xed-build-defines.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-decoded-inst.h"
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
#include <mach/i386/kern_return.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <memory>
#include <stdio.h>
#include <stdlib.h>
#include <vector>

#include "Instructions/Instructions.h"

const xed_state_t dstate = {.mmode = XED_MACHINE_MODE_LONG_64,
                            .stack_addr_width = XED_ADDRESS_WIDTH_64b};

void decode_instruction_internal(uint8_t *inst, xed_decoded_inst_t *xedd, uint8_t *olen) {
    xed_machine_mode_enum_t mmode = XED_MACHINE_MODE_LONG_64;
    xed_address_width_enum_t stack_addr_width = XED_ADDRESS_WIDTH_64b;

    uint32_t instruction_length = 15;

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

void reencode_instructions(uint8_t* instructionPointer) {
    // decoode as many instructions as we can
    std::vector<std::shared_ptr<Instruction>> decodedInstructions;
    uint64_t decodedInstructionLength = 0;
    while (1) {
        xed_decoded_inst_t xedd;
        uint8_t olen = 15;

        auto currentInstrPointer = instructionPointer + decodedInstructionLength;

        decode_instruction_internal(currentInstrPointer, &xedd, &olen);

        printf("olen = %d\n", olen);
        for(uint32_t i = 0; i < olen; i++) {
            printf(" %02x", (unsigned int)(*((unsigned char*)currentInstrPointer + i)));
        }
        printf("\n");

        decodedInstructionLength += olen;

        xed_iclass_enum_t iclass = xed_decoded_inst_get_iclass(&xedd);

        if (!iclassMapping.contains(iclass)) {
            // We found an unsupported instruction, stop decoding and try to compile
            decodedInstructionLength -= olen;
            break;
        }

        auto instrFactory = iclassMapping.at(iclass);
        auto instr = instrFactory((uint64_t)currentInstrPointer, olen, xedd);
        decodedInstructions.push_back(instr);
    }

    if (decodedInstructions.empty()) {
        printf("No supported instructions found\n");
        printf("Last decoded instruction:\n");
        xed_decoded_inst_t xedd;
        uint8_t olen = 15;
        decode_instruction_internal(instructionPointer + decodedInstructionLength, &xedd, &olen);
        waitForDebugger();
        exit(1);
    }

    //waitForDebugger();

    printf("Compiling...\n");

    // compile the instructions
    Compiler compiler;
    for (auto & instr : decodedInstructions) {
        compiler.addInstruction(instr);
    }

    uint64_t tid;
    pthread_t self;
    self = pthread_self();
    pthread_threadid_np(self, &tid);
    ymm_t *ymm = get_ymm_for_thread(tid); // TODO redo to use thread local storage


    kern_return_t kret = vm_protect(current_task(), (vm_address_t)instructionPointer, decodedInstructionLength, FALSE, VM_PROT_READ | VM_PROT_WRITE | VM_PROT_EXECUTE | VM_PROT_ALL);
    if (kret != KERN_SUCCESS) {
        printf("vm_protect failed: %d\n", kret);
        exit(1);
    }

    // if we have enough bytes to encode 
    // PUSH RAX (1b)
    // MOV RAX imm64 (10b)
    // JMP RAX (2b) - encode a far call
    // POP RAX (1b)
    const uint64_t trampolineSize = 1 + 10 + 2 + 1;
    if (decodedInstructionLength > trampolineSize) {
    // if (false) {
        uint32_t encodedLength = 0;
        uint8_t* chunk = compiler.encode(ymm, CompilationStrategy::FarJump, &encodedLength, (uint64_t)instructionPointer + decodedInstructionLength - 1);

        printf("Chunk at %llx, length %d\n", (uint64_t)chunk, encodedLength);

        // fill nops
        uint32_t i = 0;
        for (i = 0; i < decodedInstructionLength - trampolineSize; i++) {
            instructionPointer[i] = 0x90;
        }
        printf("Written %d nops, will emit trampoline at %llx\n", i, (uint64_t)instructionPointer + i);


        // PUSH RAX
        instructionPointer[i] = 0x50;
        i++;

        // MOV RAX imm64
        instructionPointer[i] = 0x48;
        i++;
        instructionPointer[i] = 0xb8;
        i++;
        *((uint64_t*)(instructionPointer + i)) = (uint64_t)chunk;
        i += 8;

        // JMP RAX
        instructionPointer[i] = 0xff;
        i++;
        instructionPointer[i] = 0xe0;
        i++;

        // POP RAX
        instructionPointer[i] = 0x58;
        i++;
    } else {
        uint32_t encodedLength = 0;
        uint8_t* chunk = compiler.encode(ymm, CompilationStrategy::DirectCall, &encodedLength, -1);

        // otherwise emit INT3 at the end of the block from where we taken the instructions
        // fill nops
        for (uint32_t i = 0; i < decodedInstructionLength - 1; i++) {
            instructionPointer[i] = 0x90;
        }

        instructionPointer[decodedInstructionLength - 1] = 0xcc;
        jumptable_add_chunk((uint64_t)instructionPointer + (decodedInstructionLength - 1), chunk);
    }
}