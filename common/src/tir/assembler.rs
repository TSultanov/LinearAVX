use std::error::Error;

use iced_x86::{
    code_asm::{oword_ptr, xmmword_ptr, AsmRegister64, CodeAssembler},
    IcedError, Instruction, MemoryOperand,
};

use crate::compiler::{ControlBlock, FunctionBlock};

pub fn assemble(fb: FunctionBlock) -> Result<(), Box<dyn Error>> {
    let mut a = CodeAssembler::new(64)?;

    for cb in fb.control_blocks {
        assemble_codeblock(&mut a, &cb)?;
    }

    Ok(())
}

pub fn assemble_codeblock(a: &mut CodeAssembler, cb: &ControlBlock) -> Result<(), IcedError> {
    for (i, _) in &cb.instructions {
        assemble_instruction(a, i)?;
    }

    Ok(())
}

pub fn assemble_instruction(
    a: &mut CodeAssembler,
    i: &super::Instruction,
) -> Result<(), IcedError> {
    match i.target_mnemonic {
        super::Mnemonic::Real(m) => match i.original_instr {
            Some(original_instr) => {
                let mut instr = original_instr.clone();
                instr.set_ip(0);
                a.add_instruction(instr)
            }
            None => assemble_translated(a, m, i),
        },
        super::Mnemonic::Regzero => assemble_regzero(a),
        super::Mnemonic::SwapInHighYmm => todo!(),
        super::Mnemonic::SwapOutHighYmm => todo!(),
    }
}

fn assemble_regzero(a: &CodeAssembler) -> Result<(), IcedError> {
    todo!();
}

fn assemble_translated(
    a: &mut CodeAssembler,
    m: iced_x86::Mnemonic,
    i: &super::Instruction,
) -> Result<(), IcedError> {
    println!("Encoding {:?}", i);
    match m {
        iced_x86::Mnemonic::Movsd => match (i.operands[0], i.operands[1]) {
            (super::Operand::Register(_), super::Operand::Register(_)) => todo!(),
            (
                super::Operand::Register(reg),
                super::Operand::Memory {
                    base,
                    index,
                    scale,
                    displacement,
                },
            ) => match reg {
                super::Register::Native(reg) => Ok(a.add_instruction(Instruction::with2(
                    iced_x86::Code::Movsd_xmm_xmmm64,
                    reg,
                    MemoryOperand::with_base_index_scale_displ_size(
                        base,
                        index,
                        scale,
                        displacement as i64,
                        1,
                    ),
                )?)?),
                super::Register::Virtual(_) => todo!(),
            },
            (
                super::Operand::Memory {
                    base,
                    index,
                    scale,
                    displacement,
                },
                super::Operand::Register(_),
            ) => todo!(),
            _ => {
                todo!()
            }
        },
        _ => {
            panic!("assembler: {:?} not implemented", m);
        }
    }
}
