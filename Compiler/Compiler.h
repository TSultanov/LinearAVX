#pragma once

extern "C"{
#include <xed/xed-interface.h>
#include <xed/xed-decoded-inst-api.h>
#include <xed/xed-encode.h>
}

#include <vector>
#include <memory>
#include "../Instructions/Instruction.h"

class Compiler {
    std::vector<std::shared_ptr<Instruction>> instructions;
public:
    struct instruction {
        uint8_t buffer[15];
        uint32_t olen;
    };

    Compiler();

    void addInstruction(std::shared_ptr<Instruction> const& instr);

    std::vector<instruction> compile(CompilationStrategy compilationStrategy, uint64_t returnAddress);

    uint8_t* encode(CompilationStrategy compilationStrategy, uint32_t *length, uint64_t returnAddress);
};