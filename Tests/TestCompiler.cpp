#include "TestCompiler.h"

#include <cstring>
#include <xed/xed-interface.h>
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-decoded-inst.h"
#include "xed/xed-encode.h"
#include "xed/xed-reg-class-enum.h"
#include "xed/xed-reg-enum.h"

#include <optional>
#include <sys/unistd.h>
#include <unordered_set>
#include <vector>
#include "../memmanager.h"
#include "../Compiler/Compiler.h"
#include "../Instructions/Instructions.h"

static xed_state_t dstate {
    .mmode = XED_MACHINE_MODE_LONG_64,
    .stack_addr_width = XED_ADDRESS_WIDTH_64b
};

const std::vector<xed_reg_enum_t> TestCompiler::gpRegs = {
    XED_REG_RAX, XED_REG_RBX, XED_REG_RCX, XED_REG_RDX, XED_REG_RSI, XED_REG_RDI, XED_REG_R8, XED_REG_R9,
    XED_REG_R10, XED_REG_R11, XED_REG_R12, XED_REG_R13, XED_REG_R14, XED_REG_R15
};

const std::vector<xed_reg_enum_t> TestCompiler::xmmRegs = {
    XED_REG_XMM0, XED_REG_XMM1, XED_REG_XMM2, XED_REG_XMM3, XED_REG_XMM4, XED_REG_XMM5, XED_REG_XMM6, XED_REG_XMM7,
    XED_REG_XMM8, XED_REG_XMM9, XED_REG_XMM10, XED_REG_XMM11, XED_REG_XMM12, XED_REG_XMM13, XED_REG_XMM14,
    XED_REG_XMM15
};

const std::vector<xed_reg_enum_t> TestCompiler::ymmRegs = {
    XED_REG_YMM0, XED_REG_YMM1, XED_REG_YMM2, XED_REG_YMM3, XED_REG_YMM4, XED_REG_YMM5, XED_REG_YMM6, XED_REG_YMM7,
    XED_REG_YMM8, XED_REG_YMM9, XED_REG_YMM10, XED_REG_YMM11, XED_REG_YMM12, XED_REG_YMM13, XED_REG_YMM14,
    XED_REG_YMM15
};

TestCompiler::TestCompiler(InstructionMetadata const& metadata)
: metadata(metadata)
{}

std::vector<ThunkRequest> TestCompiler::generateInstructions() const {
    std::vector<ThunkRequest> ret;
    for(auto const& op : metadata.operandSets) {
        auto inst = generateInstruction(op);
        ret.emplace_back(inst);
    }
    return ret;
}

ThunkRequest TestCompiler::generateInstruction(OperandsMetadata const& om) const {
    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;
    xed_encoder_request_zero_set_mode(&req, &dstate);
    xed_encoder_request_set_iclass(&req, metadata.iclass);

    std::unordered_set<xed_reg_enum_t> usedRegisters;
    usedRegisters.insert(XED_REG_RAX); //reserve RAX
    xed_reg_enum_t baseReg;
    std::vector<uint8_t> memory;
    memory.reserve(om.vectorLength / 8);
    for (int i = 0; i < om.vectorLength / 8; i++) {
        memory.push_back(0xff);
    }

    auto getOperand = [&](OperandMetadata const& om) -> std::optional<xed_encoder_operand_t> {
        switch (om.operand) {
            case XED_ENCODER_OPERAND_TYPE_REG:
            {
                switch (om.regClass) {
                    case XED_REG_CLASS_XMM:
                    {
                        for (auto reg : xmmRegs) {
                            if (usedRegisters.contains(reg) || usedRegisters.contains(xmmToYmm(reg))) continue;
                            usedRegisters.insert(reg);
                            return xed_reg(reg);
                        }

                        return std::nullopt;
                    }
                    case XED_REG_CLASS_YMM:
                    {
                        for (auto reg : ymmRegs) {
                            if (usedRegisters.contains(reg) || usedRegisters.contains(ymmToXmm(reg))) continue;
                            usedRegisters.insert(reg);
                            return xed_reg(reg);
                        }

                        return std::nullopt;
                    }
                    case XED_REG_CLASS_GPR:
                    {
                        for (auto reg : gpRegs) {
                            if (usedRegisters.contains(reg)) continue;
                            usedRegisters.insert(reg);
                            return xed_reg(reg);
                        }

                        return std::nullopt;
                    }
                    default:
                    {
                        printf("Unsupported reg class\n");
                        exit(1);
                    }
                }
            }
            case XED_ENCODER_OPERAND_TYPE_MEM:
            {
                for (auto reg : gpRegs) {
                    if (usedRegisters.contains(reg)) continue;
                    usedRegisters.insert(reg);
                    baseReg = reg;
                    return xed_mem_b(reg, 32);
                }
                return std::nullopt;
            }
            default:
            {
                printf("Unsupported operand type\n");
                exit(1);
            }
        }
    };

    if (om.operands.size() == 1) {
        auto op = getOperand(om.operands[0]);
        if (!op.has_value()) {
            printf("Failed to generated instruction.\n");
            exit(1);
        }
        xed_inst1(&enc_inst, dstate, metadata.iclass, 0, *op);
    }

    if (om.operands.size() == 2) {
        auto op1 = getOperand(om.operands[0]);
        if (!op1.has_value()) {
            printf("Failed to generated instruction.\n");
            exit(1);
        }
        auto op2 = getOperand(om.operands[1]);
        if (!op2.has_value()) {
            printf("Failed to generated instruction.\n");
            exit(1);
        }
        xed_inst2(&enc_inst, dstate, metadata.iclass, 0, *op1, *op2);
    }

    if (om.operands.size() == 3) {
        auto op1 = getOperand(om.operands[0]);
        if (!op1.has_value()) {
            printf("Failed to generated instruction.\n");
            exit(1);
        }
        auto op2 = getOperand(om.operands[1]);
        if (!op2.has_value()) {
            printf("Failed to generated instruction.\n");
            exit(1);
        }
        auto op3 = getOperand(om.operands[2]);
        if (!op3.has_value()) {
            printf("Failed to generated instruction.\n");
            exit(1);
        }
        xed_inst3(&enc_inst, dstate, metadata.iclass, 0, *op1, *op2, *op3);
    }

    xed_convert_to_encoder_request(&req, &enc_inst);
    xed3_operand_set_vl(&req, om.vectorLength);

    return ThunkRequest(usedRegisters, TempMemory(baseReg, memory), req);
}

void* TestCompiler::compileRequests(std::vector<xed_encoder_request_t> requests) {
    struct compiledInstruction {
        uint8_t buf[15];
        uint32_t length;
    };
    std::vector<compiledInstruction> instructions;

    for (auto& req : requests) {
        compiledInstruction instr;
        auto err = xed_encode(&req, instr.buf, 15, &instr.length);
        if (err != XED_ERROR_NONE) {
            printf("Error encoding %s\n", xed_iclass_enum_t2str(xed_decoded_inst_get_iclass(&req)));
            exit(1);
        }
        instructions.push_back(instr);
    }

    uint32_t totalOlen = 0;
    for (auto const& instr : instructions) {
        totalOlen += instr.length;
    }

    uint8_t* stencil = alloc_executable(totalOlen);
    uint32_t offset = 0;
    for (auto const &instr : instructions) {
        memcpy(stencil + offset, instr.buf, instr.length);
        offset += instr.length;
    }
    return stencil;
}

TestThunk TestCompiler::compileThunk(ThunkRequest const& request) const {
    void* nativeThunk = compileNativeThunk(request);
    void* translatedThunk = compileTranslatedThunk(request);

    return TestThunk(request.usedRegisters, request.usedMemory, nativeThunk, translatedThunk);
}

void* TestCompiler::compileNativeThunk(ThunkRequest const& request) const {
    std::vector<xed_encoder_request_t> requests;
    requests.push_back(request.instructionRequest);

    return compileRequests(requests);
}

void* TestCompiler::compileTranslatedThunk(ThunkRequest const& request) const {
    xed_iclass_enum_t iclass = xed_decoded_inst_get_iclass(&request.instructionRequest);
    auto instructionFactory = iclassMapping.at(iclass);
    auto instruction = instructionFactory(0, 0, request.instructionRequest);

    Compiler compiler;
    compiler.addInstruction(instruction);
    uint32_t olen;
    return compiler.encode(CompilationStrategy::DirectCall, &olen, 0);
}

std::vector<TestThunk> TestCompiler::getThunks() const {
    auto thunks = generateInstructions();
    std::vector<TestThunk> ret;
    for (auto const& req : thunks) {
        ret.emplace_back(compileThunk(req));
    }
    return ret;
}