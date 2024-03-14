mod mapping;

use num::FromPrimitive;
use num_derive::FromPrimitive;

use self::mapping::map;

#[derive(Debug)]
pub struct Instruction {
    pub original_ip: u64,
    pub target_mnemonic: iced_x86::Mnemonic,
    pub operands: Vec<Operand>,
}

#[derive(Debug, FromPrimitive)]
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

#[derive(Debug)]
pub enum Register {
    Native(iced_x86::Register),
    Virtual(VirtualRegister),
}

impl From<iced_x86::Register> for Register {
    fn from(reg: iced_x86::Register) -> Self {
        if reg.is_ymm() {
            let reg_num = reg as usize - iced_x86::Register::YMM0 as usize;
            let virtual_reg = VirtualRegister::from_usize(reg_num).unwrap();
            Register::Virtual(virtual_reg)
        } else {
            Register::Native(reg)
        }
    }
}

#[derive(Debug)]
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
    pub fn wrap_native(instr: iced_x86::Instruction) -> Instruction {
        let operands = (0..instr.op_count())
            .map(|o| Operand::from_instr(instr, o).unwrap())
            .collect();

        Instruction {
            original_ip: instr.ip(),
            target_mnemonic: instr.mnemonic(),
            operands: operands,
        }
    }

    pub fn translate_native(instr: iced_x86::Instruction) -> Vec<Instruction> {
        map(instr)
    }
}
