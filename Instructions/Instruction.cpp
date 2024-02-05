#include "Instruction.h"
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-reg-enum.h"
#include <cstdio>
#include <unistd.h>
#include <unordered_map>

const xed_state_t dstate = {.mmode = XED_MACHINE_MODE_LONG_64,
                            .stack_addr_width = XED_ADDRESS_WIDTH_64b};

Instruction::Instruction(uint64_t rip, const xed_decoded_inst_t *xedd)
:xi(xed_decoded_inst_inst(xedd))
,opWidth(xed_decoded_inst_get_operand_width(xedd))
,vl(xed3_operand_get_vl(xedd))
,xedd(xedd)
,rip(rip)
{
    auto n_operands = xed_inst_noperands(xi);
    for (uint32_t i = 0; i < n_operands; i++) {
        operands.emplace_back(Operand(xedd, i));
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
            printf("RSP index not supported\n");
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

void Instruction::mov(xed_reg_enum_t reg, uint64_t immediate) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_MOV, 64, xed_reg(reg), xed_imm0(immediate, 64));
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::movups(xed_reg_enum_t reg, xed_encoder_operand_t mem) {
    movups(xed_reg(reg), mem);
}

void Instruction::movups(xed_encoder_operand_t mem, xed_reg_enum_t reg) {
    movups(mem, xed_reg(reg));
}

void Instruction::movups(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        // printf("opWidth = %d\n", opWidth);
        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVUPS, opWidth, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::movaps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVAPS, 0, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::movss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVSS, opWidth, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::movsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVSD_XMM, opWidth, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::xorps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_XORPS, 0, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::swap_in_upper_ymm(ymm_t *ymm, bool force) {
    withFreeReg([=] (xed_reg_enum_t tempReg) {
        std::unordered_set<xed_reg_enum_t> usedRegs;

        for (auto& op : operands) {
            if (op.isYmm() || (op.isXmm() && force)) {
                auto reg = op.toXmmReg();
                if (usedRegs.contains(reg)) {
                    continue;
                }
                usedRegs.insert(reg);
                uint32_t regnum = reg - XED_REG_XMM0;

                mov(tempReg, (uint64_t)(&(ymm->l[regnum])));
                movups(xed_mem_b(tempReg, 128), reg);

                mov(tempReg, (uint64_t)(&(ymm->u[regnum])));
                movups(reg, xed_mem_b(tempReg, 128));
            }
        }
    });
}

void Instruction::swap_out_upper_ymm(ymm_t *ymm, bool force) {
    withFreeReg([=] (xed_reg_enum_t tempReg) {
        std::unordered_set<xed_reg_enum_t> usedRegs;

        for (auto& op : operands) {
            if (op.isYmm() || (op.isXmm() && force)) {
                auto reg = op.toXmmReg();
                if (usedRegs.contains(reg)) {
                    continue;
                }
                usedRegs.insert(reg);
                uint32_t regnum = reg - XED_REG_XMM0;

                mov(tempReg, (uint64_t)(&(ymm->u[regnum])));
                movups(xed_mem_b(tempReg, 128), reg);

                mov(tempReg, (uint64_t)(&(ymm->l[regnum])));
                movups(reg, xed_mem_b(tempReg, 128));
            }
        }
    });
}

void Instruction::with_upper_ymm(ymm_t *ymm, std::function<void()> instr) {
    // This is also a wrong approach as the applicaiton might be multithreaded
    // and each threads needs to have its own copy of the upper ymm, which
    // we need somehow find dynamically and not hardcode addresses.
    swap_in_upper_ymm(ymm);
    instr();
    swap_out_upper_ymm(ymm);
}

bool Instruction::usesYmm() const {
    for(auto const& op : operands) {
        if (op.isYmm()) {
            return true;
        }
    }
    return false;
}

void Instruction::insertps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op2) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst3(&enc_inst, dstate, XED_ICLASS_INSERTPS, opWidth, subst(op0), subst(op1), subst(op2));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::addps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_ADDPS, opWidth, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::cvtss2sd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_CVTSS2SD, opWidth, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::movq(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVQ, opWidth, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::movdqu(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVDQU, opWidth, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::movdqa(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVDQA, opWidth, subst(op0), subst(op1));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::shufps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3) {
    withRipSubstitution([=] (std::function<xed_encoder_operand_t(xed_encoder_operand_t)> subst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst3(&enc_inst, dstate, XED_ICLASS_SHUFPS, opWidth, subst(op0), subst(op1), subst(op3));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::zeroupperInternal(ymm_t * ymm, Operand const& op) {
    withFreeReg([=] (xed_reg_enum_t tempReg) {
        auto reg = op.toXmmReg();
        uint32_t regnum = reg - XED_REG_XMM0;

        mov(tempReg, (uint64_t)(&(ymm->l[regnum])));
        movups(xed_mem_b(tempReg, 128), reg);

        mov(tempReg, (uint64_t)(&(ymm->u[regnum])));
        movups(reg, xed_mem_b(tempReg, 128));

        xorps(xed_reg(reg), xed_reg(reg));

        mov(tempReg, (uint64_t)(&(ymm->u[regnum])));
        movups(xed_mem_b(tempReg, 128), reg);

        mov(tempReg, (uint64_t)(&(ymm->l[regnum])));
        movups(reg, xed_mem_b(tempReg, 128));
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

void Instruction::withRipSubstitution(std::function<void(std::function<xed_encoder_operand_t(xed_encoder_operand_t subst)>)> instr) {
    // TODO: do not replace RIP if we are compiling inline
    if (usesRipAddressing()) {
        withFreeReg([=](xed_reg_enum_t tempReg) {
            mov(tempReg, rip);

            instr([=](xed_encoder_operand_t op) { return substRip(op, tempReg); });
        });
    } else if (usesRspAddressing()) {
        instr([=](xed_encoder_operand_t op) { return offsetRsp(op, rspOffset); });
    } else
    if (usesRipAddressing() && usesRspAddressing()) {
        withFreeReg([=](xed_reg_enum_t tempRipReg) {
            mov(tempRipReg, rip);
            instr([=](xed_encoder_operand_t op) { return offsetRsp(substRip(op, tempRipReg), rspOffset); });
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

    internal_requests.push_back(req);

    if (reg == XED_REG_RSP || reg == XED_REG_ESP) {
        rspOffset += immediate;
    }
}

void Instruction::withPreserveXmmReg(Operand const& op, std::function<void()> instr) {
    if (op.isYmm()) {
        printf("Preserving YMM is not supported yet\n");
    }

    if (!op.isXmm()) {
        printf("Only XMM registers are supported\n");
    }

    sub(XED_REG_ESP, 16);
    movdqu(xed_mem_b(XED_REG_RSP, 128), op.toEncoderOperand(false));

    instr();

    movdqu(op.toEncoderOperand(false), xed_mem_b(XED_REG_RSP, 128));
    add(XED_REG_ESP, 16);
}