use std::{env, mem::MaybeUninit};
use windows::{
    core::{HSTRING, PWSTR},
    Win32::{
        Foundation::CloseHandle,
        System::Threading::{CreateProcessW, WaitForSingleObject, INFINITE, PROCESS_CREATION_FLAGS, STARTUPINFOW},
    },
};

fn main() {
    let mut args = env::args();
    args.next();

    let executable = args.next().expect("First argument should be present");
    let executable_pwstr = HSTRING::from(&executable);
    // let mut command_line_utf16 = "".encode_utf16().collect::<Vec<_>>();
    // let command_line_pwstr = PWSTR::from_raw(command_line_utf16.as_mut_ptr());

    unsafe {
        let mut pi = MaybeUninit::zeroed();
        let mut si: MaybeUninit<STARTUPINFOW> = MaybeUninit::zeroed();
        (*si.as_mut_ptr()).cb = std::mem::size_of::<STARTUPINFOW>()
            .try_into()
            .expect("Size cannot overflow");

        let pi_p = pi.as_mut_ptr();

        match CreateProcessW(
            &executable_pwstr,
            PWSTR::null(),
            None,
            None,
            false,
            /*CREATE_SUSPENDED | DEBUG_PROCESS*/ PROCESS_CREATION_FLAGS(0),
            None,
            None,
            si.as_mut_ptr(),
            pi_p,
        ) {
            Ok(_) => {}
            Err(_) => panic!("Failed to spawn process"),
        }

        let pi_init = pi.assume_init();
        let _ = si.assume_init();

        WaitForSingleObject(pi_init.hProcess, INFINITE);

        let _ = CloseHandle(pi_init.hProcess);
        let _ = CloseHandle(pi_init.hThread);
    }
}
