use std::error::Error;

use crate::debugger::process::Process;

use super::base::{Branch, DecodeContinue, DecodedBlock, Decoder};

pub struct ProcessDecoder<'a> {
    bitness: u32,
    process: &'a Process,
    default_read_size: usize,
}

impl<'a> ProcessDecoder<'_> {
    pub fn new(process: &Process) -> ProcessDecoder {
        ProcessDecoder {
            bitness: 64,
            process: process,
            default_read_size: 1024,
        }
    }
}

impl Decoder for ProcessDecoder<'_> {
    fn decode_at(
        &self,
        block_type: super::base::BlockType,
        addr: u64,
    ) -> Result<DecodedBlock, Box<dyn Error>> {
        let mut instructions = Vec::new();

        let mut branches: Vec<Branch> = Vec::new();

        let mut next_addr = addr;
        loop {
            let data = self.process.read_memory(next_addr, self.default_read_size)?;

            let decoder = iced_x86::Decoder::with_ip(
                self.bitness,
                &data,
                next_addr,
                iced_x86::DecoderOptions::NONE,
            );

            let mut cont = DecodeContinue::Continue;
            for instr in decoder {
                if instr.is_invalid() {
                    cont = DecodeContinue::Continue;
                    break;
                }

                instructions.push(instr);

                let br;
                (cont, br) = Self::process_branch_instr(instr)?;
                if let Some(mut br) = br {
                    branches.append(&mut br);
                }
    
                if cont == DecodeContinue::Break {
                    break;
                }
            }

            if cont == DecodeContinue::Break {
                break;
            }

            let next_ip = instructions.last().expect("[BUG] No instructions decoded!").next_ip();
            // Prevent inifinite loop in case the decoding repeatedly fails
            if next_ip != next_addr {
                next_addr = next_ip;
            }
        }

        Ok(DecodedBlock::new(
            block_type,
            instructions,
            Vec::new(),
            branches,
        ))
    }
}
