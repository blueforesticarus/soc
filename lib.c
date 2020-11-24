//FROM error.c
#include "apue.h"


//TODO  restore driver functionality

static void err_doit (int, int, const char *, va_list);

/*
 * Nonfatal error related to a system call.
 * Print a message and return.
 */
void
err_ret (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  err_doit (1, errno, fmt, ap);
  va_end (ap);
}

/*
 * Fatal error related to a system call.
 * Print a message and terminate.
 */
void
err_sys (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  err_doit (1, errno, fmt, ap);
  va_end (ap);
  exit (1);
}

/*
 * Nonfatal error unrelated to a system call.
 * Error code passed as explict parameter.
 * Print a message and return.
 */
void
err_cont (int error, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  err_doit (1, error, fmt, ap);
  va_end (ap);
}

/*
 * Fatal error unrelated to a system call.
 * Error code passed as explict parameter.
 * Print a message and terminate.
 */
void
err_exit (int error, const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  err_doit (1, error, fmt, ap);
  va_end (ap);
  exit (1);
}

/*
 * Fatal error related to a system call.
 * Print a message, dump core, and terminate.
 */
void
err_dump (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  err_doit (1, errno, fmt, ap);
  va_end (ap);
  abort ();			/* dump core and terminate */
  exit (1);			/* shouldn't get here */
}

/*
 * Nonfatal error unrelated to a system call.
 * Print a message and return.
 */
void
err_msg (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  err_doit (0, 0, fmt, ap);
  va_end (ap);
}

/*
 * Fatal error unrelated to a system call.
 * Print a message and terminate.
 */
void
err_quit (const char *fmt, ...)
{
  va_list ap;

  va_start (ap, fmt);
  err_doit (0, 0, fmt, ap);
  va_end (ap);
  exit (1);
}

/*
 * Print a message and return to caller.
 * Caller specifies "errnoflag".
 */
static void
err_doit (int errnoflag, int error, const char *fmt, va_list ap)
{
  char buf[MAXLINE];

  vsnprintf (buf, MAXLINE - 1, fmt, ap);
  if (errnoflag)
    snprintf (buf + strlen (buf), MAXLINE - strlen (buf) - 1, ": %s",
	      strerror (error));
  strcat (buf, "\n");
  fflush (stdout);		/* in case stdout and stderr are the same */
  fputs (buf, stderr);
  fflush (NULL);		/* flushes all stdio output streams */
}
//FROM signalintr.c
#include "apue.h"

Sigfunc *
signal_intr (int signo, Sigfunc * func)
{
  struct sigaction act, oact;

  act.sa_handler = func;
  sigemptyset (&act.sa_mask);
  act.sa_flags = 0;
#ifdef	SA_INTERRUPT
  act.sa_flags |= SA_INTERRUPT;
#endif
  if (sigaction (signo, &act, &oact) < 0)
    return (SIG_ERR);
  return (oact.sa_handler);
}
//FROM writen.c
#include "apue.h"

ssize_t				/* Write "n" bytes to a descriptor  */
writen (int fd, const void *ptr, size_t n)
{
  size_t nleft;
  ssize_t nwritten;

  nleft = n;
  while (nleft > 0)
    {
      if ((nwritten = write (fd, ptr, nleft)) < 0)
	{
	  if (nleft == n)
	    return (-1);	/* error, return -1 */
	  else
	    break;		/* error, return amount written so far */
	}
      else if (nwritten == 0)
	{
	  break;
	}
      nleft -= nwritten;
      ptr += nwritten;
    }
  return (n - nleft);		/* return >= 0 */
}
//FROM loop.c
#include "apue.h"

#define	BUFFSIZE	512

static void sig_term (int);
static volatile sig_atomic_t sigcaught;	/* set by signal handler */

void
loop (int ptym, int ignoreeof)
{
  pid_t child;
  int nread;
  char buf[BUFFSIZE];

  if ((child = fork ()) < 0)
    {
      err_sys ("fork error");
    }
  else if (child == 0)
    {				/* child copies stdin to ptym */
      for (;;)
	{
	  if ((nread = read (STDIN_FILENO, buf, BUFFSIZE)) < 0)
	    err_sys ("read error from stdin");
	  else if (nread == 0)
	    break;		/* EOF on stdin means we're done */
	  if (writen (ptym, buf, nread) != nread)
	    err_sys ("writen error to master pty");
	}

      /*
       * We always terminate when we encounter an EOF on stdin,
       * but we notify the parent only if ignoreeof is 0.
       */
      if (ignoreeof == 0)
	kill (getppid (), SIGTERM);	/* notify parent */
      exit (0);			/* and terminate; child can't return */
    }

  /*
   * Parent copies ptym to stdout.
   */
  if (signal_intr (SIGTERM, sig_term) == SIG_ERR)
    err_sys ("signal_intr error for SIGTERM");

  for (;;)
    {
      if ((nread = read (ptym, buf, BUFFSIZE)) <= 0)
	break;			/* signal caught, error, or EOF */
      if (writen (STDOUT_FILENO, buf, nread) != nread)
	err_sys ("writen error to stdout");
    }

  /*
   * There are three ways to get here: sig_term() below caught the
   * SIGTERM from the child, we read an EOF on the pty master (which
   * means we have to signal the child to stop), or an error.
   */
  if (sigcaught == 0)		/* tell child if it didn't send us the signal */
    kill (child, SIGTERM);

  /*
   * Parent returns to caller.
   */
}

/*
 * The child sends us SIGTERM when it gets EOF on the pty slave or
 * when read() fails.  We probably interrupted the read() of ptym.
 */
static void
sig_term (int signo)
{
  sigcaught = 1;		/* just set flag and return */
}

//FROM ttymodes.c
#include "apue.h"
#include <termios.h>
#include <errno.h>

static struct termios save_termios;
static int ttysavefd = -1;
static enum
{ RESET, RAW, CBREAK } ttystate = RESET;

int
tty_cbreak (int fd)		/* put terminal into a cbreak mode */
{
  int err;
  struct termios buf;

  if (ttystate != RESET)
    {
      errno = EINVAL;
      return (-1);
    }
  if (tcgetattr (fd, &buf) < 0)
    return (-1);
  save_termios = buf;		/* structure copy */

  /*
   * Echo off, canonical mode off.
   */
  buf.c_lflag &= ~(ECHO | ICANON);

  /*
   * Case B: 1 byte at a time, no timer.
   */
  buf.c_cc[VMIN] = 1;
  buf.c_cc[VTIME] = 0;
  if (tcsetattr (fd, TCSAFLUSH, &buf) < 0)
    return (-1);

  /*
   * Verify that the changes stuck.  tcsetattr can return 0 on
   * partial success.
   */
  if (tcgetattr (fd, &buf) < 0)
    {
      err = errno;
      tcsetattr (fd, TCSAFLUSH, &save_termios);
      errno = err;
      return (-1);
    }
  if ((buf.c_lflag & (ECHO | ICANON)) || buf.c_cc[VMIN] != 1 ||
      buf.c_cc[VTIME] != 0)
    {
      /*
       * Only some of the changes were made.  Restore the
       * original settings.
       */
      tcsetattr (fd, TCSAFLUSH, &save_termios);
      errno = EINVAL;
      return (-1);
    }

  ttystate = CBREAK;
  ttysavefd = fd;
  return (0);
}

int
tty_raw (int fd)		/* put terminal into a raw mode */
{
  int err;
  struct termios buf;

  if (ttystate != RESET)
    {
      errno = EINVAL;
      return (-1);
    }
  if (tcgetattr (fd, &buf) < 0)
    return (-1);
  save_termios = buf;		/* structure copy */

  /*
   * Echo off, canonical mode off, extended input
   * processing off, signal chars off.
   */
  buf.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

  /*
   * No SIGINT on BREAK, CR-to-NL off, input parity
   * check off, don't strip 8th bit on input, output
   * flow control off.
   */
  buf.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

  /*
   * Clear size bits, parity checking off.
   */
  buf.c_cflag &= ~(CSIZE | PARENB);

  /*
   * Set 8 bits/char.
   */
  buf.c_cflag |= CS8;

  /*
   * Output processing off.
   */
  buf.c_oflag &= ~(OPOST);

  /*
   * Case B: 1 byte at a time, no timer.
   */
  buf.c_cc[VMIN] = 1;
  buf.c_cc[VTIME] = 0;
  if (tcsetattr (fd, TCSAFLUSH, &buf) < 0)
    return (-1);

  /*
   * Verify that the changes stuck.  tcsetattr can return 0 on
   * partial success.
   */
  if (tcgetattr (fd, &buf) < 0)
    {
      err = errno;
      tcsetattr (fd, TCSAFLUSH, &save_termios);
      errno = err;
      return (-1);
    }
  if ((buf.c_lflag & (ECHO | ICANON | IEXTEN | ISIG)) ||
      (buf.c_iflag & (BRKINT | ICRNL | INPCK | ISTRIP | IXON)) ||
      (buf.c_cflag & (CSIZE | PARENB | CS8)) != CS8 ||
      (buf.c_oflag & OPOST) || buf.c_cc[VMIN] != 1 || buf.c_cc[VTIME] != 0)
    {
      /*
       * Only some of the changes were made.  Restore the
       * original settings.
       */
      tcsetattr (fd, TCSAFLUSH, &save_termios);
      errno = EINVAL;
      return (-1);
    }

  ttystate = RAW;
  ttysavefd = fd;
  return (0);
}

int
tty_reset (int fd)		/* restore terminal's mode */
{
  if (ttystate == RESET)
    return (0);
  if (tcsetattr (fd, TCSAFLUSH, &save_termios) < 0)
    return (-1);
  ttystate = RESET;
  return (0);
}

void
tty_atexit (void)		/* can be set up by atexit(tty_atexit) */
{
  if (ttysavefd >= 0)
    tty_reset (ttysavefd);
}

struct termios *
tty_termios (void)		/* let caller see original tty state */
{
  return (&save_termios);
}
