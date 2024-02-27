#pragma once

#include <unordered_set>
#ifdef __cplusplus
extern "C" {
#endif
#include "xed/xed-iform-enum.h"
#include "xed/xed-iclass-enum.h"
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
#include "../utils.h"

enum class CompilationStrategy {
    DirectCall,
    DirectCallPopRax,
    Inline,
    FarJump
};

class Instruction {
    const std::vector<xed_reg_enum_t> gprs = {
        XED_REG_RAX, XED_REG_RBX, XED_REG_RCX, XED_REG_RDX, XED_REG_RSI, XED_REG_RDI, XED_REG_R8, XED_REG_R9,
        XED_REG_R10, XED_REG_R11, XED_REG_R12, XED_REG_R13, XED_REG_R14, XED_REG_R15
    };

    const std::vector<xed_reg_enum_t> xmmRegs = {
        XED_REG_XMM0, XED_REG_XMM1, XED_REG_XMM2, XED_REG_XMM3, XED_REG_XMM4, XED_REG_XMM5, XED_REG_XMM6, XED_REG_XMM7,
        XED_REG_XMM8, XED_REG_XMM9, XED_REG_XMM10, XED_REG_XMM11, XED_REG_XMM12, XED_REG_XMM13, XED_REG_XMM14,
        XED_REG_XMM15
    };

    const uint64_t pointerWidthBytes = 8;


protected:
    const uint32_t opWidth;
    const xed_bits_t vl;
    std::unordered_set<xed_reg_enum_t> usedRegs;
    int64_t rspOffset = 0;
    const uint64_t rip;
    const uint8_t ilen;

    const xed_inst_t *xi;
    const xed_decoded_inst_t xedd;
    std::vector<xed_encoder_request_t> internal_requests;
    std::vector<Operand> operands;

    Instruction(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd);
    virtual ~Instruction() = default;

    bool usesYmm() const;

    void push(xed_encoder_operand_t op0);
    void push(xed_reg_enum_t reg);
    void pop(xed_reg_enum_t reg);
    void popf();
    void pushf();
    void ret();
    void sub(xed_reg_enum_t reg, int8_t immediate);
    void add(xed_reg_enum_t reg, int8_t immediate);
    void mov(xed_reg_enum_t reg, uint64_t immediate);
    void mov(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movups(xed_reg_enum_t reg, xed_encoder_operand_t mem);
    void movups(xed_encoder_operand_t mem, xed_reg_enum_t reg);
    void movups(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movupd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movups_raw(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movaps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movapd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movq(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movdqu(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movdqu_raw(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movdqa(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void xorps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void xorps_raw(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void xorpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void insertps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void addps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void addpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void subps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void subpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void divsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void divss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void addss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void addsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void mulsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void mulss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void mulpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void mulps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void paddb(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void paddw(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void paddd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void paddq(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void maxss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void maxsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void minss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void minsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void pinsrb(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2);
    void pinsrd(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2);
    void pinsrq(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2);
    void comiss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void comisd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void pand(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void por(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void psrld(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void cvtss2sd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void cvtsi2sd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void cvtsi2ss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void cvtsd2ss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void shufps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void shufpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void pshufhw(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void pshuflw(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void pextrw(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void pextrq(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void pextrd(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void pextrb(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void extractps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3);
    void pxor(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void unpckhps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void unpcklps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void pcmpeqq(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void pcmpeqd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void pcmpeqw(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void pcmpeqb(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void psignb(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void psignw(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void psignd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void pmovmskb(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void blendvpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void cvttss2si(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void cvttsd2si(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void andpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void andps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void andnpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void andnps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void psllq(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void psrldq(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void cmppd(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2);
    void movmskps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movmskpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movlhps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void ucomiss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void ucomisd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void rsqrtps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void rsqrtss(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void sqrtps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void sqrtpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void haddps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void haddpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void pshufb(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movhpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void movhps(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void shl(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void shr(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void sar(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void and_i(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void or_i(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void test_i(xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void not_i(xed_encoder_operand_t op0);
    void call(xed_encoder_operand_t op);

    void op3(xed_iclass_enum_t instr, xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2);
    void op2(xed_iclass_enum_t instr, xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void op2_raw(xed_iclass_enum_t instr, xed_encoder_operand_t op0, xed_encoder_operand_t op1);
    void op1(xed_iclass_enum_t instr, xed_encoder_operand_t op0);

    void swap_in_upper_ymm(std::unordered_set<xed_reg_enum_t> registers);
    void swap_out_upper_ymm(std::unordered_set<xed_reg_enum_t> registers);
    void swap_in_upper_ymm(bool force = false);
    void swap_out_upper_ymm(bool force = false);
    void with_upper_ymm(std::function<void()> instr);
    void zeroupperInternal(Operand const& op);

    bool usesRipAddressing() const;
    bool usesRspAddressing() const;

    xed_reg_enum_t getUnusedReg();
    xed_reg_enum_t getUnusedXmmReg();
    void returnReg(xed_reg_enum_t reg);

    void withFreeReg(std::function<void(xed_reg_enum_t)> instr);
    void withReg(xed_reg_enum_t reg, std::function<void()> instr);
    void withRipSubstitution(std::function<void(std::function<xed_encoder_operand_t(xed_encoder_operand_t)>)> instr);

    void withPreserveXmmReg(Operand const& op, std::function<void()> instr);
    void withPreserveXmmReg(xed_reg_enum_t reg, std::function<void()> instr);
    public:
    virtual std::vector<xed_encoder_request_t> const& compile(CompilationStrategy compilationStrategy, uint64_t returnAddr = 0) = 0;
    xed_iform_enum_t getIform() const;

    const xed_decoded_inst_t* getDecodedInstr() const { return &xedd; }
};
