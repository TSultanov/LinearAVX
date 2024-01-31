#pragma once
#include "Instruction.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-enum.h"
template<class T>
class CompilableInstruction : public Instruction {
protected:
    CompilableInstruction(const xed_decoded_inst_t *xedd) : Instruction(xedd) {}

    std::vector<xed_encoder_request_t> const& compile(ymm_t *ymm, CompilationStrategy compilationStrategy, uint64_t returnAddr = 0) {
        internal_requests.clear();

        // if (compilationStrategy == CompilationStrategy::ExceptionCall) {
        //     push(xed_imm0(returnAddr, 64));
        // }

        implementation(false, compilationStrategy == CompilationStrategy::Inline);

        if (usesYmm()) {
            with_upper_ymm(ymm, [=, this]{
                implementation(true, compilationStrategy == CompilationStrategy::Inline);
            });
        }

        if (compilationStrategy != CompilationStrategy::Inline) {
            ret();
        }
        
        return internal_requests;
    }

    virtual void implementation(bool upper, bool compile_inline) = 0;
};
