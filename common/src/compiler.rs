use std::collections::{HashMap, HashSet};

use itertools::Itertools;

use crate::{decoder::base::DecodedBlock, tir::{mapping::{get_prod_cons, RegProdCons}, Instruction, Operand, Register}};

#[derive(Debug)]
pub struct ControlBlock {
    pub instructions: Vec<Instruction>,
}

impl ControlBlock {
    fn get_xmm_regs(operands: Vec<Operand>) -> Vec<iced_x86::Register> {
        let mut ret = Vec::new();
        for op in operands {
            match op {
                Operand::Register(reg) => {
                    match reg {
                        Register::Native(reg) => {
                            if reg.is_xmm() || reg.is_ymm() {
                                ret.push(reg);
                            }
                        },
                        Register::Virtual(_) => todo!(),
                    }
                },
                _ => {}    
            }
        }
        ret
    }

    pub fn get_input_xmm_registers(&self) -> Vec<iced_x86::Register> {
        self.get_input_xmm_registers_until(u64::MAX)
    }

    pub fn get_input_xmm_registers_until(&self, until_ip: u64) -> Vec<iced_x86::Register> {
        let mut ret = Vec::new();

        for i in &self.instructions {
            if i.original_ip >= until_ip {
                break;
            }
            let pc = get_prod_cons(&i.target_mnemonic);
            match pc {
                RegProdCons::AllRead => {
                    let mut regs = Self::get_xmm_regs(i.operands.clone());
                    ret.append(&mut regs);
                },
                RegProdCons::FirstWriteOtherRead => {
                    let s = i.operands.iter().skip(1).map(|o| *o).collect();
                    let mut regs = Self::get_xmm_regs(s);
                    ret.append(&mut regs);
                },
                RegProdCons::None => {},
            }
        }

        ret.iter().unique().map(|r| *r).collect()
    }

    pub fn get_output_xmm_registers(&self) -> Vec<iced_x86::Register> {
        let mut ret = Vec::new();

        for i in &self.instructions {
            match i.target_mnemonic {
                crate::tir::Mnemonic::Real(m) => match m {
                    iced_x86::Mnemonic::Vzeroupper => {
                        let input_xmm_regs_until_current = self.get_input_xmm_registers_until(i.original_ip);
                        let mut ymm_regs = input_xmm_regs_until_current.iter().filter(|r| r.is_ymm()).map(|r| *r).collect();
                        ret.append(&mut ymm_regs);
                    },
                    _ => {
                        let pc = get_prod_cons(&i.target_mnemonic);
                        match pc {
                            RegProdCons::AllRead => {},
                            RegProdCons::FirstWriteOtherRead => {
                                let s = i.operands.iter().take(1).map(|o| *o).collect();
                                let mut regs = Self::get_xmm_regs(s);
                                ret.append(&mut regs);
                            },
                            RegProdCons::None => {},
                        }
                    }
                },
                crate::tir::Mnemonic::Regzero => todo!(),
            }
        }

        ret.iter().unique().map(|r| *r).collect()
    }
}

fn create_control_blocks(block: &DecodedBlock) -> Vec<ControlBlock> {
    let mut branch_points = HashSet::new();
    let mut branch_targets = HashSet::new();

    for i in &block.instructions {
        match i.flow_control() {
            iced_x86::FlowControl::Next => {}
            iced_x86::FlowControl::Exception => {}
            iced_x86::FlowControl::Interrupt => {}
            iced_x86::FlowControl::Call => {}
            iced_x86::FlowControl::IndirectCall => {}
            iced_x86::FlowControl::Return => {
                branch_points.insert(i.ip());
            }
            iced_x86::FlowControl::UnconditionalBranch
            | iced_x86::FlowControl::IndirectBranch
            | iced_x86::FlowControl::ConditionalBranch => {
                branch_points.insert(i.ip());
                match i.op0_kind() {
                    iced_x86::OpKind::NearBranch16
                    | iced_x86::OpKind::NearBranch32
                    | iced_x86::OpKind::NearBranch64 => {
                        let target = i.near_branch_target();
                        let next_ip = i.next_ip();
                        if i.flow_control() == iced_x86::FlowControl::ConditionalBranch {
                            branch_targets.insert(target);
                            branch_targets.insert(next_ip);
                        } else {
                            branch_targets.insert(target);
                        }
                    },
                    iced_x86::OpKind::FarBranch16 => todo!(),
                    iced_x86::OpKind::FarBranch32 => todo!(),
                    _ => {},
                }
            }
            iced_x86::FlowControl::XbeginXabortXend => todo!(),
        }
    }

    let mut result = Vec::new();
    let mut current_block = Vec::new();

    for i in &block.instructions {
        let wrapped_instr = Instruction::wrap_native(i.clone());
        if branch_points.contains(&i.ip()) {
            current_block.push(wrapped_instr);
            result.push(ControlBlock {
                instructions: current_block,
            });
            current_block = Vec::new();
        } else if branch_targets.contains(&i.ip()) {
            if !current_block.is_empty() {
                result.push(ControlBlock {
                    instructions: current_block,
                });
                current_block = Vec::new();
            }
            current_block.push(wrapped_instr);
        } else {
            current_block.push(wrapped_instr);
        }
    }

    result
}

pub fn analyze_block(block: &DecodedBlock) {
    let blocks = create_control_blocks(block);

    // let ir: Vec<_> = block
    //     .instructions
    //     .iter()
    //     .map(|i| Instruction::wrap_native(*i))
    //     .flat_map(|i| i.map())
    //     .collect();

    for cb in blocks {
        println!("ControlBlock {{");
        println!("  input = {:?}", cb.get_input_xmm_registers());
        for i in &cb.instructions {
            println!("  {} {:?} {:?}", i.original_ip, i.target_mnemonic, i.operands);
        }
        println!("  changes = {:?}", cb.get_output_xmm_registers());
        println!("}}");
    }
}
