#pragma once
#include "Instruction.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-enum.h"
template<class T>
class CompilableInstruction : public Instruction {
protected:
    CompilableInstruction(uint64_t rip, uint64_t rsp, const xed_decoded_inst_t *xedd) : Instruction(rip, rsp, xedd) {}

    std::vector<xed_encoder_request_t> const& compile(ymm_t *ymm, CompilationStrategy compilationStrategy, uint64_t returnAddr = 0) {
        internal_requests.clear();

        // if (compilationStrategy == CompilationStrategy::ExceptionCall) {
        //     push(xed_imm0(returnAddr, 64));
        // }

        implementation(false, compilationStrategy == CompilationStrategy::Inline, ymm);

        if (usesYmm()) {
            with_upper_ymm(ymm, [=, this]{
                implementation(true, compilationStrategy == CompilationStrategy::Inline, ymm);
            });
        }

        if (compilationStrategy != CompilationStrategy::Inline) {
            ret();
        }
        
        return internal_requests;
    }

    virtual void implementation(bool upper, bool compile_inline, ymm_t* ymm) = 0;
};
