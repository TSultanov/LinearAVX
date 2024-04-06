use std::ops::Range;
use iced_x86::{CpuidFeature, FlowControl, Formatter};
use std::collections::BTreeMap;
use itertools::Itertools;
use crate::decoder::base::{BlockType, Branch, MergeOrRebalanceResult};

#[derive(Clone)]
pub struct DecodedBlock {
    pub instructions: Vec<iced_x86::Instruction>,
    pub block_type: BlockType,
    pub branch_targets: Vec<Branch>,
    pub references: Vec<Branch>,
    pub range: Range<u64>,
}

impl DecodedBlock {
    pub fn new(
        block_type: BlockType,
        instructions: Vec<iced_x86::Instruction>,
        references: Vec<Branch>,
        branch_targets: Vec<Branch>,
    ) -> DecodedBlock {
        // find max and min address of the block and filter branch targets which are not in the block
        let min_address = instructions
            .iter()
            .map(|i| i.ip())
            .min()
            .expect("Empty instruction list is unexpected");
        let max_address = instructions.iter().map(|i| i.ip()).max().unwrap();
        let last_instruction = instructions.iter().max_by_key(|i| i.ip()).unwrap();
        let next_instr_ip = last_instruction.next_ip();

        let mut branch_targets_filtered = Vec::new();
        for t in branch_targets {
            if t.to < min_address || t.to > max_address {
                branch_targets_filtered.push(t);
            }
        }

        DecodedBlock {
            block_type,
            instructions,
            branch_targets: branch_targets_filtered,
            references,
            range: Range {
                start: min_address,
                end: next_instr_ip,
            },
        }
    }

    pub fn merge(&self, other: &DecodedBlock) -> MergeOrRebalanceResult {
        let left_block = if self.range.start <= other.range.start {
            &self
        } else {
            &other
        };

        let right_block = if self.range.start >= other.range.start {
            &self
        } else {
            &other
        };

        if left_block.range.start <= right_block.range.start
            && left_block.range.end >= right_block.range.end
        {
            return MergeOrRebalanceResult::Single((*left_block).clone());
        }

        if left_block.range.end < right_block.range.start {
            return MergeOrRebalanceResult::None;
        }

        if left_block.instructions.last().unwrap().flow_control() == FlowControl::Return {
            return MergeOrRebalanceResult::Two((*left_block).clone(), (*right_block).clone());
        }

        // if right_block.block_type == BlockType::Raw {
        //     if left_block.range.start < right_block.range.start
        //         && left_block.range.end == right_block.range.end
        //     {
        //         // In such case we probably have some jump table errorneously interpreted as a single function

        //         let left_range = Range {
        //             start: left_block.range.start,
        //             end: right_block.range.start,
        //         };

        //         let left_instructions = left_block
        //             .instructions
        //             .iter()
        //             .filter(|i| left_range.contains(&i.ip()))
        //             .map(|i| *i)
        //             .collect();

        //         let left_branches = left_block
        //             .branch_targets
        //             .iter()
        //             .filter(|b| left_range.contains(&b.from))
        //             .map(|b| *b)
        //             .collect();

        //         let left_refs = left_block
        //             .references
        //             .iter()
        //             .filter(|r| left_range.contains(&r.to))
        //             .map(|b| *b)
        //             .collect();

        //         let left_block_new = DecodedBlock::new(
        //             left_block.block_type,
        //             left_instructions,
        //             left_refs,
        //             left_branches,
        //         );

        //         return MergeOrRebalanceResult::Two(left_block_new, (**right_block).clone());
        //     }

        //     return MergeOrRebalanceResult::Two((**left_block).clone(), (**right_block).clone());
        // }

        let new_range = Range {
            start: left_block.range.start,
            end: right_block.range.end,
        };

        let mut instructions_set = BTreeMap::new();
        for instr in &left_block.instructions {
            instructions_set.insert(instr.ip(), *instr);
        }

        for instr in &right_block.instructions {
            if !instructions_set.contains_key(&instr.ip()) {
                instructions_set.insert(instr.ip(), *instr);
            }
        }

        let instuctions_new = instructions_set.into_values().collect();

        let branch_targets_new: Vec<Branch> = left_block
            .branch_targets
            .iter()
            .chain(right_block.branch_targets.iter())
            .filter(|b| !new_range.contains(&b.to))
            .map(|b| *b)
            .unique()
            .collect();

        let references_new = left_block
            .references
            .iter()
            .chain(right_block.references.iter())
            .map(|r| *r)
            .unique()
            .collect();

        MergeOrRebalanceResult::Single(DecodedBlock::new(
            left_block.block_type,
            instuctions_new,
            references_new,
            branch_targets_new,
        ))
    }

    pub fn pretty_print(&self) {
        let mut formatter = iced_x86::NasmFormatter::new();

        formatter.options_mut().set_digit_separator("`");
        formatter.options_mut().set_first_operand_char_index(10);
        formatter.options_mut().set_branch_leading_zeros(false);

        let mut output = String::new();

        println!(
            "\nBlock type {:?} at {:#x} [{:#x} - {:#x}):",
            self.block_type,
            self.instructions[0].ip(),
            self.range.start,
            self.range.end,
        );

        for instr in &self.instructions {
            output.clear();
            formatter.format(&instr, &mut output);

            print!("{:X} ", instr.ip());
            println!(" {}", output);
        }
        if !self.references.is_empty() {
            println!("Backrefs:");
            for t in &self.references {
                println!("From {:#x} to {:#x}", t.from, t.to);
            }
        }
        if !self.branch_targets.is_empty() {
            println!("Branches:");
            for t in &self.branch_targets {
                println!("From {:#x} to {:#x}", t.from, t.to);
            }
        }
        println!("*****");
    }

    pub fn needs_recompiling(&self) -> bool {
        self.instructions.iter().any(|i| {
            i.cpuid_features().iter().any(|f| match f {
                CpuidFeature::AVX => true,
                CpuidFeature::AVX2 => true,
                CpuidFeature::BMI1 => true,
                CpuidFeature::BMI2 => true,
                _ => false,
            })
        })
    }
}
