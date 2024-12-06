/**
 * Tony Givargis
 * Copyright (C), 2023
 * University of California, Irvine
 *
 * CS 238P - Operating Systems
 * main.c
 */

#include <sys/types.h>
#include <signal.h>
#include "system.h"
#include <stdio.h>

#define SCALE 1024
#define PROC_UPTIME "/proc/uptime"

/**
 * Needs:
 *   signal()
 */

static volatile int done;

static void
_signal_(int signum)
{
	assert( SIGINT == signum );

	done = 1;
}

int is_prefix(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

double
cpu_util(const char *s)
{
	static unsigned sum_, vector_[7];
	unsigned sum, vector[7];
	const char *p;
	double util;
	uint64_t i;

	/*
	  user
	  nice
	  system
	  idle
	  iowait
	  irq
	  softirq
	*/

	if (!(p = strstr(s, " ")) ||
	    (7 != sscanf(p,
			 "%u %u %u %u %u %u %u",
			 &vector[0],
			 &vector[1],
			 &vector[2],
			 &vector[3],
			 &vector[4],
			 &vector[5],
			 &vector[6]))) {
		return 0;
	}
	sum = 0.0;
	for (i=0; i<ARRAY_SIZE(vector); ++i) {
		sum += vector[i];
	}
	util = (1.0 - (vector[3] - vector_[3]) / (double)(sum - sum_)) * 100.0;
	sum_ = sum;
	for (i=0; i<ARRAY_SIZE(vector); ++i) {
		vector_[i] = vector[i];
	}
	return util;
}

int cpu_processes(const char *s) {
	int procs = -1;
	if (1 != sscanf(s, "processes %d", &procs)) {
		return -1;
	}
	return procs;
}

int cpu_procs_running(const char *s) {
	int pr = -1;
	if (1 != sscanf(s, "procs_running %d", &pr)) {
		return -1;
	}
	return pr;
}

int cpu_procs_blocked(const char *s) {
	int pb = -1;
	if (1 != sscanf(s, "procs_blocked %d", &pb)) {
		return -1;
	}
	return pb;
}

int mem_parse(const char *line, const char* prefix) {
	if (!is_prefix(prefix, line)) {
		return -3;
	}

	// find the :
	char *curr = strstr(line, ":");

	// go to the first space
	curr = curr + 1;

	// go to the non-first space
	for (;*curr != '\0' && *curr == ' '; curr = curr + 1) {}

	int val = -1;

	if (1 != sscanf(curr, "%d", &val)) {
		return -2;
	}

	return val;
}

char *go_till_digit(char *curr) {
	while (strlen(curr) > 0 && !isdigit(*curr)) {
		curr = curr + 1;
	}

	if (strlen(curr) <= 0) {
		return NULL;
	}

	return curr;
}

char *go_till_nondigit(char *curr) {
	while (strlen(curr) > 0 && isdigit(*curr)) {
		curr = curr + 1;
	}

	if (strlen(curr) <= 0) {
		return NULL;
	}

	return curr;
}

int net_parse(const char *line, int *rby, int *tby) {
	// find the :
	char *curr = strstr(line, ":");

	// go to the first char
	curr = curr + 1;

	int digit_cnt = 0;

	int rby_dc = 1;
	int tby_dc = 9;

	int digit = 0;

	char *tmp = malloc(1000);

	while(curr != NULL && (curr = go_till_digit(curr))) {
		digit_cnt++;

		if (sscanf(curr, "%d%s", &digit, tmp) <1) {
			free(tmp);
			return -1;
		}

		if (digit_cnt == rby_dc) {
			*rby = digit;
		} else if (digit_cnt == tby_dc) {
			*tby = digit;
		}

		curr = go_till_nondigit(curr);
	}

	free(tmp);
	return 0;
}

void uptime_util()
{
    char line[1024];
    FILE *file;
    float uptime, idle_time;
    if (!(file = fopen(PROC_UPTIME, "r")))
    {
        fprintf(stderr, "Error opening %s\n", PROC_UPTIME);
        return;
    }
    if (fgets(line, sizeof(line), file))
    {
        sscanf(line, "%f %f", &uptime, &idle_time);
        printf("[UPT] %.2f | Idle Time: %.2f\n", uptime, idle_time);
        fflush(stdout);
        fclose(file);
    }
    
}
int
main(int argc, char *argv[])
{
	const char * const PROC_STAT = "/proc/stat";
	const char * const MEM_STAT = "/proc/meminfo";
	const char * const NET_STAT = "/proc/net/dev";

	FILE *file;
	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	double cu = 0.0;
	int procs = 0, pr = 0, pb = 0;

	int mt = 0, mf = 0, ma = 0, buf = 0, cac = 0;
	int tmp = 0;

	int rby = 0, tby = 0;
	int prev_rby = 0, prev_tby = 0;

	uint64_t now = 0, then = 0;

	UNUSED(argc);
	UNUSED(argv);

	if (SIG_ERR == signal(SIGINT, _signal_)) {
		TRACE("signal()");
		return -1;
	}
	while (!done) {
		// CPU
		if (!(file = fopen(PROC_STAT, "r"))) {
			TRACE("fopen()");
			return -1;
		}

		/* if (fgets(line, sizeof (line), file)) {
			printf("\r%5.1f%%", cpu_util(line));
			fflush(stdout);
		} */

		while ((read = getline(&line, &len, file)) != -1) {
			/* printf("Retrieved line of length %zu:\n", read);
			printf("%s", line); */

			if (is_prefix("cpu ", line)) {
				cu = cpu_util(line);
			}

			if (is_prefix("processes ", line)) {
				procs = cpu_processes(line);
			}

			if (is_prefix("procs_running ", line)) {
				pr = cpu_procs_running(line);
			}

			if (is_prefix("procs_blocked ", line)) {
				pb = cpu_procs_blocked(line);
			}
		}

		fclose(file);

		// MEMORY

		if (!(file = fopen(MEM_STAT, "r"))) {
			TRACE("fopen()");
			return -1;
		}
 
		while ((read = getline(&line, &len, file)) != -1) {
			if ((tmp = mem_parse(line, "MemTotal")) >= 0) {
				mt = tmp / SCALE;
			}

			if ((tmp = mem_parse(line, "MemFree")) >= 0) {
				mf = tmp / SCALE;
			}

			if ((tmp = mem_parse(line, "MemAvailable")) >= 0) {
				ma = tmp / SCALE;
			}

			if ((tmp = mem_parse(line, "Buffers")) >= 0) {
				buf = tmp / SCALE;
			}

			if ((tmp = mem_parse(line, "Cached")) >= 0) {
				cac = tmp / SCALE;
			}
		}

		fclose(file);

		// NETWORK

		if (!(file = fopen(NET_STAT, "r"))) {
			TRACE("fopen()");
			return -1;
		}

		int line_no = 0;
		while ((read = getline(&line, &len, file)) != -1) {
			line_no++;
			if (line_no < 3) continue;

			if (net_parse(line, &rby, &tby)) {
				printf("NET ERR\n");
				exit(0);
			}
		}
		now = ref_time();
		double down = ((double)(rby-prev_rby) / 1024.0)/(((double)(now-then))/1000000.0);
		double up = ((double)(tby-prev_tby) / 1024.0)/(((double)(now-then))/1000000.0);
		prev_rby = rby;
		prev_tby = tby;

		fclose(file);

		if (system("clear")) {
			exit(0);
		}

		printf("[CPU] %%util:%5.1f%%\tprocs:%5d\trun:%7d\tblk:%7d\n", cu, procs, pr, pb);
		printf("[MEM] total:%5d\tfree:%6d\tavail:%5d\tcache:%5d\tkernbuf:%3d\n", mt, mf, ma, cac, buf);
		printf("[NET] down:%5.1f\tup:%7.1f\n", down, up);
		uptime_util();
		then = now;
		fflush(stdout);

		// us_sleep(500000 * 100);
		us_sleep(500000);
	}

	if (line)
		free(line);

	printf("\rDone!   \n");
	return 0;
}
