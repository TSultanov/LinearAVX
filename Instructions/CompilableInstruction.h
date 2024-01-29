#pragma once
#include "Instruction.h"
template<class T>
class CompilableInstruction : public Instruction {
protected:
    CompilableInstruction(const xed_decoded_inst_t *xedd) : Instruction(xedd) {}

    std::vector<xed_encoder_request_t> const& compile(ymm_t *ymm) {
        internal_requests.clear();

        implementation(false);

        if (usesYmm()) {
            with_upper_ymm(ymm, [this]{
                implementation(true);
            });
        }

        return internal_requests;
    }

    virtual void implementation(bool upper) = 0;
};
