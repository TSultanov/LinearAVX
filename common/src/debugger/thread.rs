use windows::Win32::Foundation::HANDLE;
use windows::Win32::System::Diagnostics::Debug::{CONTEXT, CONTEXT_ALL_AMD64, CREATE_THREAD_DEBUG_INFO, GetThreadContext, SetThreadContext};
use windows::Win32::System::Threading::{GetThreadId, ResumeThread, SuspendThread};
use std::error::Error;

pub struct Thread {
    pub tid: u32,
    pub(crate) h_thread: HANDLE,
}

impl Thread {
    pub fn new(thread_info: &CREATE_THREAD_DEBUG_INFO) -> Thread {
        let tid = unsafe { GetThreadId(thread_info.hThread) };

        Thread {
            tid,
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
