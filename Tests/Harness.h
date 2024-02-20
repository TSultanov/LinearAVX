#pragma once
#include "TestCompiler.h"
#include "xed/xed-reg-class-enum.h"
#include "xed/xed-reg-enum.h"
#include <cstddef>
#include <emmintrin.h>
#include <unordered_map>
#include <vector>
#include <xmmintrin.h>
#include <immintrin.h>

class ThunkRegisters {
    volatile uint64_t gpRegsValuesTemp[16];
    volatile __m256 ymmRegsValuesTemp[16] __attribute__((aligned(32)));
    volatile uint64_t gpRegsValuesInOut[16];
    volatile __m256 ymmRegsValuesInOut[16] __attribute__((aligned(32)));
    volatile uint64_t rflags;
public:
    ThunkRegisters();
    volatile uint64_t* getGprTempPtr(xed_reg_enum_t reg) volatile;
    volatile __m256* getYmmTempPtr(xed_reg_enum_t reg) volatile;
    volatile uint64_t* getGprInOutPtr(xed_reg_enum_t reg) volatile;
    volatile __m256* getYmmInOutPtr(xed_reg_enum_t reg) volatile;
    volatile uint64_t* getRflags() volatile;
};

union RegValueUnion {
    uint64_t value64;
    // __m128 value128;
    __m256 value256;
};

struct RegValue {
    RegValue(xed_reg_enum_t reg, RegValueUnion v)
    : reg(reg)
    , regClass(xed_reg_class(reg))
    , v(v)
    {}
    const xed_reg_enum_t reg;
    const xed_reg_class_enum_t regClass;
    const RegValueUnion v;
};

using RegValues = std::vector<RegValue>;
using MemoryValue = std::vector<uint8_t>;

struct TestValues {
    TestValues(RegValues const& reg, MemoryValue const& mem)
    : reg(reg)
    , mem(mem)
    {}

    const RegValues reg;
    const MemoryValue mem;
};

struct OneTestResult {
    OneTestResult(TestValues const& input, TestValues const& output)
    : input(input)
    , output(output)
    {}

    const TestValues input;
    const TestValues output;
};

struct TestResult {
    TestResult(OneTestResult const& nativeResult, OneTestResult const& translatedResult)
    : nativeResult(nativeResult)
    , translatedResult(translatedResult)
    {}

    OneTestResult nativeResult;
    OneTestResult translatedResult;

    bool printResult() const;
};

struct RegisterBank {
    RegisterBank() {
        for(uint32_t reg = XED_REG_RAX; reg <= XED_REG_R15; reg++) {
            gpRegs[(xed_reg_enum_t)reg] = 0;
        }

        for(uint32_t reg = XED_REG_YMM0; reg <= XED_REG_YMM15; reg++) {
            ymmRegs[(xed_reg_enum_t)reg] = _mm256_set1_ps(0);
        }
    }

    uint64_t rflags = 0;
    std::unordered_map<xed_reg_enum_t, uint64_t> gpRegs;
    std::unordered_map<xed_reg_enum_t, __m256> ymmRegs;
};

class Harness {
public:
    Harness(TestThunk const& testThunk)
    : testThunk(testThunk)
    {}

    TestResult runTests();
private:
    TestThunk testThunk;

    RegValue generateRegValue(xed_reg_enum_t reg) const;
    RegValues generateRegValues() const;
    MemoryValue generateMemValue() const;
    TestValues generateTestValues() const;
    OneTestResult runTest(TestValues const& values, const void* thunk, bool translated);
    OneTestResult runNativeTest(TestValues const& values) {
        return runTest(values, testThunk.compiledNativeThunk, false);
    }
    OneTestResult runTranslatedTest(TestValues const& values) {
        return runTest(values, testThunk.compiledTranslatedThunk, true);
    }
};