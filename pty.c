#include "apue.h"

static void set_noecho(int); /* at the end of this file */
void do_driver(char *);      /* in the file driver.c */
void loop(int, int);         /* in the file loop.c */

void reap(int signum){
    wait(NULL);
}
#define MAXLEN 100
pid_t forker(int *ptrfdm, char *slave_name, char *argv[]);

int main(int argc, char *argv[]) {
    int fdm;

    if(argv[1][0] == '@'){
        //reaper
        signal(SIGCHLD, reap); 

        unlink(argv[2]);
        mkfifo(argv[2], 0666); 

        int fd = open(argv[2], O_RDWR);
        FILE* fp = fdopen(fd, "rw");
        if (fp == NULL) {
            perror("Failed: ");
            return 1;
        }

        char buffer[MAXLEN];
        while(1){
            if (fgets(buffer, MAXLEN - 1, fp)){
                printf("PTY:  %s", buffer);
                buffer[strcspn(buffer, "\n")] = 0;

                pid_t pid = forker(&fdm, buffer, argv+1);
                if( pid == 0){
                    exit(0);
                }
            }
        }

    }else{
        char *slave_name = argv[1];

        pid_t pid = forker(&fdm, slave_name, argv);
        if( pid > 0){
            waitpid(pid, NULL, 0);
        }
    }
}

int
ptys_open (char *pts_name)
{
  int fds;
  //TODO restore solaris support
  if ((fds = open (pts_name, O_RDWR)) < 0)
    return (-1);

  return (fds);
}


static void set_noecho(int fd) /* turn off echo (for slave pty) */
{
    struct termios stermios;

    if (tcgetattr(fd, &stermios) < 0)
        err_sys("tcgetattr error");

    stermios.c_lflag &= ~(ECHO | ECHOE | ECHOK | ECHONL);

    /*
     * Also turn off NL to CR/NL mapping on output.
     */
    stermios.c_oflag &= ~(ONLCR);

    if (tcsetattr(fd, TCSANOW, &stermios) < 0)
        err_sys("tcsetattr error");
}

pid_t forker(int *ptrfdm, char *slave_name, char *argv[]){
    pid_t pid = fork();
    if( ! pid ){
        
        int fds;

        if (setsid() < 0)
            err_sys("setsid error");

        /*
         * System V acquires controlling terminal on open().
         */
        if ((fds = ptys_open(slave_name)) < 0)
            err_sys("can't open slave pty");

        //TODO restore bsd support

        /*
         * Slave becomes stdin/stdout/stderr of child.
         */
        if (dup2(fds, STDIN_FILENO) != STDIN_FILENO)
            err_sys("dup2 error to stdin");
        if (dup2(fds, STDOUT_FILENO) != STDOUT_FILENO)
            err_sys("dup2 error to stdout");
        if (dup2(fds, STDERR_FILENO) != STDERR_FILENO)
            err_sys("dup2 error to stderr");
        if (fds != STDIN_FILENO && fds != STDOUT_FILENO && fds != STDERR_FILENO)
            close(fds);

        //set_noecho(STDIN_FILENO); /* stdin is slave pty */

        if (execvp(argv[2], &argv[2]) < 0)
            err_sys("can't execute: %s", argv[2]);
        
        return (0);    /* child returns 0 just like fork() */
    }
    return pid;

}
