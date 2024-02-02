#include "Instruction.h"
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-reg-enum.h"

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
    if (ripSubstReg == XED_REG_RIP) {
        return op;
    }

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

void Instruction::push(xed_encoder_operand_t op) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst1(&enc_inst, dstate, XED_ICLASS_PUSH, 64, op);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
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
    withRipSubstitution([=] (xed_reg_enum_t ripSubst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        // printf("opWidth = %d\n", opWidth);
        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVUPS, opWidth, substRip(op0, ripSubst), substRip(op1, ripSubst));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::movaps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (xed_reg_enum_t ripSubst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVAPS, 0, substRip(op0, ripSubst), substRip(op1, ripSubst));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::movss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (xed_reg_enum_t ripSubst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVSS, opWidth, substRip(op0, ripSubst), substRip(op1, ripSubst));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::movsd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (xed_reg_enum_t ripSubst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVSD_XMM, opWidth, substRip(op0, ripSubst), substRip(op1, ripSubst));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::xorps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (xed_reg_enum_t ripSubst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_XORPS, 0, substRip(op0, ripSubst), substRip(op1, ripSubst));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::swap_in_upper_ymm(ymm_t *ymm, bool force) {
    withFreeReg([=] (xed_reg_enum_t tempReg) {
        for (auto& op : operands) {
            if (op.isYmm() || (op.isXmm() && force)) {
                auto reg = op.toXmmReg();
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
        for (auto& op : operands) {
            if (op.isYmm() || (op.isXmm() && force)) {
                auto reg = op.toXmmReg();
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
    withRipSubstitution([=] (xed_reg_enum_t ripSubst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst3(&enc_inst, dstate, XED_ICLASS_INSERTPS, opWidth, substRip(op0, ripSubst), substRip(op1, ripSubst), substRip(op2, ripSubst));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::addps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (xed_reg_enum_t ripSubst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_ADDPS, opWidth, substRip(op0, ripSubst), substRip(op1, ripSubst));
        xed_convert_to_encoder_request(&req, &enc_inst);
        xed3_operand_set_vl(&req, vl);

        internal_requests.push_back(req);
    });
}

void Instruction::cvtss2sd(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (xed_reg_enum_t ripSubst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_CVTSS2SD, opWidth, substRip(op0, ripSubst), substRip(op1, ripSubst));
        xed_convert_to_encoder_request(&req, &enc_inst);

        internal_requests.push_back(req);
    });
}

void Instruction::movq(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    withRipSubstitution([=] (xed_reg_enum_t ripSubst) {
        xed_encoder_request_t req;
        xed_encoder_instruction_t enc_inst;

        xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVQ, opWidth, substRip(op0, ripSubst), substRip(op1, ripSubst));
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

void Instruction::withRipSubstitution(std::function<void(xed_reg_enum_t ripSubst)> instr) {
    if (usesRipAddressing()) {
        withFreeReg([=](xed_reg_enum_t tempReg) {
            mov(tempReg, rip);

            instr(tempReg);
        });
    } else {
        instr(XED_REG_RIP);
    }
}