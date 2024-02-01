#include "Instruction.h"
#include "xed/xed-encode.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-reg-enum.h"

const xed_state_t dstate = {.mmode = XED_MACHINE_MODE_LONG_64,
                            .stack_addr_width = XED_ADDRESS_WIDTH_64b};

Instruction::Instruction(const xed_decoded_inst_t *xedd)
:xi(xed_decoded_inst_inst(xedd))
,opWidth(xed_decoded_inst_get_operand_width(xedd))
,vl(xed3_operand_get_vl(xedd))
,xedd(xedd)
{
    auto n_operands = xed_inst_noperands(xi);
    for (uint32_t i = 0; i < n_operands; i++) {
        operands.emplace_back(Operand(xedd, i));
    }
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
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVUPS, 0, op0, op1);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::movaps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVAPS, 0, op0, op1);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::movss(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_MOVSS, opWidth, op0, op1);
    xed_convert_to_encoder_request(&req, &enc_inst);
    xed3_operand_set_vl(&req, vl);

    internal_requests.push_back(req);
}

void Instruction::xorps(xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst2(&enc_inst, dstate, XED_ICLASS_XORPS, 0, op0, op1);
    xed_convert_to_encoder_request(&req, &enc_inst);

    internal_requests.push_back(req);
}

void Instruction::swap_in_upper_ymm(ymm_t *ymm) {
    push(XED_REG_RAX);
    
    for (auto& op : operands) {
        if (op.isXmm() || op.isYmm()) {
            auto reg = op.toXmmReg();
            uint32_t regnum = reg - XED_REG_XMM0;

            mov(XED_REG_RAX, (uint64_t)(&(ymm->l[regnum])));
            movups(xed_mem_b(XED_REG_RAX, 128), reg);

            mov(XED_REG_RAX, (uint64_t)(&(ymm->u[regnum])));
            movups(reg, xed_mem_b(XED_REG_RAX, 128));
        }
    }

    pop(XED_REG_RAX);

}

void Instruction::swap_out_upper_ymm(ymm_t *ymm) {
    push(XED_REG_RAX);
    
    for (auto& op : operands) {
        if (op.isXmm() || op.isYmm()) {
            auto reg = op.toXmmReg();
            uint32_t regnum = reg - XED_REG_XMM0;

            mov(XED_REG_RAX, (uint64_t)(&(ymm->u[regnum])));
            movups(xed_mem_b(XED_REG_RAX, 128), reg);

            mov(XED_REG_RAX, (uint64_t)(&(ymm->l[regnum])));
            movups(reg, xed_mem_b(XED_REG_RAX, 128));
        }
    }

    pop(XED_REG_RAX);
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

void Instruction::insertps(xed_encoder_operand_t op0, xed_encoder_operand_t op1, xed_encoder_operand_t op3) {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;

    xed_inst3(&enc_inst, dstate, XED_ICLASS_INSERTPS, opWidth, op0, op1, op3);
    xed_convert_to_encoder_request(&req, &enc_inst);
    xed3_operand_set_vl(&req, vl);

    internal_requests.push_back(req);
}