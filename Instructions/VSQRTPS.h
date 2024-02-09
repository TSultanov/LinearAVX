#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VSQRTPS : public CompilableInstruction<VSQRTPS> {
public:
    VSQRTPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        sqrtps(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};