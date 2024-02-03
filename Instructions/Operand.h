#pragma once

#include <vector>
#ifdef __cplusplus
extern "C" {
#endif
#include <xed/xed-interface.h>
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-error-enum.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-iform-enum.h"
#include "xed/xed-inst.h"
#include "xed/xed-operand-enum.h"
#include "xed/xed-reg-enum.h"
#ifdef __cplusplus
}
#endif
#include <cassert>

class Operand {
    const xed_decoded_inst_t *xedd;
    const xed_inst_t *xi;
    const xed_operand_t *op;
    const xed_operand_enum_t op_name;
    const xed_reg_enum_t m_reg;
    const uint32_t index;
public:
    Operand(const xed_decoded_inst_t *xedd, uint32_t i)
        :xedd(xedd)
        ,xi(xed_decoded_inst_inst(xedd))
        ,op(xed_inst_operand(xi, i))
        ,op_name(xed_operand_name(op))
        ,m_reg(xed_decoded_inst_get_reg(xedd, op_name))
        ,index(i)
    {}

    bool isMemoryOperand() const;
    bool isImmediate() const;
    uint64_t immValue() const;
    uint8_t imm8Value() const;
    xed_reg_enum_t reg() const { return m_reg; }
    bool isXmm() const;
    bool isYmm() const;
    xed_reg_enum_t toXmmReg() const;
    xed_encoder_operand_t toEncoderOperand(bool upper) const;
    bool hasRipBase() const;
    bool hasRspBase() const;
    std::vector<xed_reg_enum_t> getUsedReg() const;
};
