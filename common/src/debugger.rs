use std::{collections::HashSet, ffi::c_void, mem::MaybeUninit, ptr::addr_of_mut};

use windows::{
    core::*,
    Win32::{
        Foundation::*,
        Storage::FileSystem::{GetFinalPathNameByHandleW, FILE_NAME_NORMALIZED},
        System::{
            Diagnostics::Debug::*,
            Threading::{GetProcessId, OpenProcess, INFINITE, PROCESS_ALL_ACCESS},
        },
    },
};

pub struct ProcessCreatedData {
    pub base_address: u64,
    pub entry_point: u64,
    pub read_memory: Box<dyn Fn(u64, usize) -> Result<Vec<u8>>>,
    pub write_memory: Box<dyn Fn(u64, &[u8]) -> Result<()>>,
}

pub struct DebuggerCore {
    active_pids: HashSet<u32>,
    process_created_handler: Box<dyn Fn(ProcessCreatedData) -> core::result::Result<(), Box<dyn std::error::Error>>>,
}

impl DebuggerCore {
    pub fn new(
        process_created_handler: Box<dyn Fn(ProcessCreatedData) -> core::result::Result<(), Box<dyn std::error::Error>>>,
    ) -> DebuggerCore {
        DebuggerCore {
            active_pids: HashSet::new(),
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

    fn read_memory(hprocess: HANDLE, address: u64, size: usize) -> Result<Vec<u8>> {
        let mut memory: Vec<u8> = Vec::with_capacity(size);
        unsafe {
            let mut bytes_read = 0;
            ReadProcessMemory(
                hprocess,
                address as *const c_void,
                memory.as_mut_ptr().cast(),
                size,
                Some(addr_of_mut!(bytes_read)),
            )?;
            memory.set_len(bytes_read);
        }
        Ok(memory)
    }

    fn write_memory(hproccess: HANDLE, address: u64, data: &[u8]) -> Result<()> {
        unsafe {
            WriteProcessMemory(
                hproccess,
                address as *const c_void,
                data.as_ptr().cast(),
                data.len(),
                None,
            )?;
        }
        Ok(())
    }

    fn handle_create_process(&mut self, create_process_info: CREATE_PROCESS_DEBUG_INFO) -> bool {
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

        let pid = if create_process_info.hProcess.is_invalid() {
            0
        } else {
            unsafe { GetProcessId(create_process_info.hProcess) }
        };

        if pid != 0 {
            self.active_pids.insert(pid);
        }

        println!("Created process {} {}", pid, image_name);
        if let Some(start_address) = create_process_info.lpStartAddress {
            let start_address = start_address as usize;
            let base_address = create_process_info.lpBaseOfImage as usize;
            let hprocess = create_process_info.hProcess;

            let data = ProcessCreatedData {
                base_address: base_address
                    .try_into()
                    .expect("Cannot cast start_address into u64"),
                entry_point: start_address
                    .try_into()
                    .expect("Cannot cast start_address into u64"),
                read_memory: Box::new(move |address, size| {
                    DebuggerCore::read_memory(hprocess, address, size)
                }),
                write_memory: Box::new(move |address, data| {
                    DebuggerCore::write_memory(hprocess, address, data)
                }),
            };

            (self.process_created_handler)(data).expect("Failed to execute callback");
        } else {
            println!("No start address for process, can't proceed");
            return true;
        }
        false
    }

    fn handle_exception(
        &self,
        debug_ev: DEBUG_EVENT,
        exception_info: EXCEPTION_DEBUG_INFO,
    ) -> NTSTATUS {
        let exception_record = exception_info.ExceptionRecord;
        let exception_code = exception_record.ExceptionCode;

        match exception_code {
            EXCEPTION_ACCESS_VIOLATION => {
                println!("Access violation");
            }
            EXCEPTION_BREAKPOINT => {
                println!("Breakpoint");
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

            let h_process =
                unsafe { OpenProcess(PROCESS_ALL_ACCESS, true, debug_ev.dwProcessId).unwrap() };

            match debug_ev.dwDebugEventCode {
                EXCEPTION_DEBUG_EVENT => {
                    let exception_info = unsafe { debug_ev.u.Exception };

                    dw_continue_status = self.handle_exception(debug_ev, exception_info);
                }
                CREATE_PROCESS_DEBUG_EVENT => unsafe {
                    if self.handle_create_process(debug_ev.u.CreateProcessInfo) {
                        break;
                    }
                },
                // CREATE_THREAD_DEBUG_EVENT => {
                //     println!("Thread created");
                // }
                // EXIT_THREAD_DEBUG_EVENT => {
                //     println!("Thread exited");
                // }
                EXIT_PROCESS_DEBUG_EVENT => {
                    let exit_event = unsafe { debug_ev.u.ExitProcess };
                    println!(
                        "Process {} exited with code {:#x}",
                        debug_ev.dwProcessId, exit_event.dwExitCode
                    );
                    self.active_pids.remove(&debug_ev.dwProcessId);
                    if self.active_pids.is_empty() {
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
