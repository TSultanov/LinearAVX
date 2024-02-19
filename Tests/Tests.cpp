#include "Harness.h"
#include "TestCompiler.h"

#include <cstdio>

#include "../Instructions/Metadata.h"
#include "../Instructions/Instructions.h"
#include "xed/xed-iform-enum.h"

InstructionMetadata tests[] = {
    VSUBPS::Metadata,
    VSUBPD::Metadata,
    VXORPS::Metadata,
    VXORPD::Metadata,
    VSQRTPS::Metadata,
    VSQRTPD::Metadata,
    VUNPCKHPS::Metadata,
    VUNPCKLPS::Metadata,
    // VUCOMISS::Metadata, // TODO: fix compilation, test EFLAGS
    // VUCOMISD::Metadata, // TODO: fix compilation, test EFLAGS
    // VSHUFPS::Metadata, // TODO: immediate support
    // VSHUFPD::Metadata, // TODO: immediate support
    VPXOR::Metadata,
    VPSLLQ::Metadata,
    // VPERMILPS::Metadata, // TODO: immediate support
    VPCMPEQQ::Metadata,
    // VMULSS::Metadata, // TODO: compilation
    // VMULSD::Metadata, // TODO: compilation
    VMULPS::Metadata,
    VMULPD::Metadata,
};

int main() {
    xed_tables_init();

    bool hadErrors = false;

    for (auto metadata : tests) {
        TestCompiler compiler(metadata);
        auto thunks = compiler.getThunks();
        for (auto const& thunk : thunks) {
            printf("Test %s\n", xed_iform_enum_t2str(thunk.iform));
            Harness harness(thunk);
            TestResult result = harness.runTests();
            bool res = result.printResult();
            if (res) hadErrors = true;
        }
    }
    if (hadErrors) {
        printf("There were errors during the run\n");
        exit(1);
    }
}