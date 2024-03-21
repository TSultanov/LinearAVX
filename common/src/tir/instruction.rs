use std::collections::HashSet;

use self::mapping::{get_prod_cons, RegProdCons};
use enum_iterator::{all, Sequence};
use iced_x86::EncodingKind;
pub mod mapping;

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash, Sequence)]
pub enum VirtualRegister {
    XMM0H,
    XMM1H,
    XMM2H,
    XMM3H,
    XMM4H,
    XMM5H,
    XMM6H,
    XMM7H,
    XMM8H,
    XMM9H,
    XMM10H,
    XMM11H,
    XMM12H,
    XMM13H,
    XMM14H,
    XMM15H,
}

impl VirtualRegister {
    pub fn high_for_xmm(reg: &iced_x86::Register) -> Self {
        match reg {
            iced_x86::Register::XMM0 => Self::XMM0H,
            iced_x86::Register::XMM1 => Self::XMM1H,
            iced_x86::Register::XMM2 => Self::XMM2H,
            iced_x86::Register::XMM3 => Self::XMM3H,
            iced_x86::Register::XMM4 => Self::XMM4H,
            iced_x86::Register::XMM5 => Self::XMM5H,
            iced_x86::Register::XMM6 => Self::XMM6H,
            iced_x86::Register::XMM7 => Self::XMM7H,
            iced_x86::Register::XMM8 => Self::XMM8H,
            iced_x86::Register::XMM9 => Self::XMM9H,
            iced_x86::Register::XMM10 => Self::XMM10H,
            iced_x86::Register::XMM11 => Self::XMM11H,
            iced_x86::Register::XMM12 => Self::XMM12H,
            iced_x86::Register::XMM13 => Self::XMM13H,
            iced_x86::Register::XMM14 => Self::XMM14H,
            iced_x86::Register::XMM15 => Self::XMM15H,
            _ => {
                panic!("Unsupported register {:?}!", reg);
            }
        }
    }

    pub fn pair_for_ymm(reg: &iced_x86::Register) -> (Register, Register) {
        let p = match reg {
            iced_x86::Register::YMM0 => (iced_x86::Register::XMM0, Self::XMM0H),
            iced_x86::Register::YMM1 => (iced_x86::Register::XMM1, Self::XMM1H),
            iced_x86::Register::YMM2 => (iced_x86::Register::XMM2, Self::XMM2H),
            iced_x86::Register::YMM3 => (iced_x86::Register::XMM3, Self::XMM3H),
            iced_x86::Register::YMM4 => (iced_x86::Register::XMM4, Self::XMM4H),
            iced_x86::Register::YMM5 => (iced_x86::Register::XMM5, Self::XMM5H),
            iced_x86::Register::YMM6 => (iced_x86::Register::XMM6, Self::XMM6H),
            iced_x86::Register::YMM7 => (iced_x86::Register::XMM7, Self::XMM7H),
            iced_x86::Register::YMM8 => (iced_x86::Register::XMM8, Self::XMM8H),
            iced_x86::Register::YMM9 => (iced_x86::Register::XMM9, Self::XMM9H),
            iced_x86::Register::YMM10 => (iced_x86::Register::XMM10, Self::XMM10H),
            iced_x86::Register::YMM11 => (iced_x86::Register::XMM11, Self::XMM11H),
            iced_x86::Register::YMM12 => (iced_x86::Register::XMM12, Self::XMM12H),
            iced_x86::Register::YMM13 => (iced_x86::Register::XMM13, Self::XMM13H),
            iced_x86::Register::YMM14 => (iced_x86::Register::XMM14, Self::XMM14H),
            iced_x86::Register::YMM15 => (iced_x86::Register::XMM15, Self::XMM15H),
            _ => {
                panic!("Unsupported register {:?}!", reg);
            }
        };

        (Register::Native(p.0), Register::Virtual(p.1))
    }
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub enum RegisterValue {
    Unknown,
    Zero,
}

#[derive(Debug, Copy, Clone, PartialEq, Eq, Hash)]
pub enum Register {
    Native(iced_x86::Register),
    Virtual(VirtualRegister),
}

impl Register {
    pub fn is_xmm_hi(&self) -> bool {
        match self {
            Register::Native(reg) => false,
            Register::Virtual(reg) => match reg {
                VirtualRegister::XMM0H
                | VirtualRegister::XMM1H
                | VirtualRegister::XMM2H
                | VirtualRegister::XMM3H
                | VirtualRegister::XMM4H
                | VirtualRegister::XMM5H
                | VirtualRegister::XMM6H
                | VirtualRegister::XMM7H
                | VirtualRegister::XMM8H
                | VirtualRegister::XMM9H
                | VirtualRegister::XMM10H
                | VirtualRegister::XMM11H
                | VirtualRegister::XMM12H
                | VirtualRegister::XMM13H
                | VirtualRegister::XMM14H
                | VirtualRegister::XMM15H => true,
            },
        }
    }
}

impl From<iced_x86::Register> for Register {
    fn from(reg: iced_x86::Register) -> Self {
        Register::Native(reg)
    }
}

impl From<VirtualRegister> for Register {
    fn from(reg: VirtualRegister) -> Self {
        Register::Virtual(reg)
    }
}

#[derive(Debug, Copy, Clone, Eq, PartialEq)]
pub enum Operand {
    Register(Register),
    BranchTarget(u64),
    Immediate(u64),
    Memory {
        base: iced_x86::Register,
        index: iced_x86::Register,
        scale: u32,
        displacement: u64,
    },
}

impl Operand {
    fn from_instr(instr: iced_x86::Instruction, op: u32) -> Result<Self, String> {
        if op >= instr.op_count() {
            return Err(format!(
                "Instruction {:?} has {} operands, but op# {} was accessed",
                instr.mnemonic(),
                instr.op_count(),
                op
            ));
        }

        Ok(match instr.op_kind(op) {
            iced_x86::OpKind::Register => Operand::Register(instr.op_register(op).into()),
            iced_x86::OpKind::NearBranch16
            | iced_x86::OpKind::NearBranch32
            | iced_x86::OpKind::NearBranch64 => Operand::BranchTarget(instr.near_branch_target()),
            iced_x86::OpKind::FarBranch16 | iced_x86::OpKind::FarBranch32 => {
                panic!("FAR operands not supported");
            }
            iced_x86::OpKind::Immediate8
            | iced_x86::OpKind::Immediate8_2nd
            | iced_x86::OpKind::Immediate16
            | iced_x86::OpKind::Immediate32
            | iced_x86::OpKind::Immediate64
            | iced_x86::OpKind::Immediate8to16
            | iced_x86::OpKind::Immediate8to32
            | iced_x86::OpKind::Immediate8to64
            | iced_x86::OpKind::Immediate32to64 => Operand::Immediate(instr.immediate(op)),
            iced_x86::OpKind::MemorySegSI => todo!(),
            iced_x86::OpKind::MemorySegESI => todo!(),
            iced_x86::OpKind::MemorySegRSI => todo!(),
            iced_x86::OpKind::MemorySegDI => todo!(),
            iced_x86::OpKind::MemorySegEDI => todo!(),
            iced_x86::OpKind::MemorySegRDI => todo!(),
            iced_x86::OpKind::MemoryESDI => todo!(),
            iced_x86::OpKind::MemoryESEDI => todo!(),
            iced_x86::OpKind::MemoryESRDI => todo!(),
            iced_x86::OpKind::Memory => Operand::Memory {
                base: instr.memory_base(),
                index: instr.memory_index(),
                scale: instr.memory_index_scale(),
                displacement: instr.memory_displacement64(),
            },
        })
    }
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
pub enum Mnemonic {
    Real(iced_x86::Mnemonic),
    Regzero,
}

#[derive(Debug, Clone)]
pub struct Instruction {
    pub original_instr: iced_x86::Instruction,
    pub original_ip: u64,
    pub original_next_ip: u64,
    pub target_mnemonic: Mnemonic,
    pub flow_control: iced_x86::FlowControl,
    pub operands: Vec<Operand>,
}

impl Instruction {
    pub fn wrap_native(instr: iced_x86::Instruction) -> Self {
        let operands = (0..instr.op_count())
            .map(|o| Operand::from_instr(instr, o).unwrap())
            .collect();

        Self {
            original_instr: instr,
            original_ip: instr.ip(),
            original_next_ip: instr.next_ip(),
            target_mnemonic: Mnemonic::Real(instr.mnemonic()),
            flow_control: instr.flow_control(),
            operands: operands,
        }
    }

    fn has_ymm(&self) -> bool {
        self.operands.iter().any(|o| match o {
            Operand::Register(r) => match r {
                Register::Native(r) => r.is_ymm(),
                Register::Virtual(_) => false,
            },
            Operand::BranchTarget(_) => false,
            Operand::Immediate(_) => false,
            Operand::Memory {
                base: _,
                index: _,
                scale: _,
                displacement: _,
            } => false,
        })
    }

    fn get_xmm_regs<'a, T: Iterator<Item = &'a Operand>>(operands: T) -> Vec<Register> {
        let mut ret = Vec::new();
        for op in operands {
            match op {
                Operand::Register(reg) => match reg {
                    Register::Native(reg) => {
                        if reg.is_xmm() {
                            ret.push((*reg).into());
                        }
                        if reg.is_ymm() {
                            let (lo, hi) = VirtualRegister::pair_for_ymm(&reg);
                            ret.push(lo);
                            ret.push(hi)
                        }
                    }
                    Register::Virtual(_) => todo!(),
                },
                _ => {}
            }
        }
        ret
    }

    pub fn get_input_xmm_regs(&self) -> Vec<Register> {
        if self.original_instr.encoding() != EncodingKind::VEX {
            return vec![];
        }
        // Special cases
        match self.target_mnemonic {
            Mnemonic::Real(m) => {
                match m {
                    // (V)PXOR used to zero out a register - no real dependency on previous state
                    iced_x86::Mnemonic::Vpxor => {
                        if self.operands[0] == self.operands[1] && self.operands[1] == self.operands[2] {
                            return vec![];
                        }
                    }
                    iced_x86::Mnemonic::Pxor => {
                        if self.operands[0] == self.operands[1] {
                            return vec![];
                        }
                    }
                    // Assume that all function calls follow Vectorcall convention
                    // https://learn.microsoft.com/en-us/cpp/cpp/vectorcall?view=msvc-170
                    iced_x86::Mnemonic::Call => {
                        return vec![
                            Register::Native(iced_x86::Register::XMM0),
                            Register::Virtual(VirtualRegister::XMM0H),
                            Register::Native(iced_x86::Register::XMM1),
                            Register::Virtual(VirtualRegister::XMM1H),
                            Register::Native(iced_x86::Register::XMM2),
                            Register::Virtual(VirtualRegister::XMM2H),
                            Register::Native(iced_x86::Register::XMM3),
                            Register::Virtual(VirtualRegister::XMM3H),
                            Register::Native(iced_x86::Register::XMM4),
                            Register::Virtual(VirtualRegister::XMM4H),
                            Register::Native(iced_x86::Register::XMM5),
                            Register::Virtual(VirtualRegister::XMM5H),
                        ];
                    }
                    _ => {}
                }
            }
            _ => {}
        }

        let pc = get_prod_cons(&self.target_mnemonic);
        match pc {
            RegProdCons::AllRead => Self::get_xmm_regs(self.operands.iter()),
            RegProdCons::FirstModifyOtherRead => Self::get_xmm_regs(self.operands.iter()),
            RegProdCons::FirstWriteOtherRead => Self::get_xmm_regs(self.operands.iter().skip(1)),
            RegProdCons::None => vec![],
        }
    }

    pub fn get_output_xmm_regs(&self) -> Vec<(Register, RegisterValue)> {
        if self.original_instr.encoding() != EncodingKind::VEX {
            return vec![];
        }
        // Special cases
        match self.target_mnemonic {
            Mnemonic::Real(m) => {
                match m {
                    // (V)PXOR used to zero out a register
                    iced_x86::Mnemonic::Vpxor => {
                        if self.operands[0] == self.operands[1] && self.operands[1] == self.operands[2] {
                            let op = self.operands[0];
                            if let Operand::Register(reg) = op {
                                return vec![(reg, RegisterValue::Zero)];
                            }
                        }
                    }
                    iced_x86::Mnemonic::Pxor => {
                        if self.operands[0] == self.operands[1] {
                            let op = self.operands[0];
                            if let Operand::Register(reg) = op {
                                return vec![(reg, RegisterValue::Zero)];
                            }
                        }
                    }
                    // Assume that all function calls follow Vectorcall convention
                    // and potentially return HVA
                    // https://learn.microsoft.com/en-us/cpp/cpp/vectorcall?view=msvc-170
                    iced_x86::Mnemonic::Call => {
                        return vec![
                            (Register::Native(iced_x86::Register::XMM0), RegisterValue::Unknown),
                            (Register::Virtual(VirtualRegister::XMM0H), RegisterValue::Unknown),
                            (Register::Native(iced_x86::Register::XMM1), RegisterValue::Unknown),
                            (Register::Virtual(VirtualRegister::XMM1H), RegisterValue::Unknown),
                            (Register::Native(iced_x86::Register::XMM2), RegisterValue::Unknown),
                            (Register::Virtual(VirtualRegister::XMM2H), RegisterValue::Unknown),
                            (Register::Native(iced_x86::Register::XMM3), RegisterValue::Unknown),
                            (Register::Virtual(VirtualRegister::XMM3H), RegisterValue::Unknown),
                        ];
                    }
                    _ => {}
                }
            }
            _ => {}
        }

        match self.target_mnemonic {
            Mnemonic::Real(m) => match m {
                iced_x86::Mnemonic::Vzeroupper => all::<VirtualRegister>()
                    .map(|r| (Register::Virtual(r), RegisterValue::Zero))
                    .collect(),
                _ => {
                    let pc = get_prod_cons(&self.target_mnemonic);
                    match pc {
                        RegProdCons::AllRead => {
                            vec![]
                        }
                        RegProdCons::FirstModifyOtherRead => {
                            let ret: Vec<(Register, RegisterValue)> =
                                Self::get_xmm_regs(self.operands.iter().take(1))
                                    .into_iter()
                                    .map(|r| (r, RegisterValue::Unknown))
                                    .collect();

                            let mut xmmregs = HashSet::new();
                            for r in &ret {
                                xmmregs.insert(r.0);
                            }
                            if self.original_instr.encoding() == EncodingKind::VEX {
                                let high_zero: Vec<(Register, RegisterValue)> = ret
                                    .iter()
                                    .filter_map(|r| match r.0 {
                                        Register::Native(reg) => {
                                            let high_xmm = Register::Virtual(
                                                VirtualRegister::high_for_xmm(&reg),
                                            );
                                            if xmmregs.contains(&high_xmm) {
                                                return None;
                                            }

                                            Some((high_xmm, RegisterValue::Zero))
                                        }
                                        Register::Virtual(_) => None,
                                    })
                                    .collect();
                                ret.into_iter().chain(high_zero.into_iter()).collect()
                            } else {
                                ret
                            }
                        }
                        RegProdCons::FirstWriteOtherRead => {
                            let ret: Vec<(Register, RegisterValue)> =
                                Self::get_xmm_regs(self.operands.iter().take(1))
                                    .into_iter()
                                    .map(|r| (r, RegisterValue::Unknown))
                                    .collect();
                            let mut xmmregs = HashSet::new();
                            for r in &ret {
                                xmmregs.insert(r.0);
                            }
                            if self.original_instr.encoding() == EncodingKind::VEX {
                                let high_zero: Vec<(Register, RegisterValue)> = ret
                                    .iter()
                                    .filter_map(|r| match r.0 {
                                        Register::Native(reg) => {
                                            let high_xmm = Register::Virtual(
                                                VirtualRegister::high_for_xmm(&reg),
                                            );
                                            if xmmregs.contains(&high_xmm) {
                                                return None;
                                            }

                                            Some((high_xmm, RegisterValue::Zero))
                                        }
                                        Register::Virtual(_) => None,
                                    })
                                    .collect();
                                ret.into_iter().chain(high_zero.into_iter()).collect()
                            } else {
                                ret
                            }
                        }
                        RegProdCons::None => {
                            vec![]
                        }
                    }
                }
            },
            Mnemonic::Regzero => todo!(),
        }
    }
}
