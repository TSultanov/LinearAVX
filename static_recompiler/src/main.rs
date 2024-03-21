use common::{compiler::analyze_block, decoder::base::Decoder};
use object::{Object, ObjectSection};
use static_recompiler::{create_decoder, Config};
use std::{env, error::Error, fs};

fn main() -> Result<(), Box<dyn Error>> {
    let config = Config::build(env::args())?;

    let binary_data = fs::read(config.input_file_path)?;
    let file = object::File::parse(&*binary_data)?;
    
    let format = file.format();
    println!("Format: {:?}", format);

    let arch = file.architecture();
    println!("Arch: {:?}", arch);

    let entry = file.entry();
    println!("Entry address: {:#x}", entry);

    let base_address = file.relative_address_base();
    println!("Base address: {:#x}", base_address);

    for s in file.sections() {
        println!("{} at {:#x}", s.name()?, s.address());
    }

    let text_decoder = create_decoder(&file)?;

    let blocks = text_decoder.decode_all_from(entry)?;
    for block in blocks {
        if block.value.needs_recompiling() && block.value.range.start == 0x141fb09a0 /*0x1401cd710*/ {
            // block.value.pretty_print();
            analyze_block(&block.value);
            break;
        }
    }

    Ok(())
}
