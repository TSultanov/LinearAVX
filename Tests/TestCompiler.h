#pragma once
#include <unordered_set>
#include <vector>
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

class TestCompiler {
    InstructionMetadata const& metadata;
public:
    TestCompiler(InstructionMetadata const& metadata);

    std::vector<std::shared_ptr<ThunkRequest>> generateInstructions() const;
private:
    const std::vector<xed_reg_enum_t> gpRegs = {
        XED_REG_RAX, XED_REG_RBX, XED_REG_RCX, XED_REG_RDX, XED_REG_RSI, XED_REG_RDI, XED_REG_R8, XED_REG_R9,
        XED_REG_R10, XED_REG_R11, XED_REG_R12, XED_REG_R13, XED_REG_R14, XED_REG_R15
    };

    const std::vector<xed_reg_enum_t> xmmRegs = {
        XED_REG_XMM0, XED_REG_XMM1, XED_REG_XMM2, XED_REG_XMM3, XED_REG_XMM4, XED_REG_XMM5, XED_REG_XMM6, XED_REG_XMM7,
        XED_REG_XMM8, XED_REG_XMM9, XED_REG_XMM10, XED_REG_XMM11, XED_REG_XMM12, XED_REG_XMM13, XED_REG_XMM14,
        XED_REG_XMM15
    };

    const std::vector<xed_reg_enum_t> ymmRegs = {
        XED_REG_YMM0, XED_REG_YMM1, XED_REG_YMM2, XED_REG_YMM3, XED_REG_YMM4, XED_REG_YMM5, XED_REG_YMM6, XED_REG_YMM7,
        XED_REG_YMM8, XED_REG_YMM9, XED_REG_YMM10, XED_REG_YMM11, XED_REG_YMM12, XED_REG_YMM13, XED_REG_YMM14,
        XED_REG_YMM15
    };

    std::shared_ptr<ThunkRequest> generateInstruction(OperandsMetadata const& operands) const;
};