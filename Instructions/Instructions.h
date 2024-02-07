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
#include "VMULSD.h"
#include "VCVTSS2SD.h"
#include "VCVTSI2SD.h"
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
    ICLASSMAP(VMULSD),
    ICLASSMAP(VCVTSS2SD),
    ICLASSMAP(VCVTSI2SD),
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
    ICLASSMAP(VSUBPD)
};

inline void printSupportedInstructions() {
    for (auto const& [iclass, factory] : iclassMapping) {
        printf("%s (%d)\n", xed_iclass_enum_t2str(iclass), iclass);
    }
}
