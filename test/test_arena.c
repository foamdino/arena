/* SPDX-License-Identifier: 0BSD */

#include "arena.h"

#include <stdint.h>
#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>

static int failures = 0;
static int checks   = 0;

#define check(cond, msg)                                                                 \
	do                                                                               \
	{                                                                                \
		checks++;                                                                \
		if (!(cond))                                                             \
		{                                                                        \
			failures++;                                                      \
			fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, msg);   \
		}                                                                        \
	} while (0)

static void
test_alignment(void)
{
	arena a;
	uint8_t *p;
	uint8_t *q;

	arena_init(&a, 64);
	p = arena_push(&a, 1);
	q = arena_push(&a, 1);
	check(p == a.buf, "first push starts at the buffer");
	check(q == p + 8, "a 1-byte push takes 8 bytes");
	check(((uintptr_t)q & 7) == 0, "pushes are 8-byte aligned");
	check(a.used == 16, "used counts aligned sizes");
	arena_destroy(&a);
}

static void
test_push_zero(void)
{
	arena a;
	uint8_t *p;
	int zeroed;

	arena_init(&a, 64);
	memset(a.buf, 0xAB, 64);
	p      = arena_push_zero(&a, 40);
	zeroed = 1;
	for (size_t i = 0; i < 40; i++)
		if (p[i] != 0)
			zeroed = 0;
	check(zeroed, "push_zero zeroes what it returns");
	arena_destroy(&a);
}

static void
test_push_array(void)
{
	arena a;
	uint32_t *xs;

	arena_init(&a, 64);
	xs = arena_push_array(&a, uint32_t, 5);
	check(xs != NULL, "push_array fits");
	check(a.used == 24, "5 uint32_t round up to 24 bytes");
	check(xs[4] == 0, "push_array zeroes");
	arena_destroy(&a);
}

static void
test_exact_fit_and_overflow(void)
{
	arena a;

	arena_init(&a, 32);
	check(arena_push(&a, 32) != NULL, "a push of exactly cap fits");
	check(!a.oom, "an exact fit does not set oom");
	check(arena_push(&a, 1) == NULL, "one byte over cap fails");
	check(a.oom, "a failed push sets oom");
	check(a.used == 32, "a failed push leaves used alone");
	arena_destroy(&a);
}

/* Regression: `used + aligned > cap` wraps for a size near SIZE_MAX and lets the push
 * through. */
static void
test_huge_size_does_not_wrap(void)
{
	arena a;

	arena_init(&a, 64);
	arena_push(&a, 8);
	check(arena_push(&a, SIZE_MAX) == NULL, "SIZE_MAX fails");
	check(arena_push(&a, SIZE_MAX - 3) == NULL, "SIZE_MAX - 3 (rounds to 0) fails");
	check(arena_push(&a, SIZE_MAX - 8) == NULL, "SIZE_MAX - 8 fails");
	check(a.oom, "huge pushes set oom");
	check(a.used == 8, "huge pushes leave used alone");
	arena_destroy(&a);
}

static void
test_reset_keeps_peak(void)
{
	arena a;

	arena_init(&a, 64);
	arena_push(&a, 40);
	arena_push(&a, 64);
	arena_reset(&a);
	check(a.used == 0, "reset empties the arena");
	check(!a.oom, "reset clears oom");
	check(a.peak == 40, "reset keeps peak");
	arena_push(&a, 16);
	check(a.peak == 40, "a smaller unit of work leaves peak alone");
	arena_destroy(&a);
	check(a.buf == NULL && a.cap == 0 && a.peak == 0, "destroy clears the arena");
}

/* Runs arena_check_oom in a child; returns its exit status, or -1 if it did not exit. */
static int
check_oom_status(int full)
{
	pid_t pid;
	int status;
	arena a;

	pid = fork();
	if (pid == 0)
	{
		freopen("/dev/null", "w", stderr);
		arena_init(&a, 8);
		if (full)
			arena_push(&a, 16);
		arena_check_oom(&a, "test");
		_exit(0);
	}
	if (pid < 0 || waitpid(pid, &status, 0) != pid || !WIFEXITED(status))
		return -1;
	return WEXITSTATUS(status);
}

static void
test_check_oom(void)
{
	check(check_oom_status(0) == 0, "check_oom returns when the arena has room");
	check(check_oom_status(1) == 1, "check_oom exits 1 when the arena is full");
}

int
main(void)
{
	test_alignment();
	test_push_zero();
	test_push_array();
	test_exact_fit_and_overflow();
	test_huge_size_does_not_wrap();
	test_reset_keeps_peak();
	test_check_oom();
	printf("test_arena: %d/%d checks passed\n", checks - failures, checks);
	return failures ? 1 : 0;
}
