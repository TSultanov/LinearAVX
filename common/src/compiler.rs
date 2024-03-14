use crate::{decoder::base::DecodedBlock, tir::Instruction};

pub fn analyze_block(block: &DecodedBlock) {
    let ir: Vec<_> = block
        .instructions
        .iter()
        .map(|i| Instruction::wrap_native(*i))
        .flat_map(|i| i.map())
        .collect();

    for i in ir {
        println!("{:?}", i);
    }
}
