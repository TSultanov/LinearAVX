#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/wait.h>
#include <unistd.h>
#include <spawn.h>
#include <injector.h>

extern char **environ;

int main(int argc, char** argv){
    injector_t *injector;
    void *handle;

    if (argc < 2) {
        printf("Usage: %s <executable> [args]\n", argv[0]);
        return EINVAL;
    }

    char* app = argv[1];
    char** args = NULL;
    if (argc == 3) {
        args = &argv[2];
    }

    pid_t pid;
    int status = posix_spawn(&pid, app, NULL, NULL, args, environ);
    if (status == 0) {
        // Stop the child process immediately
        kill(pid, SIGSTOP);
        
        // attach inejctor
        if (injector_attach(&injector, pid) != 0) {
            printf("Failed to attach to process %d: %s\n", pid, injector_error());
            return errno;
        }
        // inject libavxhandler.dylib
        if (injector_inject(injector, "libavxhandler.dylib", NULL)!= 0) {
            printf("Failed to inject library: %s\n", injector_error());
            return errno;
        }

        printf("Spawned %s with pid %d\n", app, pid);

        // Wait for child exit
        waitpid(pid, &status, 0);
        if (WIFEXITED(status)) {
            printf("Child exited with status %d\n", WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
            printf("Child killed by signal %d\n", WTERMSIG(status));
        } else if (WIFSTOPPED(status)) {
            printf("Child stopped by signal %d\n", WSTOPSIG(status));
        } else {
            printf("Child exited with unknown status %d\n", status);
        }
    } else {
        printf("Failed to spawn %s: %s\n", app, strerror(status));
        return status;
    }
}