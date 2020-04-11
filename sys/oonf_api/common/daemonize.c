
/*
 * The olsr.org Optimized Link-State Routing daemon version 2 (olsrd2)
 * Copyright (c) 2004-2013, the olsr.org team - see HISTORY file
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "common/common_types.h"
#include "common/daemonize.h"

#if !defined(WIN32) && (!defined(WIN32))
/**
 * Prepare the start of a daemon. Fork into background,
 * but keep stdin/out/err open and a pipe connected to
 * the parent. Parent will wait for the exit_code
 * send by the child.
 * @return filedescriptor of pipe, -1 if an error happened
 */
int
daemonize_prepare(void) {
  int fork_pipe[2];
  int ret;
  int exit_code = -1;

  if (pipe(fork_pipe)) {
    /* error during pipe creation */
    return -1;
  }

  ret = fork();
  if (ret == -1) {
    /* first fork() failed */
    return -1;
  }

  if (ret != 0) {
    /* parent of first fork() */

    /* wait for exit_code from daemonized part */
    if (read(fork_pipe[0], &exit_code, sizeof(exit_code)) != sizeof(exit_code)) {
      exit_code = -1;
    }
    close (fork_pipe[1]);
    close (fork_pipe[0]);
    exit(exit_code);
  }

  /* child of first fork() */

  ret = fork();
  if (ret == -1) {
    /* second fork() failed */
    exit_code = -1;
    if (write(fork_pipe[1], &exit_code, sizeof(exit_code)) != sizeof(exit_code)) {
      /* there is nothing we can do here */
    }
    exit(0);
  }

  if (ret != 0) {
    /* parent of second fork(), exit to detach child */
    exit(0);
  }

  /* child is up, but keep pipe alive to transmit exit code later */
  return fork_pipe[1];
}

/**
 * Finalize the fork of the daemon, call the parent and
 * tell it to exit.
 * @param pipe_fd returned file desriptor of daemonize_prepare
 * @param exit_code exit code for parent
 * @param pidfile filename to store the new process id into, NULL to not
 *   store the PID
 * @return -1 if pidfile couldn't be written, 0 otherwise
 */
int
daemonize_finish(int pipe_fd, int exit_code, const char *pidfile) {
  int result = 0;

  if (pipe_fd == 0) {
    /* ignore call if pipe not set */
    return 0;
  }

  if (exit_code == 0 && pidfile != NULL && *pidfile != 0) {
    pid_t pidt = getpid();
    FILE *pidf = fopen(pidfile, "w");

    if (pidf != NULL) {
      fprintf(pidf, "%d\n", pidt);
      fclose(pidf);
    }
    else {
      exit_code = 1;
      result = -1;
    }
  }

  /* tell parent to shut down with defined exit_code */
  if (write(pipe_fd, &exit_code, sizeof(exit_code)) != sizeof(exit_code)) {
    /* there is nothing we can do here */
  }

  close (pipe_fd);

  /* shut down stdin/out/err */
  close (STDIN_FILENO);
  close (STDOUT_FILENO);
  close (STDERR_FILENO);

  return result;
}

#endif
