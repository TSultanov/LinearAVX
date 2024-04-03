mod allocator;
mod memory_info;

use std::{
    cell::RefCell,
    collections::HashMap,
    error::Error,
    ffi::c_void,
    mem::{size_of, MaybeUninit},
    ops::Range,
    ptr::addr_of_mut,
};

use iced_x86::code_asm::{ebx, rax, CodeAssembler};
use windows::{
    core::{HSTRING, PCSTR},
    Win32::{
        Foundation::HANDLE,
        System::{
            Diagnostics::Debug::{
                FlushInstructionCache, GetThreadContext, ReadProcessMemory, SetThreadContext,
                WriteProcessMemory, CONTEXT, CONTEXT_ALL_AMD64, CREATE_PROCESS_DEBUG_INFO,
                CREATE_THREAD_DEBUG_INFO,
            },
            LibraryLoader::{GetModuleHandleW, GetProcAddress},
            Threading::{
                GetProcessId, GetProcessInformation, GetThreadId, ProcessMachineTypeInfo,
                ResumeThread, SuspendThread, PROCESS_MACHINE_INFORMATION,
            },
        },
    },
};

use self::{allocator::Allocator, memory_info::MemoryInfo};

pub struct Thread {
    pub tid: u32,
    h_thread: HANDLE,
}

impl Thread {
    pub fn new(thread_info: &CREATE_THREAD_DEBUG_INFO) -> Thread {
        let tid = unsafe { GetThreadId(thread_info.hThread) };

        Thread {
            tid: tid,
            h_thread: thread_info.hThread,
        }
    }

    pub fn suspend(&self) {
        unsafe { SuspendThread(self.h_thread) };
    }

    pub fn resume(&self) {
        unsafe { ResumeThread(self.h_thread) };
    }

    pub fn get_context(&self) -> Result<AlignedContext, Box<dyn Error>> {
        let mut ctx = AlignedContext::default();
        ctx.ctx.ContextFlags = CONTEXT_ALL_AMD64;
        unsafe { GetThreadContext(self.h_thread, &mut ctx.ctx)? };
        Ok(ctx)
    }

    pub fn set_context(&self, context: &AlignedContext) -> Result<(), windows::core::Error> {
        unsafe { SetThreadContext(self.h_thread, &context.ctx) }
    }
}

#[repr(align(16))]
#[derive(Default, Clone)]
pub struct AlignedContext {
    pub ctx: CONTEXT,
}

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
                tid: tid,
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

        let max_code_alloc_size = 1024*1024*1024;
        let memory_info = MemoryInfo::new(process_info.hProcess);

        Process {
            pid: pid,
            base_address: process_info.lpBaseOfImage as u64,
            entry_point: process_info.lpStartAddress.unwrap() as u64,
            h_process: process_info.hProcess,
            threads: threads,
            tls_offset: None,
            machine_info: mi,
            breakpoints: HashMap::new(),
            allocator: RefCell::new(Allocator::new(
                process_info.hProcess,
                memory_info.get_nearest_free(process_info.lpBaseOfImage as u64, max_code_alloc_size),
                max_code_alloc_size
            )),
            memory_info: memory_info,
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
        let size_guess = (code_range.end - code_range.start) as usize;
        let base = self.allocator.borrow_mut().allocate(size_guess)?;

        // 2. Assemble new code with the new allocation start as a base address
        let assembled_block = asm.assemble(base)?;

        // 3. Bump allocated amount if necessary
        if assembled_block.len() > size_guess {
            self.allocator
                .borrow_mut()
                .allocation_increase(base, assembled_block.len())?
        }

        // 4. Copy assembled code
        self.write_memory(base, &assembled_block)?;

        // 5. Set up trampoline
        println!(
            "For function at {:#x} (len {}) written recompiled version at {:#x} (len {})",
            code_range.start,
            size_guess,
            base,
            assembled_block.len()
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
    a.mov(ebx, 0xdeadf00d as u32)?; // 5B
    a.int3()?; // 1B
    a.int3()?; // 1B
    a.int3()?; // 1B
               // 20B total

    Ok(a.assemble(base_addr)?)
}
