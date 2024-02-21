#include "Instruction.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-iform-enum.h"
#include "xed/xed-operand-enum.h"
#include "xed/xed-reg-enum.h"
#include <cstdio>
#include <unistd.h>
#include <unordered_map>

const xed_state_t dstate = {.mmode = XED_MACHINE_MODE_LONG_64,
                            .stack_addr_width = XED_ADDRESS_WIDTH_64b};

Instruction::Instruction(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd)
:opWidth(xed_decoded_inst_get_operand_width(&xedd))
,vl(xed3_operand_get_vl(&xedd))
,rip(rip)
,ilen(ilen)
,xi(xed_decoded_inst_inst(&xedd))
,xedd(xedd)
{
    auto n_operands = xed_inst_noperands(xi);
    for (uint32_t i = 0; i < n_operands; i++) {
        operands.emplace_back(Operand(&this->xedd, i));
    }

    for (auto const& op : operands) {
        for (auto reg : op.getUsedReg()) {
            usedRegs.insert(reg);
        }
    }
}

xed_encoder_operand_t substRip(xed_encoder_operand_t op, xed_reg_enum_t ripSubstReg) {
    if (op.type == XED_ENCODER_OPERAND_TYPE_MEM) {
        if (op.u.mem.base == XED_REG_RIP) {
            op.u.mem.base = ripSubstReg;
        }
        if (op.u.mem.index == XED_REG_RIP) {
            op.u.mem.index = ripSubstReg;
        }
        return op;
    }

    return op;
}

xed_encoder_operand_t offsetRsp(xed_encoder_operand_t op, int64_t offset) {
    if (op.type == XED_ENCODER_OPERAND_TYPE_MEM) {
        if (op.u.mem.base == XED_REG_RSP) {
            op.u.mem.disp.displacement -= offset;
        }
        if (op.u.mem.index == XED_REG_RSP) {
            debug_print("RSP index not supported\n");
            exit(1);
        }
    }

    return op;
}

void Instruction::push(xed_encoder_operand_t op) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst1(&enc_inst, dstate, XED_ICLASS_PUSH, 64, op);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
    rspOffset -= pointerWidthBytes;
}

void Instruction::push(xed_reg_enum_t reg) {
    push(xed_reg(reg));
}

void Instruction::ret() {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;
    xed_inst0(&enc_inst, dstate, XED_ICLASS_RET_NEAR, 64);
    xed_convert_to_encoder_request(&req, &enc_inst);
    
    internal_requests.push_back(req);
}

void Instruction::pop(xed_reg_enum_t reg) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst1(&enc_inst, dstate, XED_ICLASS_POP, 64, xed_reg(reg));
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
    rspOffset += pointerWidthBytes;
}

void Instruction::shl(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_SHL, opWidth, op0, op1);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::shr(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_SHR, opWidth, op0, op1);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::and_i(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_AND, opWidth, op0, op1);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::not_i(xed_encoder_operand_t op0) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst1(&enc_inst, dstate, XED_ICLASS_NOT, opWidth, op0);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::mov(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_MOV, 64, op0, op1);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::mov(xed_reg_enum_t reg, uint64_t immediate) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_MOV, 64, xed_reg(reg), xed_imm0(immediate, 64));
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::op1(xed_iclass_enum_t instr, xed_encoder_operand_t op0) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst1(&enc_inst, dstate, instr, opWidth, subst(op0));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::op2(xed_iclass_enum_t instr, xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, instr, opWidth, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::op2_raw(xed_iclass_enum_t instr, xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, instr, opWidth, op0, op1);
    xed_convert_to_encoder_request(&req, &enc_inst);
    xed3_operand_set_vl(&req, vl);

    internal_requests.push_back(req);
}

void Instruction::op3(xed_iclass_enum_t instr, xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst3(&enc_inst, dstate, instr, opWidth, subst(op0), subst(op1), subst(op2));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::movups(xed_reg_enum_t reg, xed_encoder_operand_t mem) {
    movups(xed_reg(reg), mem);
}

void Instruction::movups(xed_encoder_operand_t mem, xed_reg_enum_t reg) {
    movups(mem, xed_reg(reg));
}

void Instruction::movups(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVUPS, op0, op1);
}

void Instruction::movupd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVUPD, op0, op1);
}

void Instruction::movups_raw(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2_raw(XED_ICLASS_MOVUPS, op0, op1);
}

void Instruction::movaps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVAPS, op0, op1);
}

void Instruction::movapd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVAPD, op0, op1);
}

void Instruction::movss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVSS, op0, op1);
}

void Instruction::movsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVSD_XMM, op0, op1);
}

void Instruction::xorps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_XORPS, op0, op1);
}

void Instruction::xorps_raw(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2_raw(XED_ICLASS_XORPS, op0, op1);
}

void Instruction::xorpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_XORPD, op0, op1);
}

void Instruction::insertps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_INSERTPS, op0, op1, op2);
}

void Instruction::addps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_ADDPS, op0, op1);
}

void Instruction::addpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_ADDPD, op0, op1);
}

void Instruction::subps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_SUBPS, op0, op1);
}

void Instruction::subpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_SUBPD, op0, op1);
}

void Instruction::divsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_DIVSD, op0, op1);
}

void Instruction::divss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_DIVSS, op0, op1);
}

void Instruction::mulsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MULSD, op0, op1);
}

void Instruction::mulss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MULSS, op0, op1);
}

void Instruction::mulpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MULPD, op0, op1);
}

void Instruction::mulps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MULPS, op0, op1);
}

void Instruction::pand(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_PAND, op0, op1);
}

void Instruction::por(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_POR, op0, op1);
}

void Instruction::psrld(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_PSRLD, op0, op1);
}

void Instruction::cvtss2sd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_CVTSS2SD, op0, op1);
}

void Instruction::cvtsi2sd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_CVTSI2SD, op0, op1);
}

void Instruction::cvtsi2ss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_CVTSI2SS, op0, op1);
}

void Instruction::cvtsd2ss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_CVTSD2SS, op0, op1);
}

void Instruction::movq(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVQ, op0, op1);
}

void Instruction::movdqu(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVDQU, op0, op1);
}

void Instruction::movdqu_raw(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2_raw(XED_ICLASS_MOVDQU, op0, op1);
}

void Instruction::movdqa(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVDQA, op0, op1);
}

void Instruction::shufps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_SHUFPS, op0, op1, op2);
}

void Instruction::shufpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_SHUFPD, op0, op1, op2);
}

void Instruction::pshufhw(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_PSHUFHW, op0, op1, op2);
}

void Instruction::pshuflw(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_PSHUFLW, op0, op1, op2);
}

void Instruction::pextrw(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_PEXTRW_SSE4, op0, op1, op2);
}

void Instruction::pextrq(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_PEXTRQ, op0, op1, op2);
}

void Instruction::pextrd(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_PEXTRD, op0, op1, op2);
}

void Instruction::pextrb(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_PEXTRB, op0, op1, op2);
}

void Instruction::extractps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_EXTRACTPS, op0, op1, op2);
}

void Instruction::pxor(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_PXOR, op0, op1);
}

void Instruction::unpckhps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_UNPCKHPS, op0, op1);
}

void Instruction::unpcklps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_UNPCKLPS, op0, op1);
}

void Instruction::pcmpeqq(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_PCMPEQQ, op0, op1);
}

void Instruction::blendvpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_BLENDVPD, op0, op1);
}

void Instruction::cvttss2si(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_CVTTSS2SI, op0, op1);
}

void Instruction::cvttsd2si(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_CVTTSD2SI, op0, op1);
}

void Instruction::andpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_ANDPD, op0, op1);
}

void Instruction::andps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_ANDPS, op0, op1);
}

void Instruction::andnpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_ANDNPD, op0, op1);
}

void Instruction::andnps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_ANDNPS, op0, op1);
}

void Instruction::psllq(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_PSLLQ, op0, op1);
}

void Instruction::movmskps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVMSKPS, op0, op1);
}

void Instruction::movmskpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVMSKPD, op0, op1);
}

void Instruction::movlhps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MOVLHPS, op0, op1);
}

void Instruction::ucomiss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_UCOMISS, op0, op1);
}

void Instruction::ucomisd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_UCOMISD, op0, op1);
}

void Instruction::sqrtps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_SQRTPS, op0, op1);
}

void Instruction::sqrtpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_SQRTPD, op0, op1);
}

void Instruction::rsqrtps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_RSQRTPS, op0, op1);
}

void Instruction::rsqrtss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_RSQRTSS, op0, op1);
}

void Instruction::haddps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_HADDPS, op0, op1);
}

void Instruction::haddpd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_HADDPD, op0, op1);
}

void Instruction::maxss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MAXSS, op0, op1);
}

void Instruction::maxsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MAXSD, op0, op1);
}

void Instruction::minss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MINSS, op0, op1);
}

void Instruction::minsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_MINSD, op0, op1);
}

void Instruction::comiss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_COMISS, op0, op1);
}

void Instruction::comisd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    op2(XED_ICLASS_COMISD, op0, op1);
}

void Instruction::cmppd(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    op3(XED_ICLASS_CMPPD, op0, op1, op2);
}

void Instruction::call(xed_encoder_operand_t op) {
    xed_encoder_request_t req;
    // xed_encoder_instruction_t enc_inst;

    // xed_inst1(&enc_inst, dstate, XED_ICLASS_CALL_NEAR, 0, op);
    // xed_convert_to_encoder_request(&req, &enc_inst);
    // xed3_operand_set_vl(&req, vl);

    xed_encoder_request_zero_set_mode(&req, &dstate);
    xed_encoder_request_set_iclass(&req, XED_ICLASS_CALL_NEAR);
    xed_encoder_request_set_operand_order(&req, 0, XED_OPERAND_REG0);
    xed_encoder_request_set_reg(&req, XED_OPERAND_REG0, op.u.reg);
    xed_encoder_request_set_effective_address_size(&req, 64);
    xed_encoder_request_set_effective_operand_width(&req, 64);


    internal_requests.push_back(req);
}

void Instruction::swap_in_upper_ymm(std::unordered_set<xed_reg_enum_t> registers) {
    void* getYmmAddr = (void*)&get_ymm_storage;
    withReg(XED_REG_RBX, [=]() {
        mov(XED_REG_RBX, (uint64_t)getYmmAddr);
        withReg(XED_REG_RAX, [=]() {
            call(xed_reg(XED_REG_RBX));
            // RAX now will contain the ymm pointer

            std::unordered_set<xed_reg_enum_t> usedRegs;

            for (auto reg : registers) {
                if (usedRegs.contains(reg)) {
                    continue;
                }
                usedRegs.insert(reg);
                uint32_t regnum = reg - XED_REG_XMM0;

                auto disp = xed_disp(regnum*sizeof(__m128), 32);
                movups_raw(xed_mem_bd(XED_REG_RAX, disp, 128), xed_reg(reg));

                disp = xed_disp((regnum + 16)*sizeof(__m128), 32);
                movups_raw(xed_reg(reg), xed_mem_bd(XED_REG_RAX, disp, 128));
            }
        });
    });
}

void Instruction::swap_out_upper_ymm(std::unordered_set<xed_reg_enum_t> registers) {
    void* getYmmAddr = (void*)&get_ymm_storage;
    withReg(XED_REG_RBX, [=]() {
        mov(XED_REG_RBX, (uint64_t)getYmmAddr);
        withReg(XED_REG_RAX, [=]() {
            call(xed_reg(XED_REG_RBX));
            // RAX now will contain the ymm pointer

            std::unordered_set<xed_reg_enum_t> usedRegs;

            for (auto reg : registers) {
                if (usedRegs.contains(reg)) {
                    continue;
                }
                usedRegs.insert(reg);
                uint32_t regnum = reg - XED_REG_XMM0;

                auto disp = xed_disp((regnum+16)*sizeof(__m128), 32);
                movups_raw(xed_mem_bd(XED_REG_RAX, disp, 128), xed_reg(reg));

                disp = xed_disp(regnum*sizeof(__m128), 32);
                movups_raw(xed_reg(reg), xed_mem_bd(XED_REG_RAX, disp, 128));
            }
        });
    });
}

void Instruction::swap_in_upper_ymm(bool force) {
    void* getYmmAddr = (void*)&get_ymm_storage;
    withReg(XED_REG_RBX, [&]() {
        mov(XED_REG_RBX, (uint64_t)getYmmAddr);
        withReg(XED_REG_RAX, [&]() {
            call(xed_reg(XED_REG_RBX));
            // RAX now will contain the ymm pointer

            std::unordered_set<xed_reg_enum_t> usedRegs;

            for (auto& op : operands) {
                if (op.isYmm() || (op.isXmm() && force)) {
                    auto reg = op.toXmmReg();
                    if (usedRegs.contains(reg)) {
                        continue;
                    }
                    usedRegs.insert(reg);
                    uint32_t regnum = reg - XED_REG_XMM0;

                    auto disp = xed_disp(regnum*sizeof(__m128), 32);
                    movups_raw(xed_mem_bd(XED_REG_RAX, disp, 128), xed_reg(reg));

                    disp = xed_disp((regnum + 16)*sizeof(__m128), 32);
                    movups_raw(xed_reg(reg), xed_mem_bd(XED_REG_RAX, disp, 128));
                }
            }
        });
    });
}

void Instruction::swap_out_upper_ymm(bool force) {
    void* getYmmAddr = (void*)&get_ymm_storage;
    withReg(XED_REG_RBX, [=]() {
        mov(XED_REG_RBX, (uint64_t)getYmmAddr);
        withReg(XED_REG_RAX, [=]() {
            call(xed_reg(XED_REG_RBX));
            // RAX now will contain the ymm pointer

            std::unordered_set<xed_reg_enum_t> usedRegs;

            for (auto& op : operands) {
                if (op.isYmm() || (op.isXmm() && force)) {
                    auto reg = op.toXmmReg();
                    if (usedRegs.contains(reg)) {
                        continue;
                    }
                    usedRegs.insert(reg);
                    uint32_t regnum = reg - XED_REG_XMM0;

                    auto disp = xed_disp((regnum+16)*sizeof(__m128), 32);
                    movups_raw(xed_mem_bd(XED_REG_RAX, disp, 128), xed_reg(reg));

                    disp = xed_disp(regnum*sizeof(__m128), 32);
                    movups_raw(xed_reg(reg), xed_mem_bd(XED_REG_RAX, disp, 128));
                }
            }
        });
    });
}

void Instruction::with_upper_ymm(std::function<void()> instr) {
    swap_in_upper_ymm();
    instr();
    swap_out_upper_ymm();
}

bool Instruction::usesYmm() const {
    for(auto const& op : operands) {
        if (op.isYmm()) {
            return true;
        }
    }
    return false;
}

void Instruction::zeroupperInternal(Operand const& op) {
    void* getYmmAddr = (void*)&get_ymm_storage;
    withReg(XED_REG_RBX, [=]() {
        mov(XED_REG_RBX, (uint64_t)getYmmAddr);
        withReg(XED_REG_RAX, [=]() {
            call(xed_reg(XED_REG_RBX));
            // RAX now will contain the ymm pointer

            auto reg = op.toXmmReg();
            uint32_t regnum = reg - XED_REG_XMM0;

            // swap in the reg
            auto disp = xed_disp(regnum*sizeof(__m128), 32);
            movups_raw(xed_mem_bd(XED_REG_RAX, disp, 128), xed_reg(reg));

            disp = xed_disp((regnum + 16)*sizeof(__m128), 32);
            movups_raw(xed_reg(reg), xed_mem_bd(XED_REG_RAX, disp, 128));

            xorps_raw(xed_reg(reg), xed_reg(reg));

            // swap it out
            disp = xed_disp((regnum + 16)*sizeof(__m128), 32);
            movups_raw(xed_mem_bd(XED_REG_RAX, disp, 128), xed_reg(reg));

            disp = xed_disp(regnum*sizeof(__m128), 32);
            movups_raw(xed_reg(reg), xed_mem_bd(XED_REG_RAX, disp, 128));
        });
    });
}

bool Instruction::usesRipAddressing() const {
    for (auto const& op : operands) {
        if (op.hasRipBase()) {
            return true;
        }
    }
    return false;
}

bool Instruction::usesRspAddressing() const {
    for (auto const& op : operands) {
        if (op.hasRspBase()) {
            return true;
        }
    }
    return false;
}

xed_reg_enum_t Instruction::getUnusedReg() {
    for (auto reg : gprs) {
        if (usedRegs.count(reg) == 0) {
            usedRegs.insert(reg);
            return reg;
        }
    }

    return XED_REG_INVALID;
}

xed_reg_enum_t xmmToYmm(xed_reg_enum_t reg) {
    if (reg >= XED_REG_YMM0 && reg <= XED_REG_YMM15) {
        return reg;
    }
    return (xed_reg_enum_t)(reg - XED_REG_XMM0 + XED_REG_YMM0);
}

xed_reg_enum_t Instruction::getUnusedXmmReg() {
    for (auto reg : xmmRegs) {
        if (!usedRegs.contains(reg) && !usedRegs.contains(xmmToYmm(reg))) {
            usedRegs.insert(reg);
            return reg;
        }
    }

    return XED_REG_INVALID;
}

void Instruction::returnReg(xed_reg_enum_t reg) {
    usedRegs.erase(reg);
}

void Instruction::withFreeReg(std::function<void(xed_reg_enum_t)> instr) {
    auto reg = getUnusedReg();
    push(reg);
    instr(reg);
    pop(reg);
    returnReg(reg);
}

void Instruction::withReg(xed_reg_enum_t reg, std::function<void()> instr) {
    usedRegs.insert(reg);
    push(reg);
    instr();
    pop(reg);
    returnReg(reg);
}

void Instruction::withRipSubstitution(std::function<void(std::function<xed_encoder_operand_t(xed_encoder_operand_t subst)>)> instr) {
    // TODO: do not replace RIP if we are compiling inline
    if (usesRipAddressing()) {
        withFreeReg([=](xed_reg_enum_t tempReg) {
            mov(tempReg, rip+ilen);

            instr([=](xed_encoder_operand_t op) { return substRip(op, tempReg); });
        });
    } else if (usesRspAddressing()) {
        instr([=](xed_encoder_operand_t op) { return offsetRsp(op, rspOffset); });
    } else
    if (usesRipAddressing() && usesRspAddressing()) {
        withFreeReg([=](xed_reg_enum_t tempRipReg) {
            mov(tempRipReg, rip+ilen);
            instr([=](xed_encoder_operand_t op) {
                if (op.type == XED_ENCODER_OPERAND_TYPE_MEM) {
                    if (op.u.mem.base == XED_REG_RSP || op.u.mem.base == XED_REG_ESP) {
                        return offsetRsp(substRip(op, tempRipReg), rspOffset);
                    } else {
                        return substRip(op, tempRipReg);
                    }
                }
                return op;
            });
        });
    } else {
        instr([](xed_encoder_operand_t op) { return op; });
    }
}

void Instruction::sub(xed_reg_enum_t reg, int8_t immediate) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_SUB, opWidth, xed_reg(reg), xed_imm0(immediate, 8));
    xed_convert_to_encoder_request(&req, &enc_inst);
    xed_encoder_request_set_effective_operand_width(&req, 64);

    internal_requests.push_back(req);

    if (reg == XED_REG_RSP || reg == XED_REG_ESP) {
        rspOffset -= immediate;
    }
}

void Instruction::add(xed_reg_enum_t reg, int8_t immediate) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_ADD, opWidth, xed_reg(reg), xed_imm0(immediate, 8));
    xed_convert_to_encoder_request(&req, &enc_inst);
    xed_encoder_request_set_effective_operand_width(&req, 64);

    internal_requests.push_back(req);

    if (reg == XED_REG_RSP || reg == XED_REG_ESP) {
        rspOffset += immediate;
    }
}

void Instruction::withPreserveXmmReg(Operand const& op, std::function<void()> instr) {
    withPreserveXmmReg(op.toXmmReg(), instr);
}

void Instruction::withPreserveXmmReg(xed_reg_enum_t reg, std::function<void()> instr) {
    sub(XED_REG_RSP, 16);
    movdqu_raw(xed_mem_b(XED_REG_RSP, 128), xed_reg(reg));

    instr();

    movdqu_raw(xed_reg(reg), xed_mem_b(XED_REG_RSP, 128));
    add(XED_REG_RSP, 16);
}

xed_iform_enum_t Instruction::getIform() const {
    return xed_decoded_inst_get_iform_enum(&xedd);
}