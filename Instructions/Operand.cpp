#include "Operand.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-operand-enum.h"
#include "xed/xed-reg-enum.h"
#include <_types/_uint64_t.h>
#include <cstdio>
#include <vector>

bool Operand::isMemoryOperand() const {
    return op_name == XED_OPERAND_MEM0 || op_name == XED_OPERAND_MEM1;
}

bool Operand::isImmediate() const {
    return op_name == XED_OPERAND_IMM0 || op_name == XED_OPERAND_IMM1;
}

bool Operand::isXmm() const {
    return m_reg >= XED_REG_XMM0 && m_reg <= XED_REG_XMM16;
}

bool Operand::isYmm() const {
    return m_reg >= XED_REG_YMM0 && m_reg <= XED_REG_YMM16;
}

bool Operand::is32BitRegister() const {
    return m_reg >= XED_REG_EAX && m_reg <= XED_REG_EDI;
}

bool Operand::is64BitRegister() const {
    return m_reg >= XED_REG_RAX && m_reg <= XED_REG_R15;
}

xed_reg_enum_t Operand::to64BitRegister() const {
    if (is32BitRegister()) {
        return (xed_reg_enum_t)(m_reg - XED_REG_EAX + XED_REG_RAX);
    }
    return m_reg;
}

xed_reg_enum_t Operand::to32BitRegister() const {
    if (is64BitRegister()) {
        return (xed_reg_enum_t)(m_reg - XED_REG_RAX + XED_REG_EAX);
    }
    return m_reg;
}

xed_reg_enum_t Operand::toXmmReg() const {
    assert(isYmm() || isXmm());
    if (isYmm()) {
        return (xed_reg_enum_t)(m_reg - (XED_REG_YMM0 - XED_REG_XMM0));
    }
    return m_reg;
}

uint64_t Operand::immValue() const {
    return xed_decoded_inst_get_unsigned_immediate(xedd);
}

uint8_t Operand::imm8Value() const {
    return xed_decoded_inst_get_second_immediate(xedd);
}

bool Operand::hasRipBase() const {
    if (!isMemoryOperand()) {
        return false;
    }

    auto base = xed_decoded_inst_get_base_reg(xedd, 0);
    auto index = xed_decoded_inst_get_index_reg(xedd, 0);
    return base == XED_REG_RIP || index == XED_REG_RIP;
}

bool Operand::hasRspBase() const {
    if (!isMemoryOperand()) {
        return false;
    }

    auto base = xed_decoded_inst_get_base_reg(xedd, 0);
    auto index = xed_decoded_inst_get_index_reg(xedd, 0);
    return base == XED_REG_RSP || index == XED_REG_RSP;
}

std::vector<xed_reg_enum_t> Operand::getUsedReg() const {
    if (isMemoryOperand()) {
        auto baseReg = xed_decoded_inst_get_base_reg(xedd, 0);
        auto indexReg = xed_decoded_inst_get_index_reg(xedd, 0);
        return { baseReg, indexReg };
    }

    if (isImmediate()) {
        return {};
    }

    return { m_reg };
}

xed_encoder_operand_t translateBaseReg(xed_encoder_operand_t op, int64_t fromValue, xed_reg_enum_t to, int64_t toValue) {
    op.u.mem.base = to;
    op.u.mem.disp.displacement += fromValue - toValue;
    return op;
}

xed_encoder_operand_t Operand::toEncoderOperand(bool upper) const {
    if (isMemoryOperand()) {
        auto width = xed_decoded_inst_get_memory_displacement_width_bits(xedd, 0);

        // printf("width: %d\n", width);

        auto mem_width = xed_decoded_inst_operand_length_bits(xedd, index);
        xed_uint_t actual_mem_width = mem_width;

        auto displacement = xed_decoded_inst_get_memory_displacement(xedd, 0);

        if (mem_width == 256) {
            actual_mem_width = 128;
        }

        if (upper) {
            displacement += 16;
        }
        width = 32;

        // printf("mem_width: %d\n", mem_width);
        // printf("actual_mem_width: %d\n", actual_mem_width);
        // printf("displacement width = %d\n", width);
        // printf("displacement: %llx\n", displacement);

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
    if (isImmediate()) {
        switch(op_name) {
            case XED_OPERAND_IMM0:
            {
                auto width = xed_decoded_inst_get_immediate_width_bits(xedd);
                auto value = xed_decoded_inst_get_unsigned_immediate(xedd);
                return xed_imm0(value, width);
            }
            case XED_OPERAND_IMM1:
            {
                auto value = xed_decoded_inst_get_second_immediate(xedd);
                return xed_imm1(value);
            }
            default:
                break;

        }
    }

    if (isYmm() || isXmm()) {
        auto xmmReg = toXmmReg();
        // printf("xxmReg = %s\n", xed_reg_enum_t2str(xmmReg));
        return xed_reg(xmmReg);
    }

    return xed_reg(m_reg);
}