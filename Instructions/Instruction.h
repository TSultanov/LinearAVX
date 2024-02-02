#pragma once

#include <unordered_set>
#ifdef __cplusplus
extern "C" {
#endif
#include <xed/xed-interface.h>
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-inst.h"
#include "xed/xed-reg-enum.h"
#ifdef __cplusplus
}
#endif

#include <functional>
#include <vector>
#include "Operand.h"
#include "../memmanager.h"

enum class CompilationStrategy {
    DirectCall,
    Inline,
    // ExceptionCall
};

class Instruction {
    const uint64_t rip;
    const uint64_t rsp;

    const std::vector<xed_reg_enum_t> gprs = {
        XED_REG_RAX, XED_REG_RBX, XED_REG_RCX, XED_REG_RDX, XED_REG_RSI, XED_REG_RDI, XED_REG_R8, XED_REG_R9,
        XED_REG_R10, XED_REG_R11, XED_REG_R12, XED_REG_R13, XED_REG_R14, XED_REG_R15
    };

    const uint32_t opWidth;
    const xed_bits_t vl;
    std::unordered_set<xed_reg_enum_t> usedRegs;
protected:
    const xed_inst_t *xi;
    const xed_encoder_request_t *xedd;
    std::vector<xed_encoder_request_t> internal_requests;
    std::vector<Operand> operands;

    Instruction(uint64_t rip, uint64_t rsp, const xed_decoded_inst_t *xedd);

    bool usesYmm() const;

    void push(xed_encoder_operand_t op0);
    void push(xed_reg_enum_t reg);
    void pop(xed_reg_enum_t reg);
    void ret();
    void mov(xed_reg_enum_t reg, uint64_t immediate);
    void movups(xed_reg_enum_t reg, xed_encoder_operand_t mem);
    void movups(xed_encoder_operand_t mem, xed_reg_enum_t reg);
    void movups(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movaps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movq(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void xorps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void insertps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void addps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void cvtss2sd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void swap_in_upper_ymm(ymm_t *ymm, bool force = false);
    void swap_out_upper_ymm(ymm_t *ymm, bool force = false);
    void with_upper_ymm(ymm_t *ymm, std::function<void()> instr);
    void zeroupperInternal(ymm_t * ymm, Operand const& op);

    bool usesRipAddressing() const;
    bool usesRspAddressing() const;

    xed_reg_enum_t getUnusedReg();
    void returnReg(xed_reg_enum_t reg);

    void withFreeReg(std::function<void(xed_reg_enum_t)> instr);
    void withRipSubstitution(std::function<void(std::function<xed_encoder_operand_t(xed_encoder_operand_t)>)> instr);
    public:
    virtual std::vector<xed_encoder_request_t> const& compile(ymm_t *ymm, CompilationStrategy compilationStrategy, uint64_t returnAddr = 0) = 0;
};
