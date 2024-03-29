use std::{collections::HashMap, mem::MaybeUninit, ptr::addr_of_mut};
pub mod process;

use im::HashSet;
use windows::{
    core::*,
    Win32::{
        Foundation::*,
        Storage::FileSystem::{GetFinalPathNameByHandleW, FILE_NAME_NORMALIZED},
        System::{
            Diagnostics::Debug::*,
            SystemInformation::IMAGE_FILE_MACHINE_AMD64,
            Threading::{GetProcessId, OpenProcess, INFINITE, PROCESS_ALL_ACCESS},
        },
    },
};

use self::process::{AlignedContext, Process};

pub struct DebuggerCore {
    active_processes: HashMap<u32, Process>,
    tls_allocation_awaiting: HashMap<u32, AlignedContext>,
    tls_allocated_pids: HashSet<u32>,
    process_created_handler:
        Box<dyn Fn(&Process) -> core::result::Result<(), Box<dyn std::error::Error>>>,
}

impl DebuggerCore {
    pub fn new(
        process_created_handler: Box<
            dyn Fn(&Process) -> core::result::Result<(), Box<dyn std::error::Error>>,
        >,
    ) -> DebuggerCore {
        DebuggerCore {
            active_processes: HashMap::new(),
            tls_allocation_awaiting: HashMap::new(),
            tls_allocated_pids: HashSet::new(),
            process_created_handler,
        }
    }

    fn handle_dll_load_event(&self, load_dll_info: LOAD_DLL_DEBUG_INFO) {
        let buf_length = 4096;
        let mut file_name_buf = Vec::<u16>::with_capacity(buf_length);
        unsafe {
            file_name_buf.set_len(buf_length.try_into().expect("Failed to convert length"));
            let length = GetFinalPathNameByHandleW(
                load_dll_info.hFile,
                file_name_buf.as_mut_slice(),
                FILE_NAME_NORMALIZED,
            );
            file_name_buf.set_len(length.try_into().expect("Failed to convert length"));
            CloseHandle(load_dll_info.hFile).expect("Failed to close handle");
        };

        let image_name_pwstr = PWSTR::from_raw(file_name_buf.as_mut_ptr());
        let image_name = unsafe {
            image_name_pwstr
                .to_string()
                .expect("(Failed to decode utf16)")
        };
        println!("Load DLL {}", image_name);
    }

    fn handle_create_thread(
        &mut self,
        pid: u32,
        tid: u32,
        create_thread_info: CREATE_THREAD_DEBUG_INFO,
    ) {
        let process = self
            .active_processes
            .get_mut(&pid)
            .expect("Unexpected CREATE_THREAD event from unknown process");
        process.add_thread(&create_thread_info);
    }

    fn handle_thread_exit(&mut self, pid: u32, tid: u32) {
        let process = self
            .active_processes
            .get_mut(&pid)
            .expect("Unexpected CREATE_THREAD event from unknown process");
        process.remove_thread(tid);
    }

    fn handle_create_process(
        &mut self,
        pid: u32,
        tid: u32,
        create_process_info: CREATE_PROCESS_DEBUG_INFO,
    ) -> bool {
        let image_name = if create_process_info.hFile.is_invalid() {
            String::from("(unknown)")
        } else {
            let buf_length = 4096;
            let mut file_name_buf = Vec::<u16>::with_capacity(buf_length);
            unsafe {
                file_name_buf.set_len(buf_length.try_into().expect("Failed to convert length"));
                let length = GetFinalPathNameByHandleW(
                    create_process_info.hFile,
                    file_name_buf.as_mut_slice(),
                    FILE_NAME_NORMALIZED,
                );
                file_name_buf.set_len(length.try_into().expect("Failed to convert length"));
                CloseHandle(create_process_info.hFile).expect("Failed to close handle");
            };

            let image_name_pwstr = PWSTR::from_raw(file_name_buf.as_mut_ptr());
            unsafe {
                image_name_pwstr
                    .to_string()
                    .expect("(Failed to decode utf16)")
            }
        };

        if !create_process_info.hProcess.is_invalid() {
            let mut process = Process::new(&create_process_info);

            println!(
                "Created process {} {}, entry_point = {:#x}, base_address = {:#x}",
                process.pid, image_name, process.entry_point, process.base_address
            );
            process.breakpoint_set(process.entry_point).unwrap();

            // let context = process.get_thread(tid).unwrap().get_context().unwrap();
            // println!("Start RIP = {:#x}", context.ctx.Rip);
            self.active_processes.insert(process.pid, process);
        }
        false
    }

    fn handle_breakpoint(&mut self, pid: u32, tid: u32, record: EXCEPTION_RECORD) -> NTSTATUS {
        let addr = record.ExceptionAddress as u64;
        let process = self.active_processes.get_mut(&pid).unwrap();
        let thread = process.get_thread(tid).unwrap();
        if let Some(context) = self.tls_allocation_awaiting.get(&tid) {
            let old_context = thread.get_context().unwrap();
            let _ = thread.set_context(context);

            let tls_index = old_context.ctx.Rax;
            let canary = old_context.ctx.Rbx;
            if canary != 0xdeadf00d {
                // TODO randomize canary
                panic!("TLS allcoation failed: mismatched canary");
            }
            println!("TLS allocated at index {}", tls_index);
            let process = self.active_processes.get_mut(&pid).unwrap();
            process.set_tls_offset(tls_index);

            (self.process_created_handler)(process)
                .expect("Failed to execute callback");

            DBG_CONTINUE
        } else {
            if addr == process.entry_point {
                process.breakpoint_remove(process.entry_point).unwrap();
                println!("Set TlsAlloc execution");
                let context = process.set_alloc_tls_execution(tid).unwrap();
                self.tls_allocation_awaiting.insert(tid, context);
                DBG_CONTINUE
            } else {
                println!("Unknown breakpoint at {:#x}", addr as u64);
                DBG_EXCEPTION_NOT_HANDLED
            }
        }
    }

    fn handle_exception(
        &mut self,
        debug_ev: DEBUG_EVENT,
        exception_info: EXCEPTION_DEBUG_INFO,
    ) -> NTSTATUS {
        let exception_record = exception_info.ExceptionRecord;
        let exception_code = exception_record.ExceptionCode;
        let pid = debug_ev.dwProcessId;
        let tid = debug_ev.dwThreadId;

        match exception_code {
            EXCEPTION_ACCESS_VIOLATION => {
                println!("Access violation");
            }
            EXCEPTION_BREAKPOINT => {
                return self.handle_breakpoint(pid, tid, exception_record);
            }
            EXCEPTION_DATATYPE_MISALIGNMENT => {
                println!("Datatype misalignment");
            }
            EXCEPTION_SINGLE_STEP => {
                println!("Single step");
            }
            DBG_CONTROL_C => {
                println!("Control C");
            }
            EXCEPTION_ILLEGAL_INSTRUCTION => {
                println!(
                    "Illegal instruction in PID {} TID {}",
                    debug_ev.dwProcessId, debug_ev.dwThreadId
                );
            }
            _ => {
                println!("Unknown exception {:#x}", exception_code.0);
            }
        }

        DBG_EXCEPTION_NOT_HANDLED
    }

    fn handle_debug_string(&self, h_process: HANDLE, debug_string_info: OUTPUT_DEBUG_STRING_INFO) {
        let s = if debug_string_info.fUnicode != 0 {
            let mut buf: Vec<u16> =
                Vec::with_capacity((debug_string_info.nDebugStringLength + 1).into());

            let mut bytes_read = 0;
            unsafe {
                let _ = ReadProcessMemory(
                    h_process,
                    debug_string_info.lpDebugStringData.as_ptr().cast(),
                    buf.as_mut_ptr().cast(),
                    debug_string_info.nDebugStringLength.into(),
                    Some(addr_of_mut!(bytes_read)),
                );
            }
            if let Some(v) = buf.get_mut(bytes_read) {
                *v = 0;
            }

            let pwstr = PWSTR::from_raw(buf.as_mut_ptr());
            unsafe {
                pwstr
                    .to_string()
                    .unwrap_or(String::from("(failed to parse utf16)"))
            }
        } else {
            let mut buf: Vec<u8> =
                Vec::with_capacity((debug_string_info.nDebugStringLength + 1).into());

            let mut bytes_read = 0;
            unsafe {
                let _ = ReadProcessMemory(
                    h_process,
                    debug_string_info.lpDebugStringData.as_ptr().cast(),
                    buf.as_mut_ptr().cast(),
                    debug_string_info.nDebugStringLength.into(),
                    Some(addr_of_mut!(bytes_read)),
                );
            }
            if let Some(v) = buf.get_mut(bytes_read) {
                *v = 0;
            }

            let pstr = PSTR::from_raw(buf.as_mut_ptr());
            unsafe {
                pstr.to_string()
                    .unwrap_or(String::from("(failed to parse utf8)"))
            }
        };

        println!("[debug] {}", s);
    }

    pub fn debug_loop(&mut self) {
        let mut dw_continue_status = DBG_CONTINUE;

        loop {
            let mut debug_ev_uninit = MaybeUninit::zeroed();
            unsafe {
                WaitForDebugEvent(debug_ev_uninit.as_mut_ptr(), INFINITE)
                    .expect("Failed to wait for debug event");
            }

            let debug_ev = unsafe { debug_ev_uninit.assume_init() };

            if let Some(process) = self.active_processes.get(&debug_ev.dwProcessId) {
                if process.machine_info.ProcessMachine != IMAGE_FILE_MACHINE_AMD64 {
                    match debug_ev.dwDebugEventCode {
                        EXIT_PROCESS_DEBUG_EVENT => {
                            let exit_event = unsafe { debug_ev.u.ExitProcess };
                            println!(
                                "Process {} exited with code {:#x}",
                                debug_ev.dwProcessId, exit_event.dwExitCode
                            );
                            self.active_processes.remove(&debug_ev.dwProcessId);
                            if self.active_processes.is_empty() {
                                break;
                            }
                        }
                        _ => {}
                    }

                    unsafe {
                        ContinueDebugEvent(debug_ev.dwProcessId, debug_ev.dwThreadId, DBG_CONTINUE)
                            .expect("Failed to continue debugging");
                    }
                    continue;
                }
            }

            let h_process =
                unsafe { OpenProcess(PROCESS_ALL_ACCESS, true, debug_ev.dwProcessId).unwrap() };

            match debug_ev.dwDebugEventCode {
                EXCEPTION_DEBUG_EVENT => {
                    let exception_info = unsafe { debug_ev.u.Exception };

                    dw_continue_status = self.handle_exception(debug_ev, exception_info);
                }
                CREATE_PROCESS_DEBUG_EVENT => unsafe {
                    if self.handle_create_process(
                        debug_ev.dwProcessId,
                        debug_ev.dwThreadId,
                        debug_ev.u.CreateProcessInfo,
                    ) {
                        break;
                    }
                },
                CREATE_THREAD_DEBUG_EVENT => {
                    let thread_info = unsafe { debug_ev.u.CreateThread };
                    self.handle_create_thread(
                        debug_ev.dwProcessId,
                        debug_ev.dwThreadId,
                        thread_info,
                    );
                }
                EXIT_THREAD_DEBUG_EVENT => {
                    self.handle_thread_exit(debug_ev.dwProcessId, debug_ev.dwThreadId);
                }
                EXIT_PROCESS_DEBUG_EVENT => {
                    let exit_event = unsafe { debug_ev.u.ExitProcess };
                    println!(
                        "Process {} exited with code {:#x}",
                        debug_ev.dwProcessId, exit_event.dwExitCode
                    );
                    self.active_processes.remove(&debug_ev.dwProcessId);
                    if self.active_processes.is_empty() {
                        break;
                    }
                }
                // LOAD_DLL_DEBUG_EVENT => unsafe { self.handle_dll_load_event(debug_ev.u.LoadDll) },
                // UNLOAD_DLL_DEBUG_EVENT => {
                //     println!("Unload DLL debug event");
                // }
                OUTPUT_DEBUG_STRING_EVENT => {
                    let debug_string_info = unsafe { debug_ev.u.DebugString };
                    self.handle_debug_string(h_process, debug_string_info);
                }
                RIP_EVENT => {
                    println!("RIP event");
                }
                _ => {
                    // println!("Unknown event {}", debug_ev.dwDebugEventCode.0);
                }
            }

            unsafe {
                CloseHandle(h_process).expect("Failed to close process");
            }
            unsafe {
                ContinueDebugEvent(
                    debug_ev.dwProcessId,
                    debug_ev.dwThreadId,
                    dw_continue_status,
                )
                .expect("Failed to continue debugging");
            }
        }
    }
}
