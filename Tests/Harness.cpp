#include "Harness.h"
#include "TestCompiler.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-encode.h"
#include "xed/xed-error-enum.h"
#include "xed/xed-iclass-enum.h"
#include "xed/xed-inst.h"
#include "xed/xed-reg-class-enum.h"
#include "xed/xed-reg-enum.h"
#include <climits>
#include <cstdlib>
#include <cstring>
#include <immintrin.h>
#include <vector>
#include <xmmintrin.h>
#include <ucontext.h>

uint64_t* ThunkRegisters::getGprTempPtr(xed_reg_enum_t reg) {
    if (gpMap.contains(reg)) {
        return &gpRegsValuesTemp[gpMap[reg]];
    }

    gpRegsValuesTemp.push_back(0);
    gpRegsValuesInOut.push_back(0);
    gpMap[reg] = gpRegsValuesTemp.size() - 1;
    return &gpRegsValuesTemp[gpMap[reg]];
}

uint64_t* ThunkRegisters::getGprInOutPtr(xed_reg_enum_t reg) {
    if (ymmMap.contains(reg)) {
        return &gpRegsValuesInOut[gpMap[reg]];
    }

    gpRegsValuesTemp.push_back(0);
    gpRegsValuesInOut.push_back(0);
    ymmMap[reg] = gpRegsValuesTemp.size() - 1;
    return &gpRegsValuesInOut[ymmMap[reg]];
}

__m256* ThunkRegisters::getYmmTempPtr(xed_reg_enum_t reg) {
    if (ymmMap.contains(reg)) {
        return &ymmRegsValuesTemp[ymmMap[reg]];
    }

    const auto zero = _mm256_set1_epi8(0);
    ymmRegsValuesTemp.push_back(zero);
    ymmRegsValuesInOut.push_back(zero);
    ymmMap[reg] = gpRegsValuesTemp.size() - 1;
    return &ymmRegsValuesTemp[ymmMap[reg]];
}

__m256* ThunkRegisters::getYmmInOutPtr(xed_reg_enum_t reg) {
    if (ymmMap.contains(reg)) {
        return &ymmRegsValuesInOut[ymmMap[reg]];
    }

    const auto zero = _mm256_set1_epi8(0);
    ymmRegsValuesTemp.push_back(zero);
    ymmRegsValuesInOut.push_back(zero);
    ymmMap[reg] = gpRegsValuesTemp.size() - 1;
    return &ymmRegsValuesInOut[ymmMap[reg]];
}

TestResult Harness::runTests() {
    auto testValues = generateTestValues();
    auto nativeResult = runNativeTest(testValues);
    auto translatedResult = runTranslatedTest(testValues);
    return TestResult(nativeResult, translatedResult);
}

__m256 m128tm256(__m128 in) {
    float d[4];
    _mm_store_ps(d, in);
    return _mm256_set_ps(d[0], d[1], d[2], d[3], 0, 0, 0, 0);
}

xed_encoder_request_t inst0(xed_iclass_enum_t iclass, xed_uint_t opWidth) {
    const xed_state_t dstate {
        .mmode = XED_MACHINE_MODE_LONG_64,
        .stack_addr_width = XED_ADDRESS_WIDTH_64b
    };

    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;
    xed_encoder_request_zero_set_mode(&req, &dstate);
    xed_encoder_request_set_iclass(&req, iclass);

    xed_inst0(&enc_inst, dstate, iclass, opWidth);

    xed_convert_to_encoder_request(&req, &enc_inst);

    return req;
}

xed_encoder_request_t inst1(xed_iclass_enum_t iclass, xed_uint_t opWidth, xed_encoder_operand_t op0) {
    const xed_state_t dstate {
        .mmode = XED_MACHINE_MODE_LONG_64,
        .stack_addr_width = XED_ADDRESS_WIDTH_64b
    };

    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;
    xed_encoder_request_zero_set_mode(&req, &dstate);
    xed_encoder_request_set_iclass(&req, iclass);

    xed_inst1(&enc_inst, dstate, iclass, opWidth, op0);

    xed_convert_to_encoder_request(&req, &enc_inst);

    return req;
}

xed_encoder_request_t inst2(xed_iclass_enum_t iclass, xed_bits_t vl, xed_uint_t opWidth, xed_encoder_operand_t op0, xed_encoder_operand_t op1) {
    const xed_state_t dstate {
        .mmode = XED_MACHINE_MODE_LONG_64,
        .stack_addr_width = XED_ADDRESS_WIDTH_64b
    };

    xed_encoder_request_t req;
    xed_encoder_instruction_t enc_inst;
    xed_encoder_request_zero_set_mode(&req, &dstate);
    xed_encoder_request_set_iclass(&req, iclass);

    xed_inst2(&enc_inst, dstate, iclass, opWidth, op0, op1);

    xed_convert_to_encoder_request(&req, &enc_inst);
    xed3_operand_set_vl(&req, vl);

    return req;
}

std::vector<xed_encoder_request_t> generateHarness(ThunkRegisters & registers, const void* thunk)
{
    std::vector<xed_encoder_request_t> requests;
    // Generate instructions to save registers
    // First save all YMM registers
    for (xed_reg_enum_t reg : TestCompiler::ymmRegs) {
        uint64_t memoryAddress = (uint64_t)registers.getYmmTempPtr(reg);
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(XED_REG_RAX), xed_imm0(memoryAddress, 64)));
        requests.push_back(inst2(XED_ICLASS_VMOVUPS, 256, 0, xed_mem_b(XED_REG_RAX, 64), xed_reg(reg)));
    }

    // Next, save all GPRs (except RAX)
    for (xed_reg_enum_t reg : TestCompiler::gpRegs) {
        if (reg == XED_REG_RAX) continue;
        uint64_t memoryAddress = (uint64_t)registers.getGprTempPtr(reg);
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(XED_REG_RAX), xed_imm0(memoryAddress, 64)));
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_mem_b(XED_REG_RAX, 64), xed_reg(reg)));
    }

    // Load input state
    // First load all YMM registers
    for (xed_reg_enum_t reg : TestCompiler::ymmRegs) {
        uint64_t memoryAddress = (uint64_t)registers.getYmmInOutPtr(reg);
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(XED_REG_RAX), xed_imm0(memoryAddress, 64)));
        requests.push_back(inst2(XED_ICLASS_VMOVUPS, 256, 0, xed_reg(reg), xed_mem_b(XED_REG_RAX, 64)));
    }

    // Next, load all GPRs (except RAX)
    for (xed_reg_enum_t reg : TestCompiler::gpRegs) {
        if (reg == XED_REG_RAX) continue;
        uint64_t memoryAddress = (uint64_t)registers.getGprInOutPtr(reg);
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(XED_REG_RAX), xed_imm0(memoryAddress, 64)));
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(reg), xed_mem_b(XED_REG_RAX, 64)));
    }

    // Call the thunk
    requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(XED_REG_RAX), xed_imm0((uint64_t)thunk, 64)));
    requests.push_back(inst1(XED_ICLASS_CALL_NEAR, 64, xed_reg(XED_REG_RAX)));

    // Save registers
    // First save all YMM registers
    for (xed_reg_enum_t reg : TestCompiler::ymmRegs) {
        uint64_t memoryAddress = (uint64_t)registers.getYmmInOutPtr(reg);
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(XED_REG_RAX), xed_imm0(memoryAddress, 64)));
        requests.push_back(inst2(XED_ICLASS_VMOVUPS, 256, 0, xed_mem_b(XED_REG_RAX, 64), xed_reg(reg)));
    }

    // Next, save all GPRs (except RAX)
    for (xed_reg_enum_t reg : TestCompiler::gpRegs) {
        if (reg == XED_REG_RAX) continue;
        uint64_t memoryAddress = (uint64_t)registers.getGprInOutPtr(reg);
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(XED_REG_RAX), xed_imm0(memoryAddress, 64)));
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_mem_b(XED_REG_RAX, 64), xed_reg(reg)));
    }

    // Restore registers
    // First load all YMM registers
    for (xed_reg_enum_t reg : TestCompiler::ymmRegs) {
        uint64_t memoryAddress = (uint64_t)registers.getYmmTempPtr(reg);
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(XED_REG_RAX), xed_imm0(memoryAddress, 64)));
        requests.push_back(inst2(XED_ICLASS_VMOVUPS, 256, 0, xed_reg(reg), xed_mem_b(XED_REG_RAX, 64)));
    }

    // Next, load all GPRs (except RAX)
    for (xed_reg_enum_t reg : TestCompiler::gpRegs) {
        if (reg == XED_REG_RAX) continue;
        uint64_t memoryAddress = (uint64_t)registers.getGprTempPtr(reg);
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(XED_REG_RAX), xed_imm0(memoryAddress, 64)));
        requests.push_back(inst2(XED_ICLASS_MOV, 0, 64, xed_reg(reg), xed_mem_b(XED_REG_RAX, 64)));
    }

    requests.push_back(inst0(XED_ICLASS_RET_NEAR, 64));
    return requests;
}

OneTestResult Harness::runTest(TestValues const& values, const void* thunk) {
    RegisterBank inputBank;
    // Set register bank
    for (auto const& reg : values.reg) {
        switch(reg.regClass) {
            case XED_REG_CLASS_GPR:
            case XED_REG_CLASS_GPR64:
            {
                inputBank.gpRegs[reg.reg] = reg.v.value64;
                break;
            }
            case XED_REG_CLASS_YMM:
            {
                inputBank.ymmRegs[reg.reg] = reg.v.value256;
                break;
            }
            case XED_REG_CLASS_XMM:
            {
                inputBank.ymmRegs[xmmToYmm(reg.reg)] = m128tm256(reg.v.value128);
            }
            default:
            {
                printf("runTest(): unsupported register class: %s\n", xed_reg_class_enum_t2str(reg.regClass));
                exit(1);
            }
        }
    }
    // Set memory
    for (int i = 0; i < values.mem.size(); i++) {
        testThunk.usedMemory.memory[i] = values.mem[i];
    }

    // this is the core of the harness
    // here we will switch context to the actuall function implementation

    RegisterBank outputBank;

    ThunkRegisters registers;
    for (xed_reg_enum_t reg : TestCompiler::ymmRegs) {
        if (inputBank.ymmRegs.contains(reg)) {
            *(registers.getYmmInOutPtr(reg)) = inputBank.ymmRegs[reg];
        } else {
            *(registers.getYmmInOutPtr(reg)) = _mm256_set1_epi8(0);
        }
    }
    for (xed_reg_enum_t reg : TestCompiler::gpRegs) {
        if (inputBank.gpRegs.contains(reg)) {
            *(registers.getGprInOutPtr(reg)) = inputBank.gpRegs[reg];
        } else {
            *(registers.getGprInOutPtr(reg)) = 0;
        }
    }
 
    auto requests = generateHarness(registers, thunk);
    auto compiledHarness = TestCompiler::compileRequests(requests);
    ((void(*)(void))compiledHarness)(); // Execute harness

    for (xed_reg_enum_t reg : TestCompiler::ymmRegs) {
        if (outputBank.ymmRegs.contains(reg)) {
            outputBank.ymmRegs[reg] = *(registers.getYmmInOutPtr(reg));
        }
    }
    for (xed_reg_enum_t reg : TestCompiler::gpRegs) {
        if (outputBank.gpRegs.contains(reg)) {
            outputBank.gpRegs[reg] = *(registers.getGprInOutPtr(reg));
        }
    }

    MemoryValue memResult;
    for (int i = 0; i < values.mem.size(); i++) {
        memResult[i] = testThunk.usedMemory.memory[i];
    }
    RegValues regResult;
    for (auto const& reg : values.reg) {
        switch (reg.regClass) {
            case XED_REG_CLASS_GPR:
            case XED_REG_CLASS_GPR64:
            {
                regResult.emplace_back(RegValue(reg.reg, {.value64 = outputBank.gpRegs[reg.reg]}));
                break;
            }
            case XED_REG_CLASS_YMM:
            {
                regResult.emplace_back(RegValue(reg.reg, {.value256 = outputBank.ymmRegs[reg.reg]}));
                break;
            }
            case XED_REG_CLASS_XMM:
            {
                regResult.emplace_back(RegValue(reg.reg, {.value256 = outputBank.ymmRegs[xmmToYmm(reg.reg)]}));
                break;
            }
            default:
            {
                printf("runTest(): unsupported register class: %s\n", xed_reg_class_enum_t2str(reg.regClass));
                exit(1);
            }
        }
    }
    return OneTestResult(values, TestValues(regResult, memResult));
}

TestValues Harness::generateTestValues() const {
    return TestValues(generateRegValues(), generateMemValue());
}

MemoryValue Harness::generateMemValue() const {
    MemoryValue ret;
    for (int i = 0; i < testThunk.usedMemory.memory.size(); i++) {
        ret.push_back(std::rand());
    }
    return ret;
}

RegValues Harness::generateRegValues() const {
    RegValues values;
    for (xed_reg_enum_t usedReg : testThunk.usedRegisters) {
        values.emplace_back(generateRegValue(usedReg));
    }
    return values;
}

RegValue Harness::generateRegValue(xed_reg_enum_t reg) const {
    auto regClass = xed_reg_class(reg);
    switch (regClass) {
        case XED_REG_CLASS_XMM:
        {
            const int vecLength = 4;
            float randomValues[vecLength];
            for (int i = 0; i < vecLength; i++) {
                int randValue = std::rand();
                randomValues[i] = *(float*)&randValue;
            }
            __m128 value = _mm_set_ps(randomValues[0], randomValues[1], randomValues[2], randomValues[3]);
            return RegValue(reg, {.value128 = value});
        }
        case XED_REG_CLASS_YMM:
        {
            const int vecLength = 8;
            float randomValues[vecLength];
            for (int i = 0; i < vecLength; i++) {
                int randValue = std::rand();
                randomValues[i] = *(float*)&randValue;
            }
            __m256 value = _mm256_set_ps(randomValues[0], randomValues[1], randomValues[2], randomValues[3], randomValues[4], randomValues[5], randomValues[6], randomValues[7]);
            return RegValue(reg, {.value256 = value});
        }
        case XED_REG_CLASS_GPR:
        case XED_REG_CLASS_GPR64:
        {
            const int vecLength = 2;
            uint32_t randomValues[vecLength];
            for (int i = 0; i < vecLength; i++) {
                int randValue = std::rand();
                randomValues[i] = *(float*)&randValue;
            }
            uint64_t value = ((uint64_t)randomValues[1] << 32) + randomValues[0];
            return RegValue(reg, {.value64 = value});
        }
        default:
        {
            printf("Harness::generateRegValue(): Unsupported reg class %s\n", xed_reg_class_enum_t2str(regClass));
            exit(1);
        }
    }
}