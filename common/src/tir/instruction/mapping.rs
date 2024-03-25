use iced_x86::EncodingKind;
use itertools::Itertools;

use super::{Instruction, Mnemonic, Operand, Register, VirtualRegister};

fn add_zeroupper(vec: Vec<Instruction>, op: Operand) -> Vec<Instruction> {
    let mut result = Vec::new();
    for i in vec {
        result.push(i)
    }
    result.push(Instruction {
        original_instr: result.last().unwrap().original_instr,
        original_ip: result.last().unwrap().original_ip,
        original_next_ip: result.last().unwrap().original_next_ip,
        operands: vec![op],
        flow_control: iced_x86::FlowControl::Next,
        target_mnemonic: Mnemonic::Regzero,
    });
    result
}

impl Instruction {
    fn with_preserve_xmm_reg(&self, reg: Operand, instr: Vec<Instruction>) -> Vec<Instruction> {
        let rsp = Operand::Register(Register::Native(iced_x86::Register::RSP));
        let start = vec![
            Self {
                target_mnemonic: Mnemonic::Real(iced_x86::Mnemonic::Sub),
                operands: vec![rsp, Operand::Immediate(16)],
                ..self.clone()
            },
            Self {
                target_mnemonic: Mnemonic::Real(iced_x86::Mnemonic::Movdqu),
                operands: vec![
                    Operand::Memory {
                        base: iced_x86::Register::RSP,
                        index: iced_x86::Register::None,
                        scale: 0,
                        displacement: 0,
                    },
                    reg,
                ],
                ..self.clone()
            },
        ];

        let finish = vec![
            Self {
                target_mnemonic: Mnemonic::Real(iced_x86::Mnemonic::Movdqu),
                operands: vec![
                    reg,
                    Operand::Memory {
                        base: iced_x86::Register::RSP,
                        index: iced_x86::Register::None,
                        scale: 0,
                        displacement: 0,
                    },
                ],
                ..self.clone()
            },
            Self {
                target_mnemonic: Mnemonic::Real(iced_x86::Mnemonic::Add),
                operands: vec![rsp, Operand::Immediate(16)],
                ..self.clone()
            },
        ];

        start
            .into_iter()
            .chain(instr.into_iter())
            .chain(finish.into_iter())
            .collect_vec()
    }

    fn map3opto2op(&self, new_mnemonic: iced_x86::Mnemonic) -> Vec<Instruction> {
        let tm = Mnemonic::Real(new_mnemonic);
        if self.operands.len() == 2 {
            vec![Self {
                target_mnemonic: tm,
                ..self.clone()
            }]
        } else if self.operands.len() == 3 {
            if self.operands[0] == self.operands[1] {
                vec![Self {
                    target_mnemonic: tm,
                    operands: vec![self.operands[0], self.operands[2]],
                    ..self.clone()
                }]
            } else if self.operands[0] == self.operands[2] {
                self.with_preserve_xmm_reg(
                    self.operands[1],
                    vec![
                        Self {
                            target_mnemonic: tm,
                            operands: vec![self.operands[1], self.operands[2]],
                            ..self.clone()
                        },
                        Self {
                            target_mnemonic: Mnemonic::Real(iced_x86::Mnemonic::Movups),
                            operands: vec![self.operands[0], self.operands[1]],
                            ..self.clone()
                        },
                    ],
                )
            } else {
                vec![
                    Self {
                        target_mnemonic: Mnemonic::Real(iced_x86::Mnemonic::Movups),
                        operands: vec![self.operands[0], self.operands[1]],
                        ..self.clone()
                    },
                    Self {
                        target_mnemonic: tm,
                        operands: vec![self.operands[0], self.operands[2]],
                        ..self.clone()
                    },
                ]
            }
        } else {
            println!("{:?}", self);
            todo!();
        }
    }

    pub fn map(&self) -> Vec<Instruction> {
        if self.original_instr.encoding() == EncodingKind::VEX {
            match self.target_mnemonic {
                super::Mnemonic::Real(mnemonic) => match mnemonic {
                    iced_x86::Mnemonic::INVALID => panic!("Invalid instruction"),
                    iced_x86::Mnemonic::Vmovups => {
                        let ops = self.eliminate_ymm_by_splitting(true);

                        ops.iter()
                            .flat_map(|i| match i.target_mnemonic {
                                Mnemonic::Real(_) => i.map3opto2op(iced_x86::Mnemonic::Movsd),
                                Mnemonic::Regzero => vec![i.clone()],
                            })
                            .collect()
                    }
                    iced_x86::Mnemonic::Vmovsd => {
                        let ops = self.eliminate_ymm_by_splitting(true);

                        ops.iter()
                            .flat_map(|i| match i.target_mnemonic {
                                Mnemonic::Real(_) => i.map3opto2op(iced_x86::Mnemonic::Movsd),
                                Mnemonic::Regzero => vec![i.clone()],
                            })
                            .collect()
                    }
                    iced_x86::Mnemonic::Vmovq => {
                        let new_op = Self {
                            target_mnemonic: Mnemonic::Real(iced_x86::Mnemonic::Movq),
                            ..self.clone()
                        };
                        let no_ymm = new_op.eliminate_ymm_by_splitting(true);
                        no_ymm
                    }
                    _ => {
                        println!("map: \"{:?}\" not implemented", mnemonic);
                        todo!();
                    }
                },
                super::Mnemonic::Regzero => {
                    println!("Regzero! TODO FIXME");
                    todo!()
                }
            }
        } else {
            vec![self.clone()]
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
