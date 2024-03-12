use std::{
    mem::MaybeUninit,
    ptr::{addr_of, addr_of_mut, null, null_mut},
};

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

pub fn debug_loop() {
    let mut dw_continue_status = DBG_CONTINUE;

    loop {
        let mut debug_ev_uninit = MaybeUninit::zeroed();
        unsafe {
            WaitForDebugEvent(debug_ev_uninit.as_mut_ptr(), INFINITE)
                .expect("Failed to wait for debug event");
        }

        let debug_ev = unsafe { debug_ev_uninit.assume_init() };

        let h_process = unsafe { OpenProcess(PROCESS_ALL_ACCESS, true, debug_ev.dwProcessId).unwrap() };

        match debug_ev.dwDebugEventCode {
            EXCEPTION_DEBUG_EVENT => {
                let exception_record = unsafe { debug_ev.u.Exception.ExceptionRecord };
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
                    DBG_EXCEPTION_NOT_HANDLED => {
                        println!("Illegal instruction");
                    }
                    _ => {
                        println!("Unknown exception {:#x}", exception_code.0);
                    }
                }
                dw_continue_status = DBG_EXCEPTION_NOT_HANDLED;
            }
            CREATE_PROCESS_DEBUG_EVENT => {
                let create_process_info = unsafe { debug_ev.u.CreateProcessInfo };

                let image_name = if create_process_info.hFile.is_invalid() {
                    String::from("(unknown)")
                } else {
                    let buf_length = 4096;
                    let mut file_name_buf = Vec::<u16>::with_capacity(buf_length);
                    unsafe {
                        file_name_buf
                            .set_len(buf_length.try_into().expect("Failed to convert length"));
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

                println!("Create process {} {}", pid, image_name);
            }
            CREATE_THREAD_DEBUG_EVENT => {
                println!("Thread created");
            }
            EXIT_THREAD_DEBUG_EVENT => {
                println!("Thread exited");
            }
            EXIT_PROCESS_DEBUG_EVENT => {
                let exit_event = unsafe { debug_ev.u.ExitProcess };
                println!("Process exited with code {}", exit_event.dwExitCode);
                // break;
            }
            LOAD_DLL_DEBUG_EVENT => {
                let load_dll_info = unsafe { debug_ev.u.LoadDll };

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
            UNLOAD_DLL_DEBUG_EVENT => {
                println!("Unload DLL debug event");
            }
            OUTPUT_DEBUG_STRING_EVENT => {
                let debug_string_info = unsafe { debug_ev.u.DebugString };

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
            RIP_EVENT => {
                println!("RIP event");
            }
            _ => {
                println!("Unknown event {}", debug_ev.dwDebugEventCode.0);
            }
        }

        unsafe {CloseHandle(h_process).expect("Faield to close process");}
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
