use std::{collections::HashMap, ffi::c_void, mem::{size_of, MaybeUninit}, ptr::addr_of_mut};

use windows::{
    core::*,
    Win32::{
        Foundation::*,
        System::{
            Diagnostics::Debug::{
                GetThreadContext, ReadProcessMemory, WriteProcessMemory, CONTEXT, CREATE_PROCESS_DEBUG_INFO, CREATE_THREAD_DEBUG_INFO
            },
            Threading::{
                GetProcessId, GetProcessInformation, GetThreadId, ProcessMachineTypeInfo, SuspendThread, PROCESS_INFORMATION_CLASS, PROCESS_MACHINE_INFORMATION
            },
        },
    },
};

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

    pub fn get_context(&self) -> Result<CONTEXT> {
        let context = unsafe {
            let mut context = MaybeUninit::<CONTEXT>::zeroed();
            GetThreadContext(self.h_thread, context.as_mut_ptr())?;
            context.assume_init()
        };
        Ok(context)
    }
}

pub struct Process {
    pub pid: u32,
    pub base_address: u64,
    pub entry_point: u64,
    pub machine_info: PROCESS_MACHINE_INFORMATION,
    h_process: HANDLE,
    threads: HashMap<u32, Thread>,
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
                size_of::<PROCESS_MACHINE_INFORMATION>() as u32
            ).expect("Failed to fetch machine information for process");
            mi.assume_init()
        };

        Process {
            pid: pid,
            base_address: process_info.lpBaseOfImage as u64,
            entry_point: process_info.lpStartAddress.unwrap() as u64,
            h_process: process_info.hProcess,
            threads: threads,
            machine_info: mi,
        }
    }

    pub fn add_thread(&mut self, thread_info: &CREATE_THREAD_DEBUG_INFO) {
        let thread = Thread::new(thread_info);
        self.threads.insert(thread.tid, thread);
    }

    pub fn remove_thread(&mut self, tid: u32) {
        self.threads.remove(&tid);
    }

    pub fn read_memory(&self, addr: u64, size: usize) -> Result<Vec<u8>> {
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

    pub fn write_memory(&self, addr: u64, data: &[u8]) -> Result<()> {
        unsafe {
            WriteProcessMemory(
                self.h_process,
                addr as *const c_void,
                data.as_ptr().cast(),
                data.len(),
                None,
            )?;
        }
        Ok(())
    }

    pub fn get_thread(&mut self, tid: u32) -> Option<&mut Thread> {
        self.threads.get_mut(&tid)
    }

    pub fn alloc_tls(&mut self, tid: u32) {
        // Suspend all threads
        for (_, thread) in &self.threads {
            thread.suspend();
        }

        if let Some(thread) = self.get_thread(tid) {
            // Save thread context
            let context = thread.get_context().unwrap();
            println!("{}", context.Rip);
            // Set thread context to run a function which allocates TLS
            // Resume thread
            // Handle breakpoint
            // Suspend thread
            // Restore thread context
            // Resume all threads
        }
    }
}
