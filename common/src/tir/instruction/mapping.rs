use super::Instruction;
use iced_x86::Mnemonic;

pub fn map(instr: iced_x86::Instruction) -> Vec<Instruction> {
    match instr.mnemonic() {
        Mnemonic::INVALID => panic!("Invalid instruction"),
        
        _ => {
            let mut vec = Vec::new();
            vec.push(Instruction::wrap_native(instr));
            vec
        }
    }
}