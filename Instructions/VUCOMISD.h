#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"

class VUCOMISD : public CompilableInstruction<VUCOMISD> {
public:
    VUCOMISD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        ucomisd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};