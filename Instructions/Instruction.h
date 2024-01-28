#pragma once

#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-inst.h"
#include <functional>
#include <vector>

#include "Operand.h"
#include "xed/xed-reg-enum.h"
#include "../memmanager.h"

class Instruction {
    const xed_inst_t *xi;

    std::vector<Operand> operands;

    std::vector<xed_encoder_request_t> internal_requests;
protected:
    Instruction(const xed_decoded_inst_t *xedd)
    :xi(xed_decoded_inst_inst(xedd))
    {
        auto n_operands = xed_inst_noperands(xi);
        for (uint32_t i = 0; i < n_operands; i++) {
            operands.emplace_back(Operand(xedd, i));
        }
    }

    bool usesYmm() const;

    void push(xed_reg_enum_t reg);
    void pop(xed_reg_enum_t reg);
    void mov(xed_reg_enum_t reg, uint64_t immediate);
    void movups(xed_reg_enum_t reg, xed_encoder_operand_t mem);
    void movups(xed_encoder_operand_t mem, xed_reg_enum_t reg);
    void movups(xed_encoder_operand_t mem0, xed_encoder_operand_t mem1);
    void swap_in_upper_ymm(ymm_t *ymm);
    void swap_out_upper_ymm(ymm_t *ymm);
    void with_upper_ymm(ymm_t *ymm, std::function<void()> instr);
    public:
    virtual void compile(ymm_t *ymm);
};
