use std::collections::{HashMap, HashSet};

use crate::{
    decoder::base::DecodedBlock,
    tir::{Instruction, Register, RegisterValue},
};

#[derive(Debug)]
pub struct RegisterUse {
    pub input: Vec<Register>,
    pub output: Vec<(Register, RegisterValue)>,
}

#[derive(Debug)]
pub struct ControlBlock {
    pub instructions: Vec<(Instruction, RegisterUse)>,
    pub input_xmm_registers: Vec<Register>,
    pub output_xmm_registers: Vec<(Register, RegisterValue)>,
}

impl ControlBlock {
    pub fn new(instructions: Vec<Instruction>) -> Self {
        let reguse = Self::construct_xmm_register_use(&instructions);
        let input_xmm_registers = Self::construct_input_xmm_registers(&reguse);
        let output_xmm_registers = Self::construct_output_xmm_registers(&reguse);
        Self {
            instructions: instructions.into_iter().zip(reguse).collect(),
            input_xmm_registers: input_xmm_registers,
            output_xmm_registers: output_xmm_registers,
        }
    }

    fn construct_xmm_register_use(instructions: &Vec<Instruction>) -> Vec<RegisterUse> {
        instructions
            .iter()
            .map(|i| RegisterUse {
                input: i.get_input_xmm_regs(),
                output: i.get_output_xmm_regs(),
            })
            .collect()
    }

    fn construct_input_xmm_registers(register_use: &Vec<RegisterUse>) -> Vec<Register> {
        let mut seen_outputs = HashSet::new();
        let mut result = Vec::new();

        for ru in register_use {
            for i in &ru.input {
                if seen_outputs.contains(i) {
                    continue;
                }

                result.push(*i);
            }
            for o in &ru.output {
                seen_outputs.insert(o.0);
            }
        }

        result
    }

    fn construct_output_xmm_registers(
        register_use: &Vec<RegisterUse>,
    ) -> Vec<(Register, RegisterValue)> {
        let mut seen_outputs = HashMap::new();

        for ru in register_use {
            for o in &ru.output {
                seen_outputs.insert(o.0, o.1);
            }
        }

        seen_outputs.into_iter().collect()
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
                    }
                    iced_x86::OpKind::FarBranch16 => todo!(),
                    iced_x86::OpKind::FarBranch32 => todo!(),
                    _ => {}
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
            result.push(ControlBlock::new(current_block));
            current_block = Vec::new();
        } else if branch_targets.contains(&i.ip()) {
            if !current_block.is_empty() {
                result.push(ControlBlock::new(current_block));
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
        println!("  input = {:?}", cb.input_xmm_registers);
        for i in &cb.instructions {
            println!(
                "  {} {:?} {:?}",
                i.0.original_ip, i.0.target_mnemonic, i.0.operands
            );
        }
        println!("  outputs = {:?}", cb.output_xmm_registers);
        println!("}}");
    }
}
