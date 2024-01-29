#include "Operand.h"
#include <cstdio>

bool Operand::isMemoryOperand() const {
    return op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1;
}

bool Operand::isXmm() const {
    return m_reg >= XED_REG_XMM0 && m_reg <= XED_REG_XMM16;
}

bool Operand::isYmm() const {
    return m_reg >= XED_REG_YMM0 && m_reg <= XED_REG_YMM0;
}

xed_reg_enum_t Operand::toXmmReg() const {
    assert(isYmm() || isXmm());
    if (isYmm()) {
        return (xed_reg_enum_t)(m_reg - (XED_REG_YMM0 - XED_REG_XMM0));
    }
    return m_reg;
}

xed_encoder_operand_t Operand::toEncoderOperand(bool upper) const {
    if (isMemoryOperand()) {
        auto width = xed_decoded_inst_get_memory_displacement_width(xedd, 0) * 8;
        xed_uint_t actual_width = width;

        auto displacement = xed_decoded_inst_get_memory_displacement(xedd, 0);

        if (width == 256) {
            actual_width = 128;
            if (upper) {
                displacement += actual_width/8;
            }
        }

        auto disp = xed_disp(
            displacement, 
            actual_width);

        auto bisd = xed_mem_bisd(
            xed_decoded_inst_get_base_reg(xedd, 0),
            xed_decoded_inst_get_index_reg(xedd, 0),
            xed_decoded_inst_get_scale(xedd, 0),
            disp,
            actual_width // TODO is it the correct value?
        );
        return bisd;
    }
    
    // TODO: handle RSP offset
    // TODO: handle immediate values

    if (isYmm() || isXmm()) {
        return xed_reg(toXmmReg());
    }

    return xed_reg(m_reg);
}