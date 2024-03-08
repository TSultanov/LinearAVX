use object::Object;
use static_recompiler::{Config, TextDecoder};
use std::{env, error::Error, fs};

fn main() -> Result<(), Box<dyn Error>> {
    let config = Config::build(env::args())?;

    let binary_data = fs::read(config.input_file_path)?;
    let file = object::File::parse(&*binary_data)?;
    
    let format = file.format();
    println!("Format: {:?}", format);

    let arch = file.architecture();
    println!("Arch: {:?}", arch);

    let base_address = file.relative_address_base();
    println!("Base address: {:#x}", base_address);

    let text_decoder = TextDecoder::new(&file)?;

    let block = text_decoder.decode_at(text_decoder.base_address)?;
    block.pretty_print();

    Ok(())
}
