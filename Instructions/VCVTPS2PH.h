#include "CompilableInstruction.h"
#include "xed/xed-encoder-hl.h"
#include "xed/xed-reg-enum.h"

class VCVTPS2PH : public CompilableInstruction<VCVTPS2PH> {
    const uint32_t sign_mask = 0x80000000;
    const uint32_t exp_mask = (0b1111 << 23);
    const uint32_t exp_s_mask = (0b1 << 30);
    const uint32_t fra_mask = (0b1111111111 << 13);

    const __m128 sign_mask_v = _mm_set1_ps(*(float*)&sign_mask);
    const __m128 exp_mask_v = _mm_set1_ps(*(float*)&exp_mask);
    const __m128 exp_s_mask_v = _mm_set1_ps(*(float*)&exp_s_mask);
    const __m128 fra_mask_v = _mm_set1_ps(*(float*)&fra_mask);
public:
    VCVTPS2PH(uint64_t rip, uint8_t ilen, xed_decoded_inst_t xedd) : CompilableInstruction(rip, ilen, xedd) {
    }
private:
    /*
__asm__ volatile("movups %1, %%xmm0;\n" // move input into xmm0
                 "xorps %%xmm1, %%xmm1;\n" // zero out xmm1 (result)

                 "movups %%xmm0, %%xmm3;\n" //move input into xmm3 (temp)
                 "pand %2, %%xmm3;\n" // bitwise and temp and sign_mask
                 "psrld $16, %%xmm3;\n" // shift right temp by 16
                 "por %%xmm3, %%xmm1;\n" // bitwise or temp and result

                 "movups %%xmm0, %%xmm3;\n" //move input into xmm3 (temp)
                 "pand %3, %%xmm3;\n" // bitwise and temp and exp_mask
                 "psrld $13, %%xmm3;\n" // shift right temp by 13
                 "por %%xmm3, %%xmm1;\n" // bitwise or temp and result

                 "movups %%xmm0, %%xmm3;\n" //move input into xmm3 (temp)
                 "pand %4, %%xmm3;\n" // bitwise and temp and exp_s_mask
                 "psrld $16, %%xmm3;\n" // shift right temp by 16
                 "por %%xmm3, %%xmm1;\n" // bitwise or temp and result

                 "movups %%xmm0, %%xmm3;\n" //move input into xmm3 (temp)
                 "pand %5, %%xmm3;\n" // bitwise and temp and fra_mask
                 "psrld $13, %%xmm3;\n" // shift right temp by 13
                 "por %%xmm3, %%xmm1;\n" // bitwise or temp and result

                 "pshufhw $0xf8, %%xmm1, %%xmm1;\n" //shuffle hi word
                 "pshuflw $0xf8, %%xmm1, %%xmm1;\n" //shuffle lo word

                 "xorps %%xmm3, %%xmm3;"
                 "shufps $0x08, %%xmm3, %%xmm1;\n" // shuffle dwords

                 "movups %%xmm1, %0;\n" // write output

                : "=m"(output)
                : "m"(input),
                  "m"(sign_mask_v),
                  "m"(exp_mask_v),
                  "m"(exp_s_mask_v),
                  "m"(fra_mask_v)
                : "%xmm0","%xmm1","%xmm3");
    */
    void implementation(bool upper, bool compile_inline) {
        if (usesYmm()) {
            debug_print("VCVTPS2PH: YMM not supported\n");
            exit(1);
        }

        if (!upper) {
            auto input = operands[1].toEncoderOperand(upper);
            auto inputReg = getUnusedXmmReg();
            auto outputReg = operands[0].toEncoderOperand(upper);
            auto tempReg = getUnusedXmmReg();

            withFreeReg([=](xed_reg_enum_t tempGpr) {
                movups(xed_reg(inputReg), input);
                xorps(outputReg, outputReg);

                // get sign
                movups(xed_reg(tempReg), xed_reg(inputReg));
                mov(tempGpr, (uint64_t)&sign_mask_v);
                pand(xed_reg(tempReg), xed_mem_b(tempGpr, 128));
                psrld(xed_reg(tempReg), xed_imm0(16, 8));
                por(outputReg, xed_reg(tempReg));

                // get exponent
                movups(xed_reg(tempReg), xed_reg(inputReg));
                mov(tempGpr, (uint64_t)&exp_mask_v);
                pand(xed_reg(tempReg), xed_mem_b(tempGpr, 128));
                psrld(xed_reg(tempReg), xed_imm0(13, 8));
                por(outputReg, xed_reg(tempReg));

                // get exponent sign
                movups(xed_reg(tempReg), xed_reg(inputReg));
                mov(tempGpr, (uint64_t)&exp_s_mask_v);
                pand(xed_reg(tempReg), xed_mem_b(tempGpr, 128));
                psrld(xed_reg(tempReg), xed_imm0(16, 8));
                por(outputReg, xed_reg(tempReg));

                // get fraction
                movups(xed_reg(tempReg), xed_reg(inputReg));
                mov(tempGpr, (uint64_t)&fra_mask_v);
                pand(xed_reg(tempReg), xed_mem_b(tempGpr, 128));
                psrld(xed_reg(tempReg), xed_imm0(13, 8));
                por(outputReg, xed_reg(tempReg));

                // Compact everything into lower quadword
                pshufhw(outputReg, outputReg, xed_imm0(0xf8, 8));
                pshuflw(outputReg, outputReg, xed_imm0(0xf8, 8));

                xorps(xed_reg(tempReg), xed_reg(tempReg));
                shufps(outputReg, xed_reg(tempReg), xed_imm0(0x08, 8));
            });
            

            returnReg(tempReg);
            returnReg(inputReg); 
        }
    }
};