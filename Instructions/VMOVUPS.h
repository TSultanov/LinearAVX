#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-decoded-inst.h"
#include "xed/xed-encode.h"
#include <vector>

#include "Operand.h"
#include "Instruction.h"

class VMOVUPS : public Instruction {
    public:
    VMOVUPS(xed_decoded_inst_t *xedd) : Instruction(xedd) {

    }

    void compile(ymm_t *ymm) {
        // TODO

        if (usesYmm()) {
            with_upper_ymm(ymm, [this]{
                // TODO
            });
        }
    }

private:
    void implementation() {
        // 
    }
};