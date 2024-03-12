use std::{
    collections::{BTreeMap, BTreeSet, HashMap, HashSet, VecDeque},
    error::Error,
    ops::Range,
};

use iced_x86::{FlowControl, Formatter};
use intervaltree::{Element, IntervalTree};
use itertools::Itertools;

pub struct TextDecoder<'a> {
    pub base_address: u64,
    bitness: u32,
    data: &'a [u8]
}

impl<'a> TextDecoder<'_> {
    pub fn new(bitness: u32, base_address: u64, data: &'a [u8]) -> Result<TextDecoder<'a>, Box<dyn Error>> {
        Ok(TextDecoder {
            base_address: base_address,
            bitness: bitness,
            data: data,
        })
    }

    pub fn decode_all_from(
        &self,
        addr: u64,
    ) -> Result<IntervalTree<u64, DecodedBlock>, Box<dyn Error>> {
        let raw_blocks = self.decode_from(addr)?;
        Ok(Self::merge_blocks(raw_blocks))
    }

    pub fn decode_at(
        &self,
        block_type: BlockType,
        addr: u64,
    ) -> Result<DecodedBlock, Box<dyn Error>> {
        let position: usize = (addr - self.base_address).try_into().unwrap();

        let decoder = iced_x86::Decoder::with_ip(
            self.bitness,
            &self.data[position..],
            addr,
            iced_x86::DecoderOptions::NONE,
        );

        let mut instructions = Vec::new();

        let mut branches: Vec<Branch> = Vec::new();

        for instr in decoder {
            instructions.push(instr);

            match instr.flow_control() {
                FlowControl::Next => {}

                FlowControl::Return => {
                    break;
                }

                FlowControl::UnconditionalBranch => {
                    if instr.op0_kind() == iced_x86::OpKind::NearBranch64 {
                        let target = instr.near_branch_target();
                        branches.push(Branch {
                            typ: BranchType::Jump,
                            from: instr.ip(),
                            to: target,
                        });
                        break;
                    }
                    panic!(
                        "UnconditionalBranch with op kind {:?} not supported (IP {:#x})",
                        instr.op0_kind(),
                        instr.ip()
                    );
                }

                FlowControl::IndirectBranch => match instr.op0_kind() {
                    iced_x86::OpKind::Memory => {}
                    iced_x86::OpKind::Register => {}
                    _ => {
                        panic!(
                            "IndirectBranch with op kind {:?} not supported (IP {:#x})",
                            instr.op0_kind(),
                            instr.ip()
                        );
                    }
                },

                FlowControl::ConditionalBranch => {
                    if instr.op0_kind() == iced_x86::OpKind::NearBranch64 {
                        branches.push(Branch {
                            typ: BranchType::Jump,
                            from: instr.ip(),
                            to: instr.near_branch_target(),
                        });
                    } else {
                        panic!(
                            "ConditionalBranch with op kind {:?} not supported (IP {:#x})",
                            instr.op0_kind(),
                            instr.ip()
                        );
                    }
                }

                FlowControl::Call => match instr.op0_kind() {
                    iced_x86::OpKind::NearBranch64 => {
                        branches.push(Branch {
                            typ: BranchType::Call,
                            from: instr.ip(),
                            to: instr.near_branch_target(),
                        });
                    }
                    _ => {
                        panic!(
                            "Call with op kind {:?} not supported (IP {:#x})",
                            instr.op0_kind(),
                            instr.ip()
                        );
                    }
                },

                FlowControl::IndirectCall => match instr.op0_kind() {
                    iced_x86::OpKind::Memory => {}
                    iced_x86::OpKind::Register => {}
                    _ => {
                        panic!(
                            "IndirectCall with op kind {:?} not supported (IP {:#x})",
                            instr.op0_kind(),
                            instr.ip()
                        );
                    }
                },
                FlowControl::Interrupt => match instr.mnemonic() {
                    iced_x86::Mnemonic::Int3 => {
                        break;
                    }
                    _ => match instr.op0_kind() {
                        iced_x86::OpKind::Immediate8 => {}
                        _ => panic!(
                            "Interrupt with op kind {:?} not supported (IP {:#x})",
                            instr.op0_kind(),
                            instr.ip()
                        ),
                    },
                },
                FlowControl::XbeginXabortXend => {
                    panic!("XbeginXabortXend not supported (ID {:#x})", instr.ip());
                }
                FlowControl::Exception => panic!("Exception! (ID {:#x})", instr.ip()),
            }
        }

        Ok(DecodedBlock::new(
            block_type,
            instructions,
            Vec::new(),
            branches,
        ))
    }

    pub fn decode_from(&self, addr: u64) -> Result<HashMap<u64, DecodedBlock>, Box<dyn Error>> {
        let mut hashmap = HashMap::new();

        let mut decode_queue = VecDeque::new();

        decode_queue.push_back(Branch {
            typ: BranchType::Call,
            from: 0,
            to: addr,
        });

        while let Some(item) = decode_queue.pop_front() {
            if hashmap.contains_key(&item.to) {
                continue;
            }

            let block = self.decode_at(item.typ.into(), item.to)?;
            for t in &block.branch_targets {
                decode_queue.push_back(*t);
            }
            hashmap.insert(item.to, block);
        }

        return Ok(hashmap);
    }

    fn merge_blocks(blocks: HashMap<u64, DecodedBlock>) -> IntervalTree<u64, DecodedBlock> {
        let blocks_iter = blocks.values();

        let tree = IntervalTree::from_iter(blocks_iter);

        // Find intersections for blocks
        let mut block_entry_points_to_process: BTreeSet<u64> =
            BTreeSet::from_iter(blocks.keys().map(|k| *k).sorted());

        // let mut block_entry_points_processed: HashSet<u64> = HashSet::new();

        // let merged_blocks: Vec<DecodedBlock> = blocks.values().cloned().collect();

        let mut merged_blocks = Vec::new();

        while let Some(ip) = block_entry_points_to_process.first().cloned() {
            let blocks_containing_ip: Vec<_> = tree.query_point(ip).collect();

            block_entry_points_to_process.remove(&ip);

            let mut merged_block = MergeOrRebalanceResult::None;

            for Element { range: _, value: b } in blocks_containing_ip {
                match merged_block {
                    MergeOrRebalanceResult::None => {
                        block_entry_points_to_process.remove(&b.range.start);
                        merged_block = MergeOrRebalanceResult::Single((*b).clone());
                    }
                    MergeOrRebalanceResult::Single(a) => {
                        block_entry_points_to_process.remove(&a.range.start);
                        block_entry_points_to_process.remove(&b.range.start);
                        merged_block = a.merge(b);
                    }
                    MergeOrRebalanceResult::Two(a1, a2) => {
                        block_entry_points_to_process.remove(&a1.range.start);
                        block_entry_points_to_process.remove(&a2.range.start);
                        block_entry_points_to_process.remove(&b.range.start);
                        merged_blocks.push(a1);
                        merged_block = a2.merge(b);
                    }
                }
            }

            match merged_block {
                MergeOrRebalanceResult::None => {}
                MergeOrRebalanceResult::Single(b) => merged_blocks.push(b),
                MergeOrRebalanceResult::Two(a, b) => {
                    merged_blocks.push(a);
                    merged_blocks.push(b);
                }
            }
        }

        merged_blocks.sort_by_key(|b| b.range.start);
        // Second pass: merge adjacent blocks if possible

        let merged_blocks = if merged_blocks.len() > 1 {
            merged_blocks
                .iter()
                .fold(im::Vector::new(), |acc: im::Vector<DecodedBlock>, b| {
                    if let Some(a) = acc.last() {
                        let mut acc_clone = acc.clone();

                        match a.merge(b) {
                            MergeOrRebalanceResult::None => {
                                acc_clone.push_back((*b).clone());
                            }
                            MergeOrRebalanceResult::Single(m) => {
                                acc_clone.pop_back();
                                acc_clone.push_back(m);
                            }
                            MergeOrRebalanceResult::Two(m1, m2) => {
                                acc_clone.pop_back();
                                acc_clone.push_back(m1);
                                acc_clone.push_back(m2);
                            }
                        }

                        acc_clone
                    } else {
                        let mut acc_clone = acc.clone();
                        acc_clone.push_back((*b).clone());
                        acc_clone
                    }
                })
                .into_iter()
                .collect()
        } else {
            merged_blocks
        };

        // Construct backreferences
        let mut block_backrefs: BTreeMap<u64, HashSet<Branch>> = BTreeMap::new();
        for block in &merged_blocks {
            for branch in &block.branch_targets {
                if let Some(backrefs) = block_backrefs.get_mut(&branch.to) {
                    backrefs.insert(*branch);
                } else {
                    block_backrefs.insert(branch.to, HashSet::from_iter(vec![*branch]));
                }
            }
        }

        let blocks_with_backrefs = merged_blocks.into_iter().map(|block| {
            // Find backrefs by going over the range in the map

            let backrefs_for_block = block_backrefs.range(block.range);
            let backrefs = backrefs_for_block
                .flat_map(|b| b.1.into_iter())
                .map(|b| *b)
                .collect();

            DecodedBlock::new(
                block.block_type,
                block.instructions,
                backrefs,
                block.branch_targets,
            )
        });

        IntervalTree::from_iter(blocks_with_backrefs)
    }
}

#[derive(Copy, Clone, PartialEq, Eq, Hash)]
pub enum BranchType {
    Jump,
    Call,
}

#[derive(Copy, Clone, PartialEq, Eq, Hash)]
pub struct Branch {
    pub typ: BranchType,
    pub from: u64,
    pub to: u64,
}

#[derive(Copy, Clone, PartialEq, Eq, Debug)]
pub enum BlockType {
    Function,
    Raw,
}

impl From<BranchType> for BlockType {
    fn from(value: BranchType) -> Self {
        match value {
            BranchType::Call => BlockType::Function,
            BranchType::Jump => BlockType::Raw,
        }
    }
}

#[derive(Clone)]
pub struct DecodedBlock {
    instructions: Vec<iced_x86::Instruction>,
    pub block_type: BlockType,
    pub branch_targets: Vec<Branch>,
    pub references: Vec<Branch>,
    pub range: Range<u64>,
}

impl<'a> From<&'a DecodedBlock> for Element<u64, &'a DecodedBlock> {
    fn from(value: &'a DecodedBlock) -> Element<u64, &'a DecodedBlock> {
        Element {
            range: value.range.clone(),
            value: value,
        }
    }
}

impl From<DecodedBlock> for Element<u64, DecodedBlock> {
    fn from(value: DecodedBlock) -> Element<u64, DecodedBlock> {
        Element {
            range: value.range.clone(),
            value: value,
        }
    }
}

pub enum MergeOrRebalanceResult {
    None,
    Single(DecodedBlock),
    Two(DecodedBlock, DecodedBlock),
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
}
