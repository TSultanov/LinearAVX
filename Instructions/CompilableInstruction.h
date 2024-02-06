#pragma once
#include "Instruction.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-enum.h"
template<class T>
class CompilableInstruction : public Instruction {
protected:
    CompilableInstruction(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : Instruction(rip, ilen, xedd) {}

    std::vector<xed_encoder_request_t> const& compile(ymm_t *ymm, CompilationStrategy compilationStrategy, uint64_t returnAddr = 0) {
        internal_requests.clear();

        if (compilationStrategy == CompilationStrategy::DirectCall) {
            rspOffset = -8;
        }

        implementation(false, compilationStrategy == CompilationStrategy::Inline, ymm);

        if (usesYmm()) {
            with_upper_ymm(ymm, [=, this]{
                implementation(true, compilationStrategy == CompilationStrategy::Inline, ymm);
            });
        }
        
        return internal_requests;
    }

    virtual void implementation(bool upper, bool compile_inline, ymm_t* ymm) = 0;
};
