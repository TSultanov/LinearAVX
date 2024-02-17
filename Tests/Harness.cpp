#include "Harness.h"
#include "TestCompiler.h"
#include "xed/xed-reg-class-enum.h"
#include "xed/xed-reg-enum.h"
#include <climits>
#include <cstdlib>
#include <immintrin.h>
#include <vector>
#include <xmmintrin.h>
#include <ucontext.h>

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

    ucontext_t mainContext;
    getcontext(&mainContext);

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