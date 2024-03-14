use crate::{decoder::base::DecodedBlock, tir::Instruction};

pub fn analyze_block(block: &DecodedBlock) {
    let ir: Vec<_> = block
        .instructions
        .iter()
        .flat_map(|i| Instruction::translate_native(*i))
        .collect();

    for i in ir {
        println!("{:?}", i);
    }
}
