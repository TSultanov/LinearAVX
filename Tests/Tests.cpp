#include "TestCompiler.h"

#include <cstdio>

#include "../Instructions/Metadata.h"
#include "../Instructions/VSUBPS.h"

int main() {
    TestCompiler compiler(VSUBPS::Metadata);
    auto request = compiler.generateInstructions();
    // TODO: Rename TestCompiler to TestGenerator
    // TODO: Compile generated tests to both native AVX and translated AVX
    // TODO: Make test runner harness which can track registers statuses
}