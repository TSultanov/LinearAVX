use std::{
    error::Error,
    ffi::c_void,
    mem::{size_of, MaybeUninit},
};

use itertools::Itertools;
use windows::Win32::{
    Foundation::{GetLastError, HANDLE},
    System::Memory::{VirtualQueryEx, MEMORY_BASIC_INFORMATION, MEM_FREE},
};

pub struct MemoryInfo {
    h_process: HANDLE,
}

impl MemoryInfo {
    pub fn new(h_process: HANDLE) -> MemoryInfo {
        MemoryInfo {
            h_process: h_process,
        }
    }

    pub fn get_memory_info(&self, addr: u64) -> Result<MEMORY_BASIC_INFORMATION, Box<dyn Error>> {
        let mut buffer = MaybeUninit::<MEMORY_BASIC_INFORMATION>::zeroed();

        let (info, real_size) = unsafe {
            let real_size = VirtualQueryEx(
                self.h_process,
                Some(addr as *const c_void),
                buffer.as_mut_ptr(),
                size_of::<MEMORY_BASIC_INFORMATION>(),
            );
            (buffer.assume_init(), real_size)
        };

        if real_size == 0 {
            unsafe { GetLastError()? };
        }

        Ok(info)
    }

    pub fn get_memory_map_starting_at(&self, start_addr: u64) -> Vec<MEMORY_BASIC_INFORMATION> {
        let mut result = Vec::new();
        let mut start_addr = start_addr;
        loop {
            match self.get_memory_info(start_addr) {
                Ok(info) => {
                    result.push(info);
                    start_addr = info.BaseAddress as u64 + info.RegionSize as u64;
                }
                Err(_) => break,
            };
        }
        result
    }

    pub fn get_nearest_free(&self, starting_at: u64, size: usize) -> u64 {
        let map = self.get_memory_map_starting_at(0);
        let suitable_maps = map
            .iter()
            .filter(|m| m.RegionSize >= size && m.State == MEM_FREE)
            .sorted_by_key(|a| (a.BaseAddress as u64).abs_diff(starting_at))
            .collect_vec();
        let suitable = suitable_maps.get(1).unwrap();
        let base_addr = if (suitable.BaseAddress as u64) < starting_at {
            (suitable.BaseAddress as usize + suitable.RegionSize - size) as u64
        } else {
            suitable.BaseAddress as u64
        };
        println!("Distance: {}", base_addr.abs_diff(starting_at));
        base_addr
    }
}
