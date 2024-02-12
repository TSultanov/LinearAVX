#include "CompilableInstruction.h"
#include "Operand.h"
#include "xed/xed-encoder-hl.h"

class VCMPPD : public CompilableInstruction<VCMPPD> {
public:
    VCMPPD(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {}
private:
    void implementation(bool upper, bool compile_inline) {
        auto mode = operands[3].immValue();

        if (mode <= 0x07) {
            map3opto2op(upper, [=](xed_encoder_operand_t const& op0, xed_encoder_operand_t const& op1) {
                cmppd(op0, op1, operands[3].toEncoderOperand(upper));
            });
        } else
        // special case only mode 0x16 for now
        if (mode == 0x16) {
            // map to mode 0x06. It will signal on NaN unlike 0x16
            map3opto2op(upper, [=](xed_encoder_operand_t const& op0, xed_encoder_operand_t const& op1) {
                cmppd(op0, op1, xed_imm0(0x06, 8));
            });
        } else {
            debug_print("VCMPPD: Unsupported mode %llx\n", mode);
            exit(1);
        }

        if (operands[0].isXmm()) {
            zeroupperInternal(operands[0]);
        }
    }
};