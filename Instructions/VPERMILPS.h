#include "CompilableInstruction.h"

class VPERMILPS : public CompilableInstruction<VPERMILPS> {
public:
    VPERMILPS(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        if (!operands[2].isImmediate()) {
            printf("VPERMILPS: only immeddiate SRC2 (operand 2) argument is supported!\n");
            exit(1);
        }

        movups(operands[0].toEncoderOperand(upper), operands[1].toEncoderOperand(upper));
        shufps(
            operands[0].toEncoderOperand(upper),
            operands[1].toEncoderOperand(upper),
            operands[2].toEncoderOperand(upper));


        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};
