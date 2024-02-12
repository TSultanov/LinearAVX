#include "CompilableInstruction.h"

class VCOMISD : public CompilableInstruction<VCOMISD> {
public:
    VCOMISD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        comisd(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
    }
};