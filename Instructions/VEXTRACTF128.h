#include "CompilableInstruction.h"

class VEXTRACTF128 : public CompilableInstruction<VEXTRACTF128> {
public:
    VEXTRACTF128(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        auto value = operands[2].immValue();
        if(value) {
            if (!upper) {
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            }
        } else {
            if (upper) {
                movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
            }
        }
    }
};
