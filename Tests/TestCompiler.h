#pragma once
#include <unordered_set>
#include <vector>
#include <memory>
#ifdef __cplusplus
extern "C" {
#endif
#include <xed/xed-interface.h>
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-error-enum.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-iform-enum.h"
#include "xed/xed-inst.h"
#include "xed/xed-operand-enum.h"
#include "xed/xed-iform-map.h"
#include "xed/xed-reg-enum.h"
#include "xed/xed-category-enum.h"
#include "xed/xed-operand-accessors.h"
#include "xed/xed-address-width-enum.h"
#include "xed/xed-machine-mode-enum.h"
#include "xed/xed-reg-class-enum.h"
#include "xed/xed-operand-type-enum.h"
#ifdef __cplusplus
}
#endif
#include "../Instructions/Metadata.h"

inline xed_reg_enum_t xmmToYmm(xed_reg_enum_t reg) {
    return (xed_reg_enum_t)(reg + (XED_REG_YMM0 - XED_REG_XMM0));
}

inline xed_reg_enum_t ymmToXmm(xed_reg_enum_t reg) {
    return (xed_reg_enum_t)(reg - (XED_REG_YMM0 - XED_REG_XMM0));
}

struct TempMemory {
    TempMemory(xed_reg_enum_t baseReg, std::vector<uint8_t> const& memory)
    : baseReg(baseReg)
    , memory(memory)
    {}

    const xed_reg_enum_t baseReg;
    std::vector<uint8_t> memory;
};

struct ThunkRequest {
    ThunkRequest(std::unordered_set<xed_reg_enum_t> const& usedRegisters,
    TempMemory const& usedMemory,
    xed_encoder_request_t instructionRequest)
    : usedRegisters(usedRegisters)
    , usedMemory(usedMemory)
    , instructionRequest(instructionRequest)
    {}

    const std::unordered_set<xed_reg_enum_t> usedRegisters;
    const TempMemory usedMemory;
    const xed_encoder_request_t instructionRequest;
};

struct TestThunk {
    TestThunk(std::unordered_set<xed_reg_enum_t> const& usedRegisters,
    TempMemory const& usedMemory,
    const void* compiledNativeThunk,
    const void* compiledTranslatedThunk)
    : usedRegisters(usedRegisters)
    , usedMemory(usedMemory)
    , compiledNativeThunk(compiledNativeThunk)
    , compiledTranslatedThunk(compiledTranslatedThunk)
    {}

    const std::unordered_set<xed_reg_enum_t> usedRegisters;
    TempMemory usedMemory;
    const void* compiledNativeThunk;
    const void* compiledTranslatedThunk;
};

class TestCompiler {
    InstructionMetadata const& metadata;
public:
    TestCompiler(InstructionMetadata const& metadata);

    std::vector<TestThunk> getThunks() const;

    static void* compileRequests(std::vector<xed_encoder_request_t> requests);

    static const std::vector<xed_reg_enum_t> gpRegs;
    static const std::vector<xed_reg_enum_t> xmmRegs;
    static const std::vector<xed_reg_enum_t> ymmRegs;

private:
    std::vector<ThunkRequest> generateInstructions() const;
    ThunkRequest generateInstruction(OperandsMetadata const& operands) const;
    TestThunk compileThunk(ThunkRequest const& request) const;
    void* compileNativeThunk(ThunkRequest const& request) const;
    void* compileTranslatedThunk(ThunkRequest const& request) const;
};