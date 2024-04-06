mod allocator;
mod memory_info;

use std::{
    cell::RefCell,
    collections::HashMap,
    error::Error,
    ffi::c_void,
    mem::{MaybeUninit, size_of},
    ops::Range,
    ptr::addr_of_mut,
};

use iced_x86::code_asm::{CodeAssembler, ebx, rax};
use windows::{
    core::{HSTRING, PCSTR},
    Win32::{
        Foundation::HANDLE,
        System::{
            Diagnostics::Debug::{
                CREATE_PROCESS_DEBUG_INFO,
                CREATE_THREAD_DEBUG_INFO, FlushInstructionCache, ReadProcessMemory,
                WriteProcessMemory,
            },
            LibraryLoader::{GetModuleHandleW, GetProcAddress},
            Threading::{
                GetProcessId, GetProcessInformation, GetThreadId, PROCESS_MACHINE_INFORMATION
                , ProcessMachineTypeInfo,
            },
        },
    },
};
use crate::debugger::thread::{AlignedContext, Thread};

use self::{allocator::Allocator, memory_info::MemoryInfo};

pub struct Process {
    pub pid: u32,
    pub base_address: u64,
    pub entry_point: u64,
    pub machine_info: PROCESS_MACHINE_INFORMATION,
    pub tls_offset: Option<u64>,
    h_process: HANDLE,
    threads: HashMap<u32, Thread>,
    breakpoints: HashMap<u64, u8>,
    allocator: RefCell<Allocator>,
    pub memory_info: MemoryInfo,
}

impl Process {
    pub fn new(process_info: &CREATE_PROCESS_DEBUG_INFO) -> Process {
        let pid = unsafe { GetProcessId(process_info.hProcess) };
        let tid = unsafe { GetThreadId(process_info.hThread) };

        let mut threads = HashMap::new();
        threads.insert(
            tid,
            Thread {
                tid,
                h_thread: process_info.hThread,
            },
        );

        let mi = unsafe {
            let mut mi: MaybeUninit<PROCESS_MACHINE_INFORMATION> = MaybeUninit::zeroed();
            GetProcessInformation(
                process_info.hProcess,
                ProcessMachineTypeInfo,
                mi.as_mut_ptr() as *mut c_void,
                size_of::<PROCESS_MACHINE_INFORMATION>() as u32,
            )
            .expect("Failed to fetch machine information for process");
            mi.assume_init()
        };

        let max_code_alloc_size = 1024 * 1024 * 1024;
        let memory_info = MemoryInfo::new(process_info.hProcess);

        Process {
            pid,
            base_address: process_info.lpBaseOfImage as u64,
            entry_point: process_info.lpStartAddress.unwrap() as u64,
            h_process: process_info.hProcess,
            threads,
            tls_offset: None,
            machine_info: mi,
            breakpoints: HashMap::new(),
            allocator: RefCell::new(Allocator::new(
                process_info.hProcess,
                memory_info
                    .get_nearest_free(process_info.lpBaseOfImage as u64, max_code_alloc_size),
                max_code_alloc_size,
            )),
            memory_info,
        }
    }

    pub fn add_thread(&mut self, thread_info: &CREATE_THREAD_DEBUG_INFO) {
        let thread = Thread::new(thread_info);
        self.threads.insert(thread.tid, thread);
    }

    pub fn remove_thread(&mut self, tid: u32) {
        self.threads.remove(&tid);
    }

    pub fn breakpoint_set(&mut self, addr: u64) -> Result<(), Box<dyn Error>> {
        let real_byte = self.read_memory(addr, 1)?;
        self.write_memory(addr, &vec![0xcc])?;
        self.breakpoints.insert(addr, *real_byte.first().unwrap());
        Ok(())
    }

    pub fn breakpoint_remove(&mut self, addr: u64) -> Result<(), Box<dyn Error>> {
        if let Some(real_byte) = self.breakpoints.get(&addr) {
            self.write_memory(addr, &vec![*real_byte])?;
        }
        Ok(())
    }

    pub fn read_memory(&self, addr: u64, size: usize) -> Result<Vec<u8>, Box<dyn Error>> {
        let mut memory: Vec<u8> = Vec::with_capacity(size);
        unsafe {
            let mut bytes_read = 0;
            ReadProcessMemory(
                self.h_process,
                addr as *const c_void,
                memory.as_mut_ptr().cast(),
                size,
                Some(addr_of_mut!(bytes_read)),
            )?;
            memory.set_len(bytes_read);
        }
        Ok(memory)
    }

    pub fn write_memory(&self, addr: u64, data: &[u8]) -> Result<(), Box<dyn Error>> {
        unsafe {
            WriteProcessMemory(
                self.h_process,
                addr as *const c_void,
                data.as_ptr().cast(),
                data.len(),
                None,
            )?;
            FlushInstructionCache(self.h_process, Some(addr as *const c_void), data.len())?;
        }
        Ok(())
    }

    pub fn get_thread(&self, tid: u32) -> Option<&Thread> {
        self.threads.get(&tid)
    }

    pub fn suspend(&self) {
        for (_, thread) in &self.threads {
            thread.suspend();
        }
    }

    pub fn resume(&self) {
        for (_, thread) in &self.threads {
            thread.resume();
        }
    }

    pub fn set_alloc_tls_execution(&mut self, tid: u32) -> Option<AlignedContext> {
        if let Some(thread) = self.get_thread(tid) {
            // Save thread context
            let mut context = thread.get_context().unwrap();
            context.ctx.Rip -= 1;

            let mut new_context = context.clone();

            let exec_addr = self.allocator.borrow_mut().allocate(20).unwrap();
            let shellcode = jit_tls_alloc(exec_addr).unwrap();
            self.write_memory(exec_addr, &shellcode).unwrap();

            new_context.ctx.Rip = exec_addr as u64;

            thread.set_context(&new_context).unwrap();

            Some(context)
        } else {
            None
        }
    }

    pub fn set_tls_offset(&mut self, tls_offset: u64) {
        self.tls_offset = Some(tls_offset);
    }

    pub fn rewrite_code(
        &self,
        code_range: Range<u64>,
        asm: &mut CodeAssembler,
    ) -> Result<(), Box<dyn Error>> {
        // 1. Alloc initial amount of memory for new code
        let func_size = (code_range.end - code_range.start) as usize;
        println!(
            "For function at {:#x} (len {}):",
            code_range.start, func_size,
        );
        let base = self.allocator.borrow_mut().allocate(func_size)?;

        let mut trampoline_asm = CodeAssembler::new(64)?;
        trampoline_asm.jmp(base)?;
        trampoline_asm.int3()?;
        trampoline_asm.int3()?;

        let mut trampoline = trampoline_asm.assemble(code_range.start)?;

        if trampoline.len() > func_size {
            return Ok(());
        }

        for _ in 0..(func_size - trampoline.len()) {
            trampoline.push(0xcc);
        }

        // 2. Assemble new code with the new allocation start as a base address
        // in case block is not ending with RET, jump back past the end of the original code
        asm.nop()?;
        asm.nop()?;
        asm.jmp(code_range.end)?;
        asm.int3()?;
        asm.int3()?;
        let assembled_block = asm.assemble(base)?;

        // 3. Bump allocated amount if necessary
        if assembled_block.len() > func_size {
            self.allocator
                .borrow_mut()
                .allocation_increase(base, assembled_block.len())?
        }

        // 4. Copy assembled code
        self.write_memory(base, &assembled_block)?;
        println!(
            "  written recompiled version at {:#x} (len {})",
            base,
            assembled_block.len()
        );

        // 5. Set up trampoline
        self.write_memory(code_range.start, &trampoline)?;

        println!(
            "  written trampoline at {:#x} (len {})",
            code_range.start,
            trampoline.len()
        );

        Ok(())
    }
}

fn jit_tls_alloc(base_addr: u64) -> Result<Vec<u8>, Box<dyn Error>> {
    // Abuse the fact that kernel32.dll is loaded at the same base address in every x64 process
    let module_name = HSTRING::from("kernel32.dll");
    let module = unsafe { GetModuleHandleW(&module_name)? };
    let proc_name = "TlsAlloc\0";
    let proc_address =
        unsafe { GetProcAddress(module, PCSTR::from_raw(proc_name.as_ptr())).unwrap() } as u64;

    let mut a = CodeAssembler::new(64)?;
    a.mov(rax, proc_address)?; // 10B
    a.call(rax)?; // 2B
    a.mov(ebx, 0xdeadf00du32)?; // 5B
    a.int3()?; // 1B
    a.int3()?; // 1B
    a.int3()?; // 1B
               // 20B total

    Ok(a.assemble(base_addr)?)
}
