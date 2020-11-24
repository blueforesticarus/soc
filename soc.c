#include "apue.h"

void
loop (int ptym, int ignoreeof);
int
ptym_open (char *pts_name, int pts_namesz)
{
  char *ptr;
  int fd, err;

  if ((fd = posix_openpt (O_RDWR)) < 0)
    return (-1);
  if (grantpt (fd) < 0)	/* grant access to slave */
    goto errout;
  if (unlockpt (fd) < 0)	/* clear slave's lock flag */
    goto errout;
  if ((ptr = ptsname (fd)) == NULL)	/* get slave's name */
    goto errout;

  /*
   * Return name of slave.  Null terminate to handle
   * case where strlen(ptr) > pts_namesz.
   */
  strncpy (pts_name, ptr, pts_namesz);
  pts_name[pts_namesz - 1] = '\0';
  return (fd);			/* return fd of master */
errout:
  err = errno;
  close (fd);
  errno = err;
  return (-1);
}

int main(int argc, char *argv[]) {
    int fd, ignoreeof, returncode, status;
    char slave_name[50];
    struct termios orig_termios;
    struct winsize size;

    ignoreeof = 0;
    returncode = 0;

    if (tcgetattr(STDIN_FILENO, &orig_termios) < 0)
        err_sys("tcgetattr error on stdin");
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, (char *)&size) < 0)
        err_sys("TIOCGWINSZ error");
    

    fd = ptym_open (slave_name, sizeof (slave_name));
    if(fd < 0){
        err_sys ("can't open master pty: %s, error %d", slave_name, fd);
    }
    slave_name[sizeof(slave_name) - 1] = '\0';//just in case

    FILE* fp = fopen(argv[1], "w");
    fprintf(fp, "%s\n", slave_name);
    fclose(fp);

    fprintf(stderr, "slave name = %s\n", slave_name);
    fflush(stderr);
    
    struct pollfd tmp = {fd, POLLIN, 0};
    poll(&tmp, 1, -1);//wait for pty slave process

    if (tty_raw(STDIN_FILENO) < 0) /* user's tty to raw mode */
        err_sys("tty_raw error");
    if (atexit(tty_atexit) < 0) /* reset user's tty on exit */
        err_sys("atexit error");

    loop(fd, ignoreeof); /* copies stdin -> ptym, ptym -> stdout */

    if (returncode) {
        if (WIFEXITED(status))
            exit(WEXITSTATUS(status));
        else
            exit(EXIT_FAILURE);
    }
}

