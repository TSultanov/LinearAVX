#pragma once

#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-inst.h"
#include <cassert>

class Operand {
    const xed_inst_t *xi;
    const xed_operand_t *op;
    const xed_operand_enum_t op_name;
    const xed_reg_enum_t m_reg;
public:
    Operand(const xed_decoded_inst_t *xedd, uint32_t i)
        :xi(xed_decoded_inst_inst(xedd))
        ,op(xed_inst_operand(xi, i))
        ,op_name(xed_operand_name(op))
        ,m_reg(xed_decoded_inst_get_reg(xedd, op_name))
    {}

    const bool isMemoryOperand() {
        return op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1;
    }

    xed_reg_enum_t reg() const { return m_reg; }

    bool isXmm() const {
        return m_reg >= XED_REG_XMM0 && m_reg <= XED_REG_XMM16;
    }

    bool isYmm() const {
        return m_reg >= XED_REG_YMM0 && m_reg <= XED_REG_YMM0;
    }

    xed_reg_enum_t toXmmReg() const {
        assert(isYmm() || isXmm());
        if (isYmm()) {
            return (xed_reg_enum_t)(m_reg - (XED_REG_YMM0 - XED_REG_XMM0));
        }
        return m_reg;
    }
};
