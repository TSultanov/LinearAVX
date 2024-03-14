mod mapping;

#[derive(Debug)]
pub struct Instruction {
    pub original_ip: u64,
    pub target_mnemonic: iced_x86::Mnemonic,
    pub operands: Vec<Operand>,
}

#[derive(Debug, Copy, Clone)]
pub enum VirtualRegister {
    XMM0L,
    XMM1L,
    XMM2L,
    XMM3L,
    XMM4L,
    XMM5L,
    XMM6L,
    XMM7L,
    XMM8L,
    XMM9L,
    XMM10L,
    XMM11L,
    XMM12L,
    XMM13L,
    XMM14L,
    XMM15L,
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
    pub fn pair_for_ymm(reg: &iced_x86::Register) -> (Self, Self) {
        match reg {
            iced_x86::Register::YMM0 => (Self::XMM0L, Self::XMM0H),
            iced_x86::Register::YMM1 => (Self::XMM1L, Self::XMM1H),
            iced_x86::Register::YMM2 => (Self::XMM2L, Self::XMM2H),
            iced_x86::Register::YMM3 => (Self::XMM3L, Self::XMM3H),
            iced_x86::Register::YMM4 => (Self::XMM4L, Self::XMM4H),
            iced_x86::Register::YMM5 => (Self::XMM5L, Self::XMM5H),
            iced_x86::Register::YMM6 => (Self::XMM6L, Self::XMM6H),
            iced_x86::Register::YMM7 => (Self::XMM7L, Self::XMM7H),
            iced_x86::Register::YMM8 => (Self::XMM8L, Self::XMM8H),
            iced_x86::Register::YMM9 => (Self::XMM9L, Self::XMM9H),
            iced_x86::Register::YMM10 => (Self::XMM10L, Self::XMM10H),
            iced_x86::Register::YMM11 => (Self::XMM11L, Self::XMM11H),
            iced_x86::Register::YMM12 => (Self::XMM12L, Self::XMM12H),
            iced_x86::Register::YMM13 => (Self::XMM13L, Self::XMM13H),
            iced_x86::Register::YMM14 => (Self::XMM14L, Self::XMM14H),
            iced_x86::Register::YMM15 => (Self::XMM15L, Self::XMM15H),
            _ => {
                panic!("Unsupported register {:?}!", reg);
            }
        }
    }
}

#[derive(Debug, Copy, Clone)]
pub enum Register {
    Native(iced_x86::Register),
    Virtual(VirtualRegister),
}

impl From<iced_x86::Register> for Register {
    fn from(reg: iced_x86::Register) -> Self {
        Register::Native(reg)
    }
}

#[derive(Debug, Copy, Clone)]
pub enum Operand {
    Register(Register),
    BranchTarget(u64),
    Immedaite(u64),
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
            | iced_x86::OpKind::Immediate32to64 => Operand::Immedaite(instr.immediate(op)),
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

impl Instruction {
    pub fn wrap_native(instr: iced_x86::Instruction) -> Self {
        let operands = (0..instr.op_count())
            .map(|o| Operand::from_instr(instr, o).unwrap())
            .collect();

        Self {
            original_ip: instr.ip(),
            target_mnemonic: instr.mnemonic(),
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
            Operand::Immedaite(_) => false,
            Operand::Memory {
                base: _,
                index: _,
                scale: _,
                displacement: _,
            } => false,
        })
    }
}
