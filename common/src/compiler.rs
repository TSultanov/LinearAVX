use std::collections::{HashMap, HashSet, VecDeque};

use iced_x86::{code_asm::CodeAssembler, EncodingKind};
use itertools::Itertools;

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
    pub to_recompile: bool,
    pub instructions: Vec<(Instruction, RegisterUse)>,
    pub input_xmm_registers: Vec<Register>,
    pub output_xmm_registers: Vec<(Register, RegisterValue)>,
}

#[derive(Debug)]
pub struct FunctionBlock {
    pub control_blocks: Vec<ControlBlock>,
    pub edges: HashMap<usize, Vec<usize>>,
    range: std::ops::Range<u64>,
}

impl FunctionBlock {
    pub fn pretty_print(&self) {
        println!("FunctionBlock {{");
        println!(
            "  start = {:#x}, end = {:#x}",
            self.range.start, self.range.end
        );
        for cb in &self.control_blocks {
            if cb.to_recompile {
                println!("  ControlBlock(VEX) {{");
            } else {
                println!("  ControlBlock {{");
            }
            println!("    input = {:?}", cb.input_xmm_registers);
            for i in &cb.instructions {
                println!(
                    "    {} {:?} {:?}",
                    i.0.original_ip, i.0.target_mnemonic, i.0.operands
                );
            }
            println!("    outputs = {:?}", cb.output_xmm_registers);
            println!("  }}");
        }
        println!("}}");
    }
}

impl ControlBlock {
    pub fn new(instructions: Vec<Instruction>) -> Self {
        let reguse = Self::construct_xmm_register_use(&instructions);
        let input_xmm_registers = Self::construct_input_xmm_registers(&reguse);
        let output_xmm_registers = Self::construct_output_xmm_registers(&reguse);
        Self {
            to_recompile: instructions
                .iter()
                .any(|i| i.original_instr.encoding() == EncodingKind::VEX),
            instructions: instructions.into_iter().zip(reguse).collect(),
            input_xmm_registers: input_xmm_registers,
            output_xmm_registers: output_xmm_registers,
        }
    }

    fn construct_xmm_register_use(instructions: &Vec<Instruction>) -> Vec<RegisterUse> {
        instructions
            .iter()
            .map(|i| RegisterUse {
                input: i.get_input_regs(),
                output: i.get_output_regs(),
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

        result.into_iter().unique().collect()
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

fn create_control_blocks(block: &DecodedBlock) -> Vec<(ControlBlock, Vec<u64>)> {
    let mut branch_points = HashSet::new();
    let mut branch_targets = HashSet::new();
    let mut branches = HashMap::new();

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
                            branches.insert(i.ip(), vec![target, next_ip]);
                        } else {
                            branch_targets.insert(target);
                            branches.insert(i.ip(), vec![target]);
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

    for i in 1..block.instructions.len() {
        let curr = block.instructions.get(i).unwrap();
        let prev = block.instructions.get(i - 1).unwrap();
        let wrapped_instr = Instruction::wrap_native(*curr);

        if (prev.encoding() == EncodingKind::VEX && curr.encoding() != EncodingKind::VEX)
            || (prev.encoding() != EncodingKind::VEX && curr.encoding() == EncodingKind::VEX)
        {
            if !current_block.is_empty() {
                result.push((ControlBlock::new(current_block), vec![curr.ip()]));
                current_block = Vec::new();
            }
        }

        if branch_points.contains(&curr.ip()) {
            current_block.push(wrapped_instr);

            let targets = if let Some(targets) = branches.get(&curr.ip()) {
                targets.clone()
            } else {
                vec![]
            };

            result.push((ControlBlock::new(current_block), targets));
            current_block = Vec::new();
        } else if branch_targets.contains(&curr.ip()) {
            if !current_block.is_empty() {
                result.push((ControlBlock::new(current_block), vec![curr.ip()]));
                current_block = Vec::new();
            }
            current_block.push(wrapped_instr);
        } else {
            current_block.push(wrapped_instr);
        }
    }

    result
}

// fn propagate_registers(bl: FunctionBlock) -> FunctionBlock {
//     let mut output_registers: HashMap<usize, Vec<(Register, RegisterValue)>> = HashMap::new();

//     let mut visited_blocks = HashSet::new();
//     let mut blocks_to_visit = VecDeque::new();

//     blocks_to_visit.push_front((0, im::HashMap::<Register, RegisterValue>::new()));

//     loop {
//         if let Some((b, outputs_seen_so_far)) = blocks_to_visit.pop_front() {
//             visited_blocks.insert(b);

//             let block = bl.control_blocks.get(b).unwrap();
//             let mut outputs_seen_so_far = outputs_seen_so_far.clone();
//             for (reg, val) in &block.output_xmm_registers {
//                 outputs_seen_so_far.insert(*reg, *val);
//             }

//             output_registers.insert(
//                 b,
//                 outputs_seen_so_far
//                     .iter()
//                     .map(|(reg, value)| (reg.clone(), value.clone()))
//                     .collect_vec(),
//             );

//             if let Some(targets) = bl.edges.get(&b) {
//                 for t in targets {
//                     if !visited_blocks.contains(t) {
//                         blocks_to_visit.push_front((*t, outputs_seen_so_far.clone()));
//                     }
//                 }
//             }
//         } else {
//             break;
//         }
//     }

//     let control_blocks = bl
//         .control_blocks
//         .into_iter()
//         .zip(0..)
//         .map(|(block, i)| ControlBlock {
//             instructions: block.instructions,
//             to_recompile: block.to_recompile,
//             input_xmm_registers: block.input_xmm_registers,
//             output_xmm_registers: output_registers.get(&i).unwrap().clone(),
//         })
//         .collect();

//     FunctionBlock {
//         control_blocks: control_blocks,
//         edges: bl.edges,
//     }
// }

pub fn analyze_block(block: &DecodedBlock) -> FunctionBlock {
    let blocks = create_control_blocks(block);

    let mut block_start_num_map = HashMap::new();

    for i in 0..blocks.len() {
        let block = blocks.get(i).unwrap();
        let (first_instr, _) = block.0.instructions.first().unwrap();
        let ip = first_instr.original_ip;
        block_start_num_map.insert(ip, i);
    }

    let mut edges: HashMap<usize, Vec<usize>> = HashMap::new();

    for i in 0..blocks.len() {
        let targets = &blocks[i].1;
        for t in targets {
            if !block.range.contains(t) {
                continue;
            }
            let target_index = block_start_num_map.get(&t).unwrap();

            if edges.contains_key(&i) {
                edges.get_mut(&i).unwrap().push(*target_index);
            } else {
                edges.insert(i, vec![*target_index]);
            }
        }
    }

    let control_blocks: Vec<ControlBlock> = blocks.into_iter().map(|b| b.0).collect();

    let fb = FunctionBlock {
        range: block.range.clone(),
        control_blocks,
        edges,
    };

    fb
}

pub fn translate_block(fb: FunctionBlock) -> FunctionBlock {
    let cbs = fb
        .control_blocks
        .iter()
        .map(|cb| translate_control_block(cb))
        .collect();

    FunctionBlock {
        control_blocks: cbs,
        edges: fb.edges,
        range: fb.range,
    }
}

fn translate_control_block(cb: &ControlBlock) -> ControlBlock {
    let instructions_no_ymm = cb
        .instructions
        .iter()
        // .flat_map(|(i, _)| i.eliminate_ymm_by_splitting(true))
        .flat_map(|(i, _)| i.map())
        // .map(|(i, _)| i.clone())
        .collect();
    ControlBlock::new(instructions_no_ymm)
}

// fn assemble(instr: &Instruction) -> Vec<iced_x86::Instruction> {
//     match instr.target_mnemonic {
//         crate::tir::Mnemonic::Real(rm) => {
//             if rm != instr.original_instr.mnemonic() {
//                 match rm {
//                     iced_x86::Mnemonic::Movsd => {}
//                     _ => todo!(),
//                 }
//             } else {
//                 let inst = instr.original_instr.clone();
//                 inst.set_ip(0);
//                 vec![inst]
//             }
//         }
//         crate::tir::Mnemonic::Regzero => todo!(),
//     }
// }

pub fn compile_block(fb: &FunctionBlock) {}
