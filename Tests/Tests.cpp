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
    VEXTRACTF128::Metadata,
    VEXTRACTPS::Metadata,
    VINSERTF128::Metadata,
    VINSERTPS::Metadata,
    VMOVQ::Metadata,
    VMOVSS::Metadata,
    VMOVSD::Metadata,
    VPERM2F128::Metadata,
    VRSQRTPS::Metadata,
    VRSQRTSS::Metadata,
    VPEXTRB::Metadata,
    VPEXTRD::Metadata,
    VPEXTRQ::Metadata,
    VPEXTRW::Metadata,
    VPSHUFB::Metadata,
    VMOVD::Metadata,
    VADDSS::Metadata,
    VADDSD::Metadata,
    VPINSRB::Metadata,
    VPINSRD::Metadata,
    VPINSRQ::Metadata,
    VPADDB::Metadata,
    VPADDW::Metadata,
    VPADDD::Metadata,
    VPADDQ::Metadata,
    VPSRLDQ::Metadata,
    ANDN::Metadata,
    SARX::Metadata,
    BLSR::Metadata,
    VMOVHPD::Metadata,
    VMOVHPS::Metadata,
    VPBROADCASTB::Metadata,
    VPCMPEQD::Metadata,
    VPCMPEQW::Metadata,
    VPCMPEQB::Metadata,
    VPMOVMSKB::Metadata,
    VPSIGNB::Metadata,
    VPSIGNW::Metadata,
    VPSIGND::Metadata,
    VPCMPGTB::Metadata,
    VPCMPGTW::Metadata,
    VPCMPGTD::Metadata,
    VPCMPGTQ::Metadata,
    VPSRLQ::Metadata,
    VPAND::Metadata,
    VPSUBQ::Metadata,
    VPSUBD::Metadata,
    VPSUBW::Metadata,
    VPSUBB::Metadata,
    VSTMXCSR::Metadata,
    // VLDMXCSR::Metadata, // TODO: memory need to have proper data before executing instruction
    VBLENDVPS::Metadata,
};

int main() {
    xed_tables_init();

    uint64_t numErrors = 0;

    for (int i = 0; i < 3; i ++) {
        printf("==== Run %d ====\n", i);
        uint64_t runErrors = 0;
        for (auto metadata : tests) {
            TestCompiler compiler(metadata);
            auto thunks = compiler.getThunks();
            for (auto const& thunk : thunks) {
                printf("Test %s\n", xed_iform_enum_t2str(thunk.iform));
                Harness harness(thunk);
                TestResult result = harness.runTests();
                bool res = result.printResult();
                if (res) { 
                    numErrors++;
                    runErrors++;
                }
            }
        }
        printf("There were %lu errors during run %d\n\n", runErrors, i);
    }
    if (numErrors) {
        printf("There were %lu errors during all runs\n", numErrors);
        exit(1);
    }
}