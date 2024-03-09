use std::{
    collections::{BTreeMap, HashMap, HashSet, VecDeque},
    error::Error,
    ops::Range,
};

use iced_x86::{FlowControl, Formatter};
use intervaltree::{Element, IntervalTree};
use object::{Object, ObjectSection};

pub struct Config {
    pub input_file_path: String,
}

impl Config {
    pub fn build(mut args: impl Iterator<Item = String>) -> Result<Config, &'static str> {
        args.next();

        let input_file_name = match args.next() {
            Some(arg) => arg,
            None => return Err("Missing input file"),
        };

        Ok(Config {
            input_file_path: input_file_name,
        })
    }
}

pub struct TextDecoder<'a> {
    pub base_address: u64,
    pub section_end: u64,
    bitness: u32,
    section: object::Section<'a, 'a>,
}

impl<'a> TextDecoder<'_> {
    pub fn new(file: &'a object::File) -> Result<TextDecoder<'a>, Box<dyn Error>> {
        let arch = file.architecture();

        let text_section = file.section_by_name(".text").unwrap();

        let bitness = match arch {
            object::Architecture::X86_64 => 64,
            object::Architecture::I386 => 32,
            _ => return Err("Unsupported architecture")?,
        };

        let base_address = text_section.address();

        Ok(TextDecoder {
            base_address: base_address,
            section_end: base_address + text_section.size(),
            bitness: bitness,
            section: text_section,
        })
    }

    pub fn decode_all_from(&self, addr: u64) -> Result<Vec<DecodedBlock>, Box<dyn Error>> {
        let raw_blocks = self.decode_from(addr)?;
        Ok(Self::merge_blocks(raw_blocks))
    }

    pub fn decode_at(&self, addr: u64) -> Result<DecodedBlock, Box<dyn Error>> {
        let bytes = self.section.data()?;

        let position: usize = (addr - self.base_address).try_into().unwrap();

        let decoder = iced_x86::Decoder::with_ip(
            self.bitness,
            &bytes[position..],
            addr,
            iced_x86::DecoderOptions::NONE,
        );

        let mut instructions = Vec::new();

        let mut branch_addresses = Vec::new();

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
                        branch_addresses.push(target);
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
                        branch_addresses.push(instr.near_branch_target());
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
                        branch_addresses.push(instr.near_branch_target());
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

        Ok(DecodedBlock::new(instructions, branch_addresses))
    }

    pub fn decode_from(&self, addr: u64) -> Result<HashMap<u64, DecodedBlock>, Box<dyn Error>> {
        let mut hashmap = HashMap::new();

        let mut decode_queue = VecDeque::new();

        decode_queue.push_back(addr);

        while let Some(item) = decode_queue.pop_front() {
            if hashmap.contains_key(&item) {
                continue;
            }

            let block = self.decode_at(item)?;
            for t in &block.branch_targets {
                decode_queue.push_back(*t);
            }
            hashmap.insert(item, block);
        }

        return Ok(hashmap);
    }

    fn merge_blocks(blocks: HashMap<u64, DecodedBlock>) -> Vec<DecodedBlock> {
        let blocks_iter = blocks.values();

        let tree = IntervalTree::from_iter(blocks_iter);

        // Find intersections for blocks
        let mut block_entry_points_to_process: HashSet<u64> =
            HashSet::from_iter(blocks.keys().map(|k| *k));

        let mut block_entry_points_to_process_iter = block_entry_points_to_process.drain();

        let mut merged_blocks = Vec::new();

        while let Some(ip) = block_entry_points_to_process_iter.next() {
            let blocks_containing_ip = tree.query_point(ip);
            let merged_block = blocks_containing_ip.fold(None, |acc: Option<DecodedBlock>, b| {
                if let Some(a) = acc {
                    a.merge(b.value)
                } else {
                    Some((*b.value).clone())
                }
            });

            if let Some(block) = merged_block {
                merged_blocks.push(block);
            } else {
                panic!("BUG: Tried to merge non-intersecting blocks");
            }
        }

        merged_blocks
    }
}

#[derive(Clone)]
pub struct DecodedBlock {
    instructions: Vec<iced_x86::Instruction>,
    pub branch_targets: Vec<u64>,
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

impl DecodedBlock {
    pub fn new(instructions: Vec<iced_x86::Instruction>, branch_targets: Vec<u64>) -> DecodedBlock {
        // find max and min address of the block and filter branch targets which are not in the block
        let min_address = instructions.iter().map(|i| i.ip()).min().unwrap();
        let max_address = instructions.iter().map(|i| i.ip()).max().unwrap();

        let mut branch_targets_filtered = Vec::new();
        for t in branch_targets {
            if t < min_address || t > max_address {
                branch_targets_filtered.push(t);
            }
        }

        DecodedBlock {
            instructions,
            branch_targets: branch_targets_filtered,
            range: Range {
                start: min_address,
                end: max_address+1,
            },
        }
    }

    pub fn merge(&self, other: &DecodedBlock) -> Option<DecodedBlock> {
        let left_block = if self.range.start <= other.range.start {
            &self
        } else {
            &other
        };

        let right_block = if self.range.end >= other.range.end {
            &self
        } else {
            &other
        };

        if left_block.range.end < right_block.range.start {
            return None;
        }

        let new_range = Range {
            start: left_block.range.start,
            end: left_block.range.end,
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

        let branch_targets_new = left_block.branch_targets.iter()
            .chain(right_block.branch_targets.iter())
            .filter(|ip| !new_range.contains(ip))
            .map(|ip| *ip)
            .collect();

        Some(DecodedBlock::new(instuctions_new, branch_targets_new))
    }

    pub fn pretty_print(&self) {
        let mut formatter = iced_x86::NasmFormatter::new();

        formatter.options_mut().set_digit_separator("`");
        formatter.options_mut().set_first_operand_char_index(10);
        formatter.options_mut().set_branch_leading_zeros(false);

        let mut output = String::new();

        println!("\nFunction at {:#x}:", self.instructions[0].ip());

        for instr in &self.instructions {
            output.clear();
            formatter.format(&instr, &mut output);

            print!("{:X} ", instr.ip());
            println!(" {}", output);
        }
        if !self.branch_targets.is_empty() {
            println!("Branch targets:");
            for t in &self.branch_targets {
                println!("- {:#x}", t);
            }
        }
        println!("*****");
    }
}
