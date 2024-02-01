#pragma once

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
    const uint32_t opWidth;
    const xed_bits_t vl;
protected:
    const xed_inst_t *xi;
    const xed_encoder_request_t *xedd;
    std::vector<xed_encoder_request_t> internal_requests;
    std::vector<Operand> operands;

    Instruction(const xed_decoded_inst_t *xedd);

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
    void xorps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void insertps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void swap_in_upper_ymm(ymm_t *ymm);
    void swap_out_upper_ymm(ymm_t *ymm);
    void with_upper_ymm(ymm_t *ymm, std::function<void()> instr);
    public:
    virtual std::vector<xed_encoder_request_t> const& compile(ymm_t *ymm, CompilationStrategy compilationStrategy, uint64_t returnAddr = 0) = 0;
};
