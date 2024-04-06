use std::{error::Error, ffi::c_void, fmt::Display};

use windows::Win32::{Foundation::{GetLastError, HANDLE}, System::Memory::{VirtualAllocEx, MEM_COMMIT, MEM_RESERVE, PAGE_EXECUTE_READWRITE}};
use crate::debugger::process::memory_info::MemoryInfo;

#[derive(Debug, Clone)]
pub enum AllocatorError {
    InitializationFailed,
    OutOfSpace,
    NotTheLastAllocation
}

impl Display for AllocatorError {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        write!(f, "Failed to allocate memory: {:?}", self)
    }
}

impl Error for AllocatorError {}

pub struct Allocator {
    h_process: HANDLE,
    requested_base: u64,
    max_size: usize,
    allocated: usize,
    allocation_base: Option<u64>,
    allocations: Vec<(u64, usize)>,
    memory_info: MemoryInfo,
}

impl Allocator {
    pub fn new(h_process: HANDLE, memory_info: MemoryInfo, code_base: u64, max_size: usize) -> Allocator {
        Allocator {
            h_process,
            requested_base: code_base,
            max_size,
            allocated: 0,
            allocation_base: None,
            allocations: Vec::new(),
            memory_info,
        }
    }

    fn init(&mut self) -> Result<(), Box<dyn Error>> {
        let free_blocks = self.memory_info.get_nearest_free(self.requested_base, self.max_size);
        for b in free_blocks {
            let base = unsafe {
                VirtualAllocEx(
                    self.h_process,
                    Some(b as *const c_void),
                    self.max_size,
                    MEM_RESERVE,
                    PAGE_EXECUTE_READWRITE,
                ) as u64
            };

            if base != 0 {
                self.allocation_base = Some(base);
                break;
            }
        }

        if self.allocation_base.is_none() {
            unsafe { GetLastError()? };
        }

        Ok(())
    }

    fn commit(&self, addr: u64, size: usize) -> Result<(), Box<dyn Error>> {
        let base = unsafe {
            VirtualAllocEx(
                self.h_process,
                Some(addr as *const c_void),
                size,
                MEM_COMMIT,
                PAGE_EXECUTE_READWRITE,
            ) as u64
        };

        if base == 0 {
            unsafe { GetLastError()? };
        }
        Ok(())
    }

    pub fn allocate(&mut self, size: usize) -> Result<u64, Box<dyn Error>> {
        if self.allocation_base == None {
            self.init()?;
        }

        if let Some(base) = self.allocation_base {
            let address = base + self.allocated as u64;
            let size = if size % 8 != 0 {
                size + (8 - size % 8)
            } else {
                size
            };
            self.allocated += size;
            if self.allocated > self.max_size {
                Err(Box::new(AllocatorError::OutOfSpace))
            } else {
                self.commit(address, size)?;
                self.allocations.push((address, size));
                Ok(address)
            }
        } else {
            Err(Box::new(AllocatorError::InitializationFailed))
        }
    }

    pub fn allocation_increase(&mut self, addr: u64, new_size: usize) -> Result<(), Box<dyn Error>> {
        if let Some((last_addr, last_size)) = self.allocations.last() {
            if *last_addr != addr {
                return Err(Box::new(AllocatorError::NotTheLastAllocation));
            }

            if new_size <= *last_size {
                return Ok(());
            }

            let increase = new_size - *last_size;
            let increase = if increase % 8 != 0 {
                increase + (8 - increase % 8)
            } else {
                increase
            };
            self.commit(addr, *last_size + increase)?;
            self.allocated += increase;
            Ok(())
        } else {
            Err(Box::new(AllocatorError::NotTheLastAllocation))
        }
    }
}