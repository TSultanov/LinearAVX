#pragma once

#include "../Cache/Cache.h"
#include "../Instructions/Instruction.h"

class Encoder {
    Cache cache;

    uint64_t totalInstructionsRecompiled = 0;
    pthread_mutex_t csMutex =  PTHREAD_RECURSIVE_MUTEX_INITIALIZER;

    enum class DecoderError {
        NopTrap,
        UnsupportedInstruction,
    };

    struct DecodedInstructions {
        const std::vector<std::shared_ptr<Instruction>> instructions;
        const uint64_t decodedInstructionLength;
    };

    std::variant<DecodedInstructions, DecoderError> decodeInstructions(const uint8_t* instructionPointer) const;
    void emitInstructions(DecodedInstructions const& instructions, uint8_t* instructionPointer);
    void printStats() const;
public:
    Encoder(Cache && cache) 
    : cache(cache)
    {}

    int reencodeInstruction(void* instructionPointer);
};