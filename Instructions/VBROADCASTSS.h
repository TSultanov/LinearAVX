#include "CompilableInstruction.h"
#include "xed/xed-decoded-inst-api.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-operand-accessors.h"
#include <unistd.h>

class VBROADCASTSS : public CompilableInstruction<VBROADCASTSS> {
public:
    VBROADCASTSS(uint64_t rip, const xed_decoded_inst_t *xedd) : CompilableInstruction(rip, xedd) {}
private:
    void implementation(bool upper, bool compile_inline, ymm_t *ymm) override {
        for (int i = 0; i < 4; i++) {
            insertps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(false), xed_imm0(i*16, 8));
        }
    }
};
