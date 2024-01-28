#pragma once

#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-inst.h"
#include <cassert>

class Operand {
    const xed_decoded_inst_t *xedd;
    const xed_inst_t *xi;
    const xed_operand_t *op;
    const xed_operand_enum_t op_name;
    const xed_reg_enum_t m_reg;
public:
    Operand(const xed_decoded_inst_t *xedd, uint32_t i)
        :xedd(xedd)
        ,xi(xed_decoded_inst_inst(xedd))
        ,op(xed_inst_operand(xi, i))
        ,op_name(xed_operand_name(op))
        ,m_reg(xed_decoded_inst_get_reg(xedd, op_name))
    {}

    bool isMemoryOperand() const;
    xed_reg_enum_t reg() const { return m_reg; }
    bool isXmm() const;
    bool isYmm() const;
    xed_reg_enum_t toXmmReg() const;
    xed_encoder_operand_t toEncoderOperand(bool upper) const;
};
