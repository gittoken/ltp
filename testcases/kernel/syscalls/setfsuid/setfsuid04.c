/******************************************************************************/
/* Copyright (c) Kerlabs 2008.                                                */
/* Copyright (c) International Business Machines  Corp., 2008                 */
/*                                                                            */
/* This program is free software;  you can redistribute it and/or modify      */
/* it under the terms of the GNU General Public License as published by       */
/* the Free Software Foundation; either version 2 of the License, or          */
/* (at your option) any later version.                                        */
/*                                                                            */
/* This program is distributed in the hope that it will be useful,            */
/* but WITHOUT ANY WARRANTY;  without even the implied warranty of            */
/* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See                  */
/* the GNU General Public License for more details.                           */
/*                                                                            */
/* You should have received a copy of the GNU General Public License          */
/* along with this program;  if not, write to the Free Software               */
/* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA    */
/*                                                                            */
/******************************************************************************/
/*
 * NAME
 * 	setfsuid04.c
 *
 * DESCRIPTION
 * 	Check if setfsuid behaves correctly with file permissions.
 *      The test creates a file as ROOT with permissions 0644, does a setfsuid
 *      and then tries to open the file with RDWR permissions.
 *      The same test is done in a fork to check if new UIDs are correctly
 *      passed to the son.
 *
 * USAGE:  <for command-line>
 *  setfsuid04 [-c n] [-e] [-i n] [-I x] [-P x] [-t]
 *     where,  -c n : Run n copies concurrently.
 *             -e   : Turn on errno logging.
 *             -i n : Execute test n times.
 *             -I x : Execute test for x seconds.
 *             -P x : Pause for x seconds between iterations.
 *             -t   : Turn on syscall timing.
 *
 * HISTORY
 *	07/2001 Created by Renaud Lottiaux
 *
 * RESTRICTIONS
 * 	Must be run as root.
 */
#include <sys/types.h>
#ifdef __GLIBC__
#include <sys/fsuid.h>
#endif
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "test.h"
#include "usctest.h"

char *TCID = "setfsuid04";
int TST_TOTAL = 1;
char nobody_uid[] = "nobody";
char testfile[256] = "";
struct passwd *ltpuser;

int exp_enos[] = { EACCES, 0 };

int fd = -1;

void setup(void);
void cleanup(void);
void do_master_child();

int main(int ac, char **av)
{
	pid_t pid;
	char *msg;
	int status;

	if ((msg = parse_opts(ac, av, NULL, NULL)) != NULL)
		tst_brkm(TBROK, NULL, "OPTION PARSING ERROR - %s", msg);

	setup();

	TEST_EXP_ENOS(exp_enos);

	pid = FORK_OR_VFORK();
	if (pid < 0)
		tst_brkm(TBROK, cleanup, "Fork failed");

	if (pid == 0)
		do_master_child();

	if (waitpid(pid, &status, 0) == -1)
		tst_resm(TBROK | TERRNO, "waitpid failed");
	if (!WIFEXITED(status) || (WEXITSTATUS(status) != 0))
		tst_resm(TFAIL, "child process terminated abnormally");

	cleanup();
	tst_exit();
}

/*
 * do_master_child()
 */
void do_master_child()
{
	int pid;
	int status;
	int tst_fd;

	if (setfsuid(ltpuser->pw_uid) == -1) {
		perror("setfsuid failed");
		exit(1);
	}

	/* Test 1: Check the process with new uid cannot open the file
	 *         with RDWR permissions.
	 */
	tst_fd = open(testfile, O_RDWR);

	if (tst_fd != -1) {
		printf("open succeeded unexpectedly");
		close(tst_fd);
		exit(1);
	}

	if (errno == EACCES)
		printf("open failed with EACCES as expected");
	else {
		perror("open failed unexpectedly");
		exit(1);
	}

	/* Test 2: Check a son process cannot open the file
	 *         with RDWR permissions.
	 */
	pid = FORK_OR_VFORK();
	if (pid < 0)
		tst_brkm(TBROK, cleanup, "Fork failed");

	if (pid == 0) {
		int tst_fd2;

		/* Test to open the file in son process */
		tst_fd2 = open(testfile, O_RDWR);

		if (tst_fd2 != -1) {
			printf("call succeeded unexpectedly\n");
			close(tst_fd2);
			exit(1);
		}

		if (errno == EACCES)
			printf("open failed with EACCES as expected\n");
		else {
			printf("open failed unexpectedly\n");
			exit(1);
		}
	} else {
		/* Wait for son completion */
		if (waitpid(pid, &status, 0) == -1) {
			perror("waitpid failed");
			exit(1);
		}
		if (!WIFEXITED(status) || WEXITSTATUS(status) != 0)
			exit(WEXITSTATUS(status));
	}

	/* Test 3: Fallback to initial uid and check we can again open
	 *         the file with RDWR permissions.
	 */
	tst_count++;
	if (setfsuid(0) == -1) {
		perror("setfsuid failed");
		exit(1);
	}

	tst_fd = open(testfile, O_RDWR);

	if (tst_fd == -1) {
		perror("open failed unexpectedly");
		exit(1);
	} else {
		printf("open call succeeded");
		close(tst_fd);
	}
	exit(0);
}

/*
 * setup() - performs all ONE TIME setup for this test
 */
void setup(void)
{
	tst_require_root(NULL);

	ltpuser = getpwnam(nobody_uid);

	tst_tmpdir();

	sprintf(testfile, "setfsuid04file%d.tst", getpid());

	/* Create test file */
	fd = open(testfile, O_CREAT | O_RDWR, 0644);
	if (fd < 0)
		tst_brkm(TBROK, cleanup, "cannot creat test file");

	tst_sig(FORK, DEF_HANDLER, cleanup);

	TEST_PAUSE;
}

/*
 * cleanup() - performs all the ONE TIME cleanup for this test at completion
 * 	       or premature exit
 */
void cleanup(void)
{
	close(fd);

	/*
	 * print timing status if that option was specified
	 * print errno log if that option was specified
	 */
	TEST_CLEANUP;

	tst_rmdir();

}
