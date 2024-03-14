use std::iter;

use super::{Instruction, Operand, Register, VirtualRegister};
use iced_x86::Mnemonic;

fn add_zeroupper(vec: Vec<Instruction>) -> Vec<Instruction> {
    let mut result = Vec::new();
    for i in vec {
        result.push(i)
    }
    result.push(Instruction {
        original_ip: result.last().unwrap().original_ip,
        operands: vec![],
        target_mnemonic: Mnemonic::Vzeroupper,
    });
    result
}

impl Instruction {
    pub fn map(self) -> Vec<Instruction> {
        match self.target_mnemonic {
            Mnemonic::INVALID => panic!("Invalid instruction"),
            Mnemonic::Vmovups => {
                let new_opcode = Self {
                    target_mnemonic: Mnemonic::Movups,
                    ..self
                };
                let no_ymm = new_opcode.eliminate_ymm_by_splitting(true);
                no_ymm
            }
            _ => iter::once(self).collect(),
        }
    }

    pub fn eliminate_ymm_by_splitting(self, zeroupper: bool) -> Vec<Self> {
        if !self.has_ymm() {
            return if zeroupper {
                add_zeroupper(vec![self])
            } else {
                vec![self]
            };
        }

        // First, handle lower portion of register
        let operands = self
            .operands
            .iter()
            .map(|o| match o {
                Operand::Register(reg) => match reg {
                    Register::Native(reg) => {
                        if reg.is_ymm() {
                            Operand::Register(Register::Virtual(VirtualRegister::pair_for_ymm(reg).0))
                        } else {
                            *o
                        }
                    }
                    _ => *o,
                },
                _ => *o,
            })
            .collect();

        let first_instr = Instruction {
            original_ip: self.original_ip,
            target_mnemonic: self.target_mnemonic,
            operands: operands,
        };

        // Next, the upper part
        let operands = self
            .operands
            .iter()
            .map(|o| match o {
                Operand::Register(reg) => match reg {
                    Register::Native(reg) => {
                        if reg.is_ymm() {
                            Operand::Register(Register::Virtual(VirtualRegister::pair_for_ymm(reg).1))
                        } else {
                            *o
                        }
                    }
                    _ => *o,
                },
                Operand::Memory {
                    base,
                    index,
                    scale,
                    displacement,
                } => Operand::Memory {
                    base: *base,
                    index: *index,
                    scale: *scale,
                    displacement: *displacement + 16,
                },
                _ => *o,
            })
            .collect();
        let second_instr = Instruction {
            original_ip: self.original_ip,
            target_mnemonic: self.target_mnemonic,
            operands: operands,
        };

        vec![first_instr, second_instr]
    }
}
