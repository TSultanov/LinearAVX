use iced_x86::EncodingKind;
use itertools::Itertools;

use super::{table::get_mapping, Instruction, Mnemonic, Operand, Register, VirtualRegister};

fn add_zeroupper(vec: Vec<Instruction>, op: Operand) -> Vec<Instruction> {
    let mut result = Vec::new();
    for i in vec {
        result.push(i)
    }
    result.push(Instruction {
        original_instr: None, //result.last().unwrap().original_instr,
        original_ip: result.last().unwrap().original_ip,
        original_next_ip: result.last().unwrap().original_next_ip,
        operands: vec![op],
        flow_control: iced_x86::FlowControl::Next,
        target_mnemonic: Mnemonic::Regzero,
    });
    result
}

impl Instruction {
    pub fn detect_high_loads(&self) -> Vec<Instruction> {
        let input_high_xmm = self.get_input_regs().into_iter().filter(|r| r.is_xmm_hi());
        let output_high_xmm = self
            .get_output_regs()
            .into_iter()
            .filter(|(r, _)| r.is_xmm_hi())
            .map(|(r, _)| r);
        let high_xmms = input_high_xmm.chain(output_high_xmm).unique();

        let operands: Vec<_> = high_xmms.map(|r| {
            Operand::Register(r)
        }).collect();

        let loads = operands.iter().map(|o| {
            Instruction {
                target_mnemonic: Mnemonic::SwapInHighYmm,
                operands: vec![*o],
                ..self.clone()
            }
        });
        let instr = std::iter::once(self.clone());
        let stores = operands.iter().map(|o| {
            Instruction {
                target_mnemonic: Mnemonic::SwapOutHighYmm,
                operands: vec![*o],
                ..self.clone()
            }
        });

        loads.chain(instr).chain(stores).collect_vec()
    }

    pub fn map(&self) -> Vec<Instruction> {
        if let Some(original_instr) = self.original_instr {
            if original_instr.encoding() == EncodingKind::VEX {
                match self.target_mnemonic {
                    super::Mnemonic::Real(mnemonic) => match get_mapping(mnemonic) {
                        Some(m) => m.map(&self),
                        None => {
                            println!("map: {:?} not implemented", mnemonic);
                            todo!();
                        }
                    },
                    super::Mnemonic::Regzero => {
                        println!("Regzero! TODO FIXME");
                        todo!()
                    }
                    Mnemonic::SwapInHighYmm => {
                        println!("SwapInHighYmm! TODO FIXME");
                        todo!()
                    }
                    Mnemonic::SwapOutHighYmm => {
                        println!("SwapOutHighYmm! TODO FIXME");
                        todo!()
                    }
                }
            } else {
                vec![self.clone()]
            }
        } else {
            todo!()
        }
    }

    pub fn eliminate_ymm_by_splitting(&self, zeroupper: bool) -> Vec<Self> {
        if !self.has_ymm() {
            // This is cursed
            return if let Some(first_op) = self.operands.first() {
                if zeroupper {
                    match first_op {
                        Operand::Register(reg) => match reg {
                            Register::Native(reg) => {
                                if reg.is_xmm() {
                                    let reg = Register::Virtual(VirtualRegister::high_for_xmm(reg));
                                    add_zeroupper(vec![self.clone()], Operand::Register(reg))
                                } else {
                                    vec![self.clone()]
                                }
                            }
                            Register::Virtual(_) => vec![self.clone()],
                        },
                        _ => vec![self.clone()],
                    }
                } else {
                    vec![self.clone()]
                }
            } else {
                vec![self.clone()]
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
                            Operand::Register(VirtualRegister::pair_for_ymm(reg).0)
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
            original_instr: self.original_instr,
            original_ip: self.original_ip,
            original_next_ip: self.original_next_ip,
            target_mnemonic: self.target_mnemonic.clone(),
            flow_control: iced_x86::FlowControl::Next,
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
                            Operand::Register(VirtualRegister::pair_for_ymm(reg).1)
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
            original_instr: self.original_instr,
            original_ip: self.original_ip,
            original_next_ip: self.original_next_ip,
            target_mnemonic: self.target_mnemonic,
            flow_control: iced_x86::FlowControl::Next,
            operands: operands,
        };

        vec![first_instr, second_instr]
    }
}
