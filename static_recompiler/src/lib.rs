use std::error::Error;

use object::{Object, ObjectSection};
use common::decoder::TextDecoder;

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

pub fn create_decoder<'a>(file: &'a object::File) -> Result<TextDecoder<'a>, Box<dyn Error>> {
    let arch = file.architecture();

    let text_section = file.section_by_name(".text").unwrap();

    let bitness = match arch {
        object::Architecture::X86_64 => 64,
        object::Architecture::I386 => 32,
        _ => return Err("Unsupported architecture")?,
    };

    let base_address = text_section.address();

    TextDecoder::<'a>::new(bitness, base_address, text_section.data()?)
}
