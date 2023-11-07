#define _GNU_SOURCE

#include "pipe.h"
#include <sys/wait.h> /* For waitpid */
#include <unistd.h> /* For fork, pipe */
#include <stdlib.h> /* For exit */
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>

extern int errno;

int run_program(char *argv[]) // ls, cd, ...
{
    if (argv == NULL) {
        return -1;
    }

    // -------------------------
    // Open a pipe
    // -------------------------

    int flags = 0 | O_CLOEXEC; // set a flag to close after exe
    int pipefd[2];
    int pipe_status = pipe2(pipefd, flags);
    // pipe2() -> On success, zero is returned.  On error, -1 is returned, errno is set to indicate the error, and pipefd is left unchanged
    if (pipe_status != 0) {
        return -1;
    }

    int exe_err_msg  = 0;

    int child_pid = fork();
    if (child_pid == -1) {
        return -1;
    } else if (child_pid == 0) { //if sucessful (inside the childpid), execute the program
        // close the "read" end of the pipe; parent to read--child to write
        close(pipefd[0]); 
        // Replace program
        int exe_var = execvp(argv[0], argv);  //first arg is the file name, the second arg is the array vector with the arg passed to the program (e.g.: cd, ls...)
        if (exe_var <0) {   // is exe_var == -1?
            exe_err_msg = errno;   // err nr from execvp

            // -------------------------
            // Write the error on the pipe
            // -------------------------

            //write the error to the pipe
            write(pipefd[1], &exe_err_msg, sizeof(int)); 
            // close the "write" end of the pipe; parent to read--child to write
            close(pipefd[1]); 
            exit(-1);
        }
         // close the "write" end of the pipe; parent to read--child to write
        close(pipefd[1]);
        exit(0);
    } else { 
        // close the "write" end of the pipe; parent to read--child to write
        close(pipefd[1]); 
        int status, hadError = 0;

        int waitError = waitpid(child_pid, &status, 0);
        if (waitError == -1) {
            // Error while waiting for child.
            hadError = 1; // set it to true
        } else if (!WIFEXITED(status)) {
            // Our child exited with another problem (e.g., a segmentation fault)
            // We use the error code ECANCELED to signal this.
            hadError = 1;
            errno = ECANCELED; // 125
        } else {

            // -------------------------
            // Set errno to the value execvp set it to ( if execvp error in the child)
            // -------------------------
            int read_value;
            (void)read_value;
            int read_status = read(pipefd[0], &read_value, sizeof(int));
            // if it reads from the pipe...
            if (read_status > 0) { 
                errno = read_value;
                hadError = 1; // set it to true
            }
        }
        // close the "read" end of the pipe; parent to read--child to write
        close(pipefd[0]); 
        return hadError ? -1 : WEXITSTATUS(status);
    }
}
