use std::error::Error;

use iced_x86::{FlowControl, Formatter};
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
            bitness: bitness,
            section: text_section,
        })
    }

    pub fn decode_at(&self, addr: u64) -> Result<DecodedBlock, Box<dyn Error>> {
        let bytes = self.section.data()?;

        let mut decoder = iced_x86::Decoder::with_ip(
            self.bitness,
            bytes,
            self.base_address,
            iced_x86::DecoderOptions::NONE,
        );
        println!("{:#x} {:#x}", addr, self.base_address);
        let position: usize = (addr - self.base_address).try_into().unwrap();

        decoder.set_position(position)?;

        let mut instructions = Vec::new();

        let mut branch_addresses = Vec::new();
        let mut call_addresses = Vec::new();

        for instr in decoder {
            instructions.push(instr);

            match instr.flow_control() {
                FlowControl::Next => {}

                FlowControl::Return => {
                    break;
                }

                FlowControl::UnconditionalBranch => {
                    println!("UnconditionalBranch to {:?}", instr.op0_kind());
                    if instr.op0_kind() == iced_x86::OpKind::NearBranch64 {
                        let _target = instr.near_branch_target();
                        // You could check if it's just jumping forward a few bytes and follow it
                        // but this is a simple example so we'll fail.
                    }
                    panic!("UnconditionalBranch Not supported");
                }

                FlowControl::IndirectBranch => {
                    panic!("IndirectBranch Not supported");
                }

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

                FlowControl::Call => {
                    if instr.op0_kind() == iced_x86::OpKind::NearBranch64 {
                        call_addresses.push(instr.near_branch_target());
                    } else {
                        panic!(
                            "Call with op kind {:?} not supported (IP {:#x})",
                            instr.op0_kind(),
                            instr.ip()
                        );
                    }
                }
                FlowControl::IndirectCall => {
                    panic!(
                        "Call with op kind {:?} not supported (IP {:#x})",
                        instr.op0_kind(),
                        instr.ip()
                    );
                }
                FlowControl::Interrupt => {}
                FlowControl::XbeginXabortXend => {
                    panic!("XbeginXabortXend not supported");
                }
                FlowControl::Exception => panic!("Exception!"),
            }
        }

        Ok(DecodedBlock::new(
            instructions,
            call_addresses,
            branch_addresses,
        ))
    }
}

pub struct DecodedBlock {
    instructions: Vec<iced_x86::Instruction>,
    call_targets: Vec<u64>,
    branch_targets: Vec<u64>,
}

impl DecodedBlock {
    pub fn new(
        instructions: Vec<iced_x86::Instruction>,
        call_targets: Vec<u64>,
        branch_targets: Vec<u64>,
    ) -> DecodedBlock {
        // find max and min address of the block and filter branch targets which are not in the block
        let min_address = instructions.iter().map(|i| i.ip()).min().unwrap();
        let max_address = instructions.iter().map(|i| i.ip()).max().unwrap();

        let mut branch_targets_filtered = Vec::new();
        for t in branch_targets {
            if t < min_address && t > max_address {
                branch_targets_filtered.push(t);
            }
        }

        DecodedBlock {
            instructions,
            call_targets,
            branch_targets: branch_targets_filtered,
        }
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
        if !self.call_targets.is_empty() {
            println!("Call targets:");
            for t in &self.call_targets {
                println!("- {:#x}", t);
            }
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
