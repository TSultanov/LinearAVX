#pragma once
#include "Instruction.h"
template<class T>
class CompilableInstruction : public Instruction {
protected:
    CompilableInstruction(const xed_decoded_inst_t *xedd) : Instruction(xedd) {}

    std::vector<xed_encoder_request_t> const& compile(ymm_t *ymm, bool compile_inline = false) {
        internal_requests.clear();

        implementation(false, compile_inline);

        if (usesYmm()) {
            with_upper_ymm(ymm, [=, this]{
                implementation(true, compile_inline);
            });
        }

        if (!compile_inline) {
            ret();
        }
        
        return internal_requests;
    }

    virtual void implementation(bool upper, bool compile_inline) = 0;
};
