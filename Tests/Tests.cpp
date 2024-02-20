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
    VUCOMISS::Metadata,
    VUCOMISD::Metadata,
    VSHUFPS::Metadata,
    VSHUFPD::Metadata,
    VPXOR::Metadata,
    VPSLLQ::Metadata,
    VPERMILPS::Metadata,
    VPCMPEQQ::Metadata,
    VMULSS::Metadata,
    VMULSD::Metadata,
    VMULPS::Metadata,
    VMULPD::Metadata,
    VMOVUPS::Metadata,
    VMOVUPD::Metadata,
    VMOVMSKPS::Metadata,
    VMOVMSKPD::Metadata,
    VMOVLHPS::Metadata,
    VMOVDQU::Metadata,
    VMOVDQA::Metadata,
    VMOVAPS::Metadata,
    VMOVAPD::Metadata,
    VMINSS::Metadata,
    VMINSD::Metadata,
    VMAXSS::Metadata,
    VMAXSD::Metadata,
    VHADDPS::Metadata,
    VHADDPD::Metadata,
    VFMSUB231PS::Metadata,
    VFMADD231PS::Metadata,
    VDIVSS::Metadata,
    VDIVSD::Metadata,
    VCVTTSS2SI::Metadata,
    VCVTTSD2SI::Metadata,
    VCVTSS2SD::Metadata,
    VCVTSI2SS::Metadata,
    VCVTSI2SD::Metadata,
    VCVTSD2SS::Metadata,
    VCVTPS2PH::Metadata,
    VCOMISS::Metadata,
    VCOMISD::Metadata,
    VCMPPD::Metadata,
    VBROADCASTSS::Metadata,
    VBLENDVPD::Metadata,
    VANDPS::Metadata,
    VANDPD::Metadata,
    VANDNPS::Metadata,
    VANDNPD::Metadata,
    VADDPS::Metadata,
    VADDPD::Metadata,
    SHRX::Metadata,
    SHLX::Metadata,
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