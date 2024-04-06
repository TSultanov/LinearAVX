use std::{
    collections::{BTreeMap, BTreeSet, HashMap, HashSet, VecDeque},
    error::Error
    ,
};

use iced_x86::{FlowControl, Instruction};
use intervaltree::{Element, IntervalTree};
use itertools::Itertools;
use crate::decoder::decoded_block::DecodedBlock;

#[derive(PartialEq, Eq)]
pub enum DecodeContinue {
    Break,
    Continue,
}

pub trait Decoder {
    fn decode_at(&self, block_type: BlockType, addr: u64) -> Result<DecodedBlock, Box<dyn Error>>;

    fn decode_all_from(
        &self,
        addr: u64,
    ) -> Result<IntervalTree<u64, DecodedBlock>, Box<dyn Error>> {
        let raw_blocks = self.decode_from(addr)?;
        Ok(merge_blocks(raw_blocks))
    }

    fn decode_from(&self, addr: u64) -> Result<HashMap<u64, DecodedBlock>, Box<dyn Error>> {
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

    fn process_branch_instr(
        instr: Instruction,
    ) -> Result<(DecodeContinue, Option<Vec<Branch>>), String> {
        let mut branches = Vec::new();
        match instr.flow_control() {
            FlowControl::Next => Ok((DecodeContinue::Continue, None)),

            FlowControl::Return => Ok((DecodeContinue::Break, None)),

            FlowControl::UnconditionalBranch => {
                if instr.op0_kind() == iced_x86::OpKind::NearBranch64 {
                    let target = instr.near_branch_target();
                    branches.push(Branch {
                        typ: BranchType::Jump,
                        from: instr.ip(),
                        to: target,
                    });
                    return Ok((DecodeContinue::Break, Some(branches)));
                }
                Err(format!(
                    "UnconditionalBranch with op kind {:?} not supported (IP {:#x})",
                    instr.op0_kind(),
                    instr.ip()
                ))
            }

            FlowControl::IndirectBranch => match instr.op0_kind() {
                iced_x86::OpKind::Memory => Ok((DecodeContinue::Continue, None)),
                iced_x86::OpKind::Register => Ok((DecodeContinue::Continue, None)),
                _ => Err(format!(
                    "IndirectBranch with op kind {:?} not supported (IP {:#x})",
                    instr.op0_kind(),
                    instr.ip()
                )),
            },

            FlowControl::ConditionalBranch => {
                if instr.op0_kind() == iced_x86::OpKind::NearBranch64 {
                    branches.push(Branch {
                        typ: BranchType::Jump,
                        from: instr.ip(),
                        to: instr.near_branch_target(),
                    });

                    Ok((DecodeContinue::Continue, Some(branches)))
                } else {
                    Err(format!(
                        "ConditionalBranch with op kind {:?} not supported (IP {:#x})",
                        instr.op0_kind(),
                        instr.ip()
                    ))
                }
            }

            FlowControl::Call => match instr.op0_kind() {
                iced_x86::OpKind::NearBranch64 => {
                    branches.push(Branch {
                        typ: BranchType::Call,
                        from: instr.ip(),
                        to: instr.near_branch_target(),
                    });

                    Ok((DecodeContinue::Continue, Some(branches)))
                }
                _ => Err(format!(
                    "Call with op kind {:?} not supported (IP {:#x})",
                    instr.op0_kind(),
                    instr.ip()
                )),
            },

            FlowControl::IndirectCall => match instr.op0_kind() {
                iced_x86::OpKind::Memory => Ok((DecodeContinue::Continue, None)),
                iced_x86::OpKind::Register => Ok((DecodeContinue::Continue, None)),
                _ => Err(format!(
                    "IndirectCall with op kind {:?} not supported (IP {:#x})",
                    instr.op0_kind(),
                    instr.ip()
                )),
            },
            FlowControl::Interrupt => match instr.mnemonic() {
                iced_x86::Mnemonic::Int3 => Ok((DecodeContinue::Break, None)),
                _ => match instr.op0_kind() {
                    iced_x86::OpKind::Immediate8 => Ok((DecodeContinue::Continue, None)),
                    _ => Err(format!(
                        "Interrupt with op kind {:?} not supported (IP {:#x})",
                        instr.op0_kind(),
                        instr.ip()
                    )),
                },
            },
            FlowControl::XbeginXabortXend => {
                panic!("XbeginXabortXend not supported (IP {:#x})", instr.ip());
            }
            FlowControl::Exception => Ok((DecodeContinue::Continue, None)),
        }
    }
}

fn merge_blocks(blocks: HashMap<u64, DecodedBlock>) -> IntervalTree<u64, DecodedBlock> {
    let blocks_iter = blocks.values();

    let tree = IntervalTree::from_iter(blocks_iter);

    // Find intersections for blocks
    let mut block_entry_points_to_process: BTreeSet<u64> =
        BTreeSet::from_iter(blocks.keys().map(|k| *k).sorted());

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

    let merged_blocks: Vec<DecodedBlock> = if merged_blocks.len() > 1 {
        merged_blocks
            .iter()
            .fold(im::Vector::new(), |acc: im::Vector<DecodedBlock>, b| {
                if let Some(a) = acc.last() {
                    let mut acc_clone = acc.clone();

                    match a.merge(&b) {
                        MergeOrRebalanceResult::None => {
                            acc_clone.push_back(b.clone());
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
                    acc_clone.push_back(b.clone());
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

    // Split blocks at backrefs
    let split_blocks = blocks_with_backrefs.flat_map(|b| {
        if b.references.len() > 1 {
            let mut blocks = Vec::new();
            let mut current_block = Vec::new();
            let mut br_iter = b.references.iter();
            let mut prev_branch = br_iter.next();
            let mut curr_br = br_iter.next();

            for instr in b.instructions {
                if !current_block.is_empty()
                    && curr_br.is_some()
                    && curr_br.unwrap().to == instr.ip()
                {
                    let br = prev_branch.unwrap();
                    blocks.push(DecodedBlock::new(
                        match br.typ {
                            BranchType::Jump => BlockType::Raw,
                            BranchType::Call => BlockType::Function,
                        },
                        current_block.clone(),
                        vec![*br],
                        vec![], //b.branch_targets.clone(),
                    ));
                    current_block = Vec::new();
                    prev_branch = curr_br;
                    curr_br = br_iter.next();
                }

                current_block.push(instr);
            }

            let br = prev_branch.unwrap();
            blocks.push(DecodedBlock::new(
                match br.typ {
                    BranchType::Jump => BlockType::Raw,
                    BranchType::Call => BlockType::Function,
                },
                current_block.clone(),
                vec![*br],
                vec![], //b.branch_targets.clone(),
            ));

            blocks
        } else {
            vec![b]
        }
    });

    IntervalTree::from_iter(split_blocks)
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
