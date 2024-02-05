#pragma once

#include "Instruction.h"
#include "VMOVSS.h"
#include "VMOVUPS.h"
#include "VMOVAPS.h"
#include "VXORPS.h"
#include "VINSERTPS.h"
#include "VINSERTF128.h"
#include "VADDPS.h"
#include "VCVTSS2SD.h"
#include "VMOVSD.h"
#include "VMOVQ.h"
#include "VEXTRACTF128.h"
#include "VSHUFPS.h"
#include "VBROADCASTSS.h"
#include "VMOVDQA.h"
#include <map>

#ifdef __cplusplus
extern "C" {
#endif
#include "xed/xed-decoded-inst.h"
#include "xed/xed-iclass-enum.h"
#ifdef __cplusplus
}
#endif

typedef std::function<std::shared_ptr<Instruction>(uint64_t, xed_decoded_inst_t const*)> instrFactory;

#define ICLASSMAP(_instr) {\
    XED_ICLASS_##_instr,  \
    [](uint64_t rip, xed_decoded_inst_t const* xedd) -> std::shared_ptr<Instruction> { return std::make_shared<_instr>(rip, xedd); } \
}

const std::map<xed_iclass_enum_t, instrFactory> iclassMapping = {
    ICLASSMAP(VMOVSS),
    ICLASSMAP(VXORPS),
    ICLASSMAP(VMOVUPS),
    ICLASSMAP(VMOVAPS),
    ICLASSMAP(VINSERTPS),
    ICLASSMAP(VINSERTF128),
    ICLASSMAP(VADDPS),
    ICLASSMAP(VCVTSS2SD),
    ICLASSMAP(VMOVSD),
    ICLASSMAP(VMOVQ),
    ICLASSMAP(VEXTRACTF128),
    ICLASSMAP(VSHUFPS),
    ICLASSMAP(VBROADCASTSS),
    ICLASSMAP(VMOVDQA)
};