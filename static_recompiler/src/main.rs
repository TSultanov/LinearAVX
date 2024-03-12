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
        block.value.pretty_print();
    }

    Ok(())
}
