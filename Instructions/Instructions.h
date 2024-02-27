#pragma once

#include "Instruction.h"
#include "VMOVSS.h"
#include "VMOVUPS.h"
#include "VMOVUPD.h"
#include "VMOVAPS.h"
#include "VMOVAPD.h"
#include "VXORPS.h"
#include "VINSERTPS.h"
#include "VINSERTF128.h"
#include "VADDPS.h"
#include "VADDPD.h"
#include "VDIVSD.h"
#include "VDIVSS.h"
#include "VMULSD.h"
#include "VMULSS.h"
#include "VCVTSS2SD.h"
#include "VCVTSI2SD.h"
#include "VCVTSI2SS.h"
#include "VCVTSD2SS.h"
#include "VCVTTSS2SI.h"
#include "VCVTTSD2SI.h"
#include "VMOVSD.h"
#include "VMOVQ.h"
#include "VEXTRACTF128.h"
#include "VSHUFPS.h"
#include "VSHUFPD.h"
#include "VBROADCASTSS.h"
#include "VMOVDQA.h"
#include "VMOVDQU.h"
#include "VPXOR.h"
#include "VUNPCKHPS.h"
#include "VUNPCKLPS.h"
#include "VXORPD.h"
#include "VPCMPEQQ.h"
#include "VBLENDVPD.h"
#include "VPERM2F128.h"
#include "VANDPD.h"
#include "VANDPS.h"
#include "VMULPD.h"
#include "VMULPS.h"
#include "VSUBPD.h"
#include "VSUBPS.h"
#include "VPSLLQ.h"
#include "VCMPPD.h"
#include "VANDNPD.h"
#include "VANDNPS.h"
#include "VMOVMSKPS.h"
#include "VMOVMSKPD.h"
#include "SHLX.h"
#include "VMOVLHPS.h"
#include "VPERMILPS.h"
#include "VFMADD231PS.h"
#include "VUCOMISS.h"
#include "VUCOMISD.h"
#include "VSQRTPS.h"
#include "VSQRTPD.h"
#include "VHADDPS.h"
#include "VHADDPD.h"
#include "VRSQRTPS.h"
#include "VRSQRTSS.h"
#include "SHRX.h"
#include "ANDN.h"
#include "VFMSUB231PS.h"
#include "VCVTPS2PH.h"
#include "VPEXTRW.h"
#include "VPEXTRQ.h"
#include "VPEXTRD.h"
#include "VPEXTRB.h"
#include "VEXTRACTPS.h"
#include "VMAXSS.h"
#include "VMAXSD.h"
#include "VMINSS.h"
#include "VMINSD.h"
#include "VCOMISS.h"
#include "VCOMISD.h"
#include "VPSHUFB.h"
#include "VMOVD.h"
#include "VADDSS.h"
#include "VADDSD.h"
#include "VPINSRB.h"
#include "VPINSRD.h"
#include "VPINSRQ.h"
#include "VPADDB.h"
#include "VPADDD.h"
#include "VPADDW.h"
#include "VPADDQ.h"
#include "VPSRLQ.h"
#include "SARX.h"
#include "BLSR.h"
#include "VMOVHPD.h"
#include "VMOVHPS.h"
#include "VPBROADCASTB.h"
#include "VPCMPEQD.h"
#include "VPCMPEQW.h"
#include "VPCMPEQB.h"
#include "VPMOVMSKB.h"
#include "VPSIGNB.h"
#include "VPSIGNW.h"
#include "VPSIGND.h"
#include <map>

#ifdef __cplusplus
extern "C" {
#endif
#include "xed/xed-decoded-inst.h"
#include "xed/xed-iclass-enum.h"
#ifdef __cplusplus
}
#endif

typedef std::function<std::shared_ptr<Instruction>(uint64_t, uint8_t, xed_decoded_inst_t)> instrFactory;

#define ICLASSMAP(_instr) {\
    XED_ICLASS_##_instr,  \
    [](uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) -> std::shared_ptr<Instruction> { return std::make_shared<_instr>(rip, ilen, xedd); } \
}

const std::map<xed_iclass_enum_t, instrFactory> iclassMapping = {
    ICLASSMAP(VMOVSS),
    ICLASSMAP(VXORPS),
    ICLASSMAP(VMOVUPS),
    ICLASSMAP(VMOVUPD),
    ICLASSMAP(VMOVAPS),
    ICLASSMAP(VMOVAPD),
    ICLASSMAP(VINSERTPS),
    ICLASSMAP(VINSERTF128),
    ICLASSMAP(VADDPS),
    ICLASSMAP(VADDPD),
    ICLASSMAP(VDIVSD),
    ICLASSMAP(VDIVSS),
    ICLASSMAP(VMULSD),
    ICLASSMAP(VMULSS),
    ICLASSMAP(VCVTSS2SD),
    ICLASSMAP(VCVTSI2SD),
    ICLASSMAP(VCVTSI2SS),
    ICLASSMAP(VCVTSD2SS),
    ICLASSMAP(VCVTTSS2SI),
    ICLASSMAP(VCVTTSD2SI),
    ICLASSMAP(VMOVSD),
    ICLASSMAP(VMOVQ),
    ICLASSMAP(VEXTRACTF128),
    ICLASSMAP(VSHUFPS),
    ICLASSMAP(VSHUFPD),
    ICLASSMAP(VBROADCASTSS),
    ICLASSMAP(VMOVDQA),
    ICLASSMAP(VMOVDQU),
    ICLASSMAP(VPXOR),
    ICLASSMAP(VUNPCKHPS),
    ICLASSMAP(VUNPCKLPS),
    ICLASSMAP(VXORPD),
    ICLASSMAP(VPCMPEQQ),
    ICLASSMAP(VBLENDVPD),
    ICLASSMAP(VPERM2F128),
    ICLASSMAP(VANDPD),
    ICLASSMAP(VANDPS),
    ICLASSMAP(VMULPD),
    ICLASSMAP(VMULPS),
    ICLASSMAP(VSUBPD),
    ICLASSMAP(VSUBPS),
    ICLASSMAP(VPSLLQ),
    ICLASSMAP(VCMPPD),
    ICLASSMAP(VANDNPD),
    ICLASSMAP(VANDNPS),
    ICLASSMAP(VMOVMSKPS),
    ICLASSMAP(VMOVMSKPD),
    ICLASSMAP(SHLX),
    ICLASSMAP(VMOVLHPS),
    ICLASSMAP(VPERMILPS),
    ICLASSMAP(VFMADD231PS),
    ICLASSMAP(VUCOMISS),
    ICLASSMAP(VUCOMISD),
    ICLASSMAP(VSQRTPS),
    ICLASSMAP(VSQRTPD),
    ICLASSMAP(VHADDPS),
    ICLASSMAP(VHADDPD),
    ICLASSMAP(VRSQRTPS),
    ICLASSMAP(VRSQRTSS),
    ICLASSMAP(SHRX),
    ICLASSMAP(ANDN),
    ICLASSMAP(VFMSUB231PS),
    ICLASSMAP(VCVTPS2PH),
    ICLASSMAP(VPEXTRW),
    ICLASSMAP(VPEXTRQ),
    ICLASSMAP(VPEXTRD),
    ICLASSMAP(VPEXTRB),
    ICLASSMAP(VEXTRACTPS),
    ICLASSMAP(VMAXSS),
    ICLASSMAP(VMAXSD),
    ICLASSMAP(VMINSS),
    ICLASSMAP(VMINSD),
    ICLASSMAP(VCOMISS),
    ICLASSMAP(VCOMISD),
    ICLASSMAP(VPSHUFB),
    ICLASSMAP(VMOVD),
    ICLASSMAP(VADDSS),
    ICLASSMAP(VADDSD),
    ICLASSMAP(VPINSRB),
    ICLASSMAP(VPINSRD),
    ICLASSMAP(VPINSRQ),
    ICLASSMAP(VPADDB),
    ICLASSMAP(VPADDW),
    ICLASSMAP(VPADDD),
    ICLASSMAP(VPADDQ),
    ICLASSMAP(VPSRLDQ),
    ICLASSMAP(SARX),
    ICLASSMAP(BLSR),
    ICLASSMAP(VMOVHPD),
    ICLASSMAP(VMOVHPS),
    ICLASSMAP(VPBROADCASTB),
    ICLASSMAP(VPCMPEQD),
    ICLASSMAP(VPCMPEQW),
    ICLASSMAP(VPCMPEQB),
    ICLASSMAP(VPMOVMSKB),
    ICLASSMAP(VPSIGNB),
    ICLASSMAP(VPSIGNW),
    ICLASSMAP(VPSIGND),
};

inline void printSupportedInstructions() {
    for (auto const& [iclass, factory] : iclassMapping) {
        debug_print("%s (%d)\n", xed_iclass_enum_t2str(iclass), iclass);
    }
}
