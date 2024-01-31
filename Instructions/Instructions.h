#pragma once

#include "Instruction.h"
#include "VMOVSS.h"
#include "VMOVUPS.h"
#include "VMOVAPS.h"
#include "VXORPS.h"
#include <map>

#ifdef __cplusplus
extern "C" {
#endif
#include "xed/xed-decoded-inst.h"
#include "xed/xed-iclass-enum.h"
#ifdef __cplusplus
}
#endif

typedef std::function<std::shared_ptr<Instruction>(xed_decoded_inst_t const*)> instrFactory;

#define ICLASSMAP(_instr) {\
    XED_ICLASS_##_instr,  \
    [](xed_decoded_inst_t const* xedd) -> std::shared_ptr<Instruction> { return std::make_shared<_instr>(xedd); } \
}

const std::map<xed_iclass_enum_t, instrFactory> iclassMapping = {
    ICLASSMAP(VMOVSS),
    ICLASSMAP(VXORPS),
    ICLASSMAP(VMOVUPS),
    ICLASSMAP(VMOVAPS)
};