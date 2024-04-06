use std::{error::Error, ffi::c_void, fmt::Display};

use windows::Win32::{Foundation::{GetLastError, HANDLE}, System::Memory::{VirtualAllocEx, MEM_COMMIT, MEM_RESERVE, PAGE_EXECUTE_READWRITE}};

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
}

impl Allocator {
    pub fn new(h_process: HANDLE, requested_base: u64, max_size: usize) -> Allocator {
        Allocator {
            h_process: h_process,
            requested_base: requested_base,
            max_size: max_size,
            allocated: 0,
            allocation_base: None,
            allocations: Vec::new(),
        }
    }

    fn init(&mut self) -> Result<(), Box<dyn Error>> {
        let base = unsafe {
            VirtualAllocEx(
                self.h_process,
                Some(self.requested_base as *const c_void),
                self.max_size,
                MEM_COMMIT | MEM_RESERVE,
                PAGE_EXECUTE_READWRITE,
            ) as u64
        };
        if base == 0 {
            unsafe { GetLastError()? };
        }
        self.allocation_base = Some(base);
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
                self.allocations.push((address, size));
                Ok(address)
            }
        } else {
            Err(Box::new(AllocatorError::InitializationFailed))
        }
    }

    pub fn allocation_increase(&mut self, addr: u64, new_size: usize) -> Result<(), AllocatorError> {
        if let Some((last_addr, last_size)) = self.allocations.last() {
            if *last_addr != addr {
                return Err(AllocatorError::NotTheLastAllocation);
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
            self.allocated += increase;
            Ok(())
        } else {
            Err(AllocatorError::NotTheLastAllocation)
        }
    }
}