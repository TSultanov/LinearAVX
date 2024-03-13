use std::error::Error;

use super::base::{BlockType, Branch, DecodeContinue, DecodedBlock, Decoder};

pub struct TextDecoder<'a> {
    pub base_address: u64,
    bitness: u32,
    data: &'a [u8],
}

impl<'a> TextDecoder<'_> {
    pub fn new(
        bitness: u32,
        base_address: u64,
        data: &'a [u8],
    ) -> TextDecoder<'a> {
        TextDecoder {
            base_address: base_address,
            bitness: bitness,
            data: data,
        }
    }
}

impl Decoder for TextDecoder<'_> {
    fn decode_at(&self, block_type: BlockType, addr: u64) -> Result<DecodedBlock, Box<dyn Error>> {
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

            let (cont, br) = Self::process_branch_instr(instr)?;
            if let Some(mut br) = br {
                branches.append(&mut br);
            }

            if cont == DecodeContinue::Break {
                break;
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
