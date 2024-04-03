use std::{cell::OnceCell, collections::HashMap};

use crate::tir::instruction;
use iced_x86::Mnemonic;

use super::{Instruction, Operand, Register};

pub struct Mapping {
    pub mnemonic: Mnemonic,
    map: &'static dyn Fn(&Instruction) -> Vec<Instruction>,
}

impl Mapping {
    pub fn map(&self, i: &Instruction) -> Vec<Instruction> {
        (self.map)(i)
    }
}

fn with_preserve_xmm_reg(
    i: &Instruction,
    reg: Operand,
    instr: Vec<Instruction>,
) -> Vec<Instruction> {
    let rsp = Operand::Register(Register::Native(iced_x86::Register::RSP));
    let start = vec![
        Instruction {
            target_mnemonic: instruction::Mnemonic::Real(iced_x86::Mnemonic::Sub),
            operands: vec![rsp, Operand::Immediate(16)],
            ..i.clone()
        },
        Instruction {
            target_mnemonic: instruction::Mnemonic::Real(iced_x86::Mnemonic::Movdqu),
            operands: vec![
                Operand::Memory {
                    base: iced_x86::Register::RSP,
                    index: iced_x86::Register::None,
                    scale: 0,
                    displacement: 0,
                },
                reg,
            ],
            ..i.clone()
        },
    ];

    let finish = vec![
        Instruction {
            target_mnemonic: instruction::Mnemonic::Real(iced_x86::Mnemonic::Movdqu),
            operands: vec![
                reg,
                Operand::Memory {
                    base: iced_x86::Register::RSP,
                    index: iced_x86::Register::None,
                    scale: 0,
                    displacement: 0,
                },
            ],
            ..i.clone()
        },
        Instruction {
            target_mnemonic: instruction::Mnemonic::Real(iced_x86::Mnemonic::Add),
            operands: vec![rsp, Operand::Immediate(16)],
            ..i.clone()
        },
    ];

    start
        .into_iter()
        .chain(instr.into_iter())
        .chain(finish.into_iter())
        .collect()
}

fn map3opto2op(i: &Instruction, new_mnemonic: iced_x86::Mnemonic) -> Vec<Instruction> {
    let tm = instruction::Mnemonic::Real(new_mnemonic);
    if i.operands.len() == 2 {
        vec![Instruction {
            target_mnemonic: tm,
            original_instr: None,
            ..i.clone()
        }]
    } else if i.operands.len() == 3 {
        if i.operands[0] == i.operands[1] {
            vec![Instruction {
                target_mnemonic: tm,
                original_instr: None,
                operands: vec![i.operands[0], i.operands[2]],
                ..i.clone()
            }]
        } else if i.operands[0] == i.operands[2] {
            with_preserve_xmm_reg(
                i,
                i.operands[1],
                vec![
                    Instruction {
                        target_mnemonic: tm,
                        original_instr: None,
                        operands: vec![i.operands[1], i.operands[2]],
                        ..i.clone()
                    },
                    Instruction {
                        target_mnemonic: instruction::Mnemonic::Real(iced_x86::Mnemonic::Movups),
                        original_instr: None,
                        operands: vec![i.operands[0], i.operands[1]],
                        ..i.clone()
                    },
                ],
            )
        } else {
            vec![
                Instruction {
                    target_mnemonic: instruction::Mnemonic::Real(iced_x86::Mnemonic::Movups),
                    original_instr: None,
                    operands: vec![i.operands[0], i.operands[1]],
                    ..i.clone()
                },
                Instruction {
                    target_mnemonic: tm,
                    original_instr: None,
                    operands: vec![i.operands[0], i.operands[2]],
                    ..i.clone()
                },
            ]
        }
    } else {
        println!("{:?}", i);
        todo!();
    }
}

pub fn get_mapping(m: Mnemonic) -> Option<&'static Mapping> {
    let index_mapping = unsafe {
        MAPPING.get_or_init(|| {
            let mut mapping = HashMap::new();
            for (i, m) in (0..).zip(MAPPINGS_VEC) {
                mapping.insert(m.mnemonic, i);
            }
            mapping
        })
    };

    if let Some(idx) = index_mapping.get(&m) {
        Some(&MAPPINGS_VEC[*idx])
    } else {
        None
    }
}

static mut MAPPING: OnceCell<HashMap<Mnemonic, usize>> = OnceCell::new();

const MAPPINGS_VEC: &[Mapping] = &[
    Mapping {
        mnemonic: Mnemonic::Vmovups,
        map: &|i| {
            let ops = i.eliminate_ymm_by_splitting(true);

            ops.iter()
                .map(|i| Instruction {
                    target_mnemonic: iced_x86::Mnemonic::Movups.into(),
                    original_instr: None,
                    ..i.clone()
                })
                .collect()
        },
    },
    Mapping {
        mnemonic: Mnemonic::Vmovsd,
        map: &|i| {
            let ops = i.eliminate_ymm_by_splitting(true);

            ops.iter()
                .flat_map(|i| match i.target_mnemonic {
                    instruction::Mnemonic::Real(_) => map3opto2op(i, iced_x86::Mnemonic::Movsd),
                    instruction::Mnemonic::Regzero => vec![i.clone()],
                    instruction::Mnemonic::SwapInHighYmm => todo!(),
                    instruction::Mnemonic::SwapOutHighYmm => todo!(),
                })
                .collect()
        },
    },
    Mapping {
        mnemonic: Mnemonic::Vmovq,
        map: &|i| {
            let new_op = Instruction {
                target_mnemonic: iced_x86::Mnemonic::Movq.into(),
                original_instr: None,
                ..i.clone()
            };
            let no_ymm = new_op.eliminate_ymm_by_splitting(true);
            no_ymm
        },
    },
];
