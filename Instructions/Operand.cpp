#include "Operand.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-reg-enum.h"
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
        auto width = xed_decoded_inst_get_memory_displacement_width_bits(xedd, 0);

        auto mem_width = xed_decoded_inst_operand_length_bits(xedd, 0);
        xed_uint_t actual_mem_width = mem_width;

        auto displacement = xed_decoded_inst_get_memory_displacement(xedd, 0);

        if (mem_width == 256) {
            actual_mem_width = 128;
        }

        if (upper) {
            displacement += 16;
        }

        auto disp = xed_disp(
            displacement, 
            width
            );

        auto seg = xed_decoded_inst_get_seg_reg(xedd, 0);
        auto base = xed_decoded_inst_get_base_reg(xedd, 0);
        auto index = xed_decoded_inst_get_index_reg(xedd, 0);
        auto scale = xed_decoded_inst_get_scale(xedd, 0);

        auto bisd = xed_mem_gbisd(
            seg,
            base,
            index,
            scale,
            disp,
            actual_mem_width
        );
        return bisd;
    }
    
    // TODO: handle RSP offset
    // TODO: handle immediate values

    if (isYmm() || isXmm()) {
        auto xmmReg = toXmmReg();
        printf("xmmReg = %s\n", xed_reg_enum_t2str(xmmReg));
        return xed_reg(xmmReg);
    }

    return xed_reg(m_reg);
}