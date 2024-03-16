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
    pub fn map(self) -> Vec<Instruction> {
        match self.target_mnemonic {
            super::Mnemonic::Real(mnemonic) => match mnemonic {
                iced_x86::Mnemonic::INVALID => panic!("Invalid instruction"),
                iced_x86::Mnemonic::Vmovups => {
                    let new_op = Self {
                        target_mnemonic: Mnemonic::Real(iced_x86::Mnemonic::Movups),
                        ..self
                    };
                    let no_ymm = new_op.eliminate_ymm_by_splitting(true);
                    no_ymm
                }
                iced_x86::Mnemonic::Vmovsd => {
                    let new_op = Self {
                        target_mnemonic: Mnemonic::Real(iced_x86::Mnemonic::Movsd),
                        ..self
                    };
                    let no_ymm = new_op.eliminate_ymm_by_splitting(true);
                    no_ymm
                }
                _ => vec![self],
            },
            super::Mnemonic::Regzero => todo!(),
        }
    }

    pub fn eliminate_ymm_by_splitting(self, zeroupper: bool) -> Vec<Self> {
        if !self.has_ymm() {
            // This is cursed
            return if let Some(first_op) = self.operands.first() {
                if zeroupper {
                    match first_op {
                        Operand::Register(reg) => match reg {
                            Register::Native(reg) => {
                                if reg.is_xmm() {
                                    let reg = Register::Virtual(VirtualRegister::high_for_xmm(reg));
                                    add_zeroupper(vec![self], Operand::Register(reg))
                                } else {
                                    vec![self]
                                }
                            }
                            Register::Virtual(_) => vec![self],
                        },
                        _ => vec![self],
                    }
                } else {
                    vec![self]
                }
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

#[derive(Clone, Debug)]
pub enum RegProdCons {
    AllRead,
    FirstWriteOtherRead,
    FirstModifyOtherRead,
    None,
}

pub fn get_prod_cons(m: &Mnemonic) -> RegProdCons {
    match m {
        Mnemonic::Real(m) => match m {
            iced_x86::Mnemonic::INVALID => todo!(),
            iced_x86::Mnemonic::Push
            | iced_x86::Mnemonic::Pop
            | iced_x86::Mnemonic::Jne
            | iced_x86::Mnemonic::Js
            | iced_x86::Mnemonic::Call
            | iced_x86::Mnemonic::Test
            | iced_x86::Mnemonic::Jmp
            | iced_x86::Mnemonic::Jb
            | iced_x86::Mnemonic::Jae
            | iced_x86::Mnemonic::Jbe
            | iced_x86::Mnemonic::Je
            | iced_x86::Mnemonic::Vcomiss
            | iced_x86::Mnemonic::Cmp => RegProdCons::AllRead,
            iced_x86::Mnemonic::Vmovss
            | iced_x86::Mnemonic::Vmovsd
            | iced_x86::Mnemonic::Vxorps
            | iced_x86::Mnemonic::Vcvtsi2ss
            | iced_x86::Mnemonic::Vcvttss2si
            | iced_x86::Mnemonic::Vaddss
            | iced_x86::Mnemonic::Vmovaps
            | iced_x86::Mnemonic::Vmulss
            | iced_x86::Mnemonic::Vmulps
            | iced_x86::Mnemonic::Vsubps
            | iced_x86::Mnemonic::Vsubss
            | iced_x86::Mnemonic::Vinsertps
            | iced_x86::Mnemonic::Vdpps
            | iced_x86::Mnemonic::Vcmpss
            | iced_x86::Mnemonic::Vblendvps
            | iced_x86::Mnemonic::Vmaxss
            | iced_x86::Mnemonic::Vmaxps
            | iced_x86::Mnemonic::Vminps
            | iced_x86::Mnemonic::Vminss
            | iced_x86::Mnemonic::Vdivss
            | iced_x86::Mnemonic::Vextractps
            | iced_x86::Mnemonic::Vpermilps
            | iced_x86::Mnemonic::Vblendps
            | iced_x86::Mnemonic::Vcmpps
            | iced_x86::Mnemonic::Vshufps
            | iced_x86::Mnemonic::Vbroadcastss
            | iced_x86::Mnemonic::Vmovdqu
            | iced_x86::Mnemonic::Vaddps
            | iced_x86::Mnemonic::Vrsqrtps
            | iced_x86::Mnemonic::Vdivps
            | iced_x86::Mnemonic::Vpand
            | iced_x86::Mnemonic::Vorps
            | iced_x86::Mnemonic::Vpsrld
            | iced_x86::Mnemonic::Vpsubd
            | iced_x86::Mnemonic::Vcvtdq2ps
            | iced_x86::Mnemonic::Vmovups => RegProdCons::FirstWriteOtherRead,
            iced_x86::Mnemonic::Xor
            | iced_x86::Mnemonic::Sub
            | iced_x86::Mnemonic::Mov
            | iced_x86::Mnemonic::Lea
            | iced_x86::Mnemonic::Shr
            | iced_x86::Mnemonic::And
            | iced_x86::Mnemonic::Or
            | iced_x86::Mnemonic::Add
            | iced_x86::Mnemonic::Shl
            | iced_x86::Mnemonic::Vfmadd231ps
            | iced_x86::Mnemonic::Inc => RegProdCons::FirstModifyOtherRead,
            iced_x86::Mnemonic::Vzeroupper
            | iced_x86::Mnemonic::Nop
            | iced_x86::Mnemonic::Ret => RegProdCons::None,

            _ => panic!("{:?} not implemented", m),
        },
        Mnemonic::Regzero => RegProdCons::FirstWriteOtherRead,
    }
}
