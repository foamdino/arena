/* SPDX-License-Identifier: 0BSD */

#ifndef ARENA_H
#define ARENA_H

/* A fixed-capacity bump allocator: nothing is freed on its own, everything goes with
 * arena_reset or arena_destroy. Pushes are 8-byte aligned. A full arena sets oom and
 * returns NULL; arena_check_oom turns that into a diagnostic and exit(1), so callers
 * that check after each push need no NULL path. peak, kept across arena_reset, is the
 * most the arena has held, for tests that size an arena from its worst unit of work. */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define arena_push_array(a, T, n) ((T *)arena_push_zero((a), sizeof(T) * (n)))

typedef struct
{
	uint8_t *buf;
	size_t used;
	size_t cap;
	size_t peak; /* the most used has been since arena_init */
	uint8_t oom;
} arena;

/* If the backing allocation fails, prints a diagnostic and exits; callers need no
 * malloc-failure path. */
static inline void
arena_init(arena *a, size_t cap)
{
	a->buf = (uint8_t *)malloc(cap);
	if (!a->buf)
	{
		fprintf(stderr, "arena: allocation failed (%zu bytes)\n", cap);
		exit(1);
	}
	a->used = 0;
	a->cap  = cap;
	a->peak = 0;
	a->oom  = 0;
}

/* NULL, with oom set, when the arena is full. The capacity test cannot wrap, even for
 * a size near SIZE_MAX. */
static inline void *
arena_push(arena *a, size_t size)
{
	size_t aligned;
	void *ptr;

	aligned = (size + 7) & ~(size_t)7;
	if (aligned < size || aligned > a->cap - a->used)
	{
		a->oom = 1;
		return NULL;
	}
	ptr = a->buf + a->used;
	a->used += aligned;
	if (a->used > a->peak)
		a->peak = a->used;
	return ptr;
}

static inline void *
arena_push_zero(arena *a, size_t size)
{
	void *ptr;

	ptr = arena_push(a, size);
	if (ptr)
		memset(ptr, 0, size);
	return ptr;
}

static inline void
arena_check_oom(arena *a, const char *context)
{
	if (a->oom)
	{
		fprintf(stderr,
		        "arena: exhausted (%s, %zu/%zu bytes)\n",
		        context,
		        a->used,
		        a->cap);
		exit(1);
	}
}

static inline void
arena_reset(arena *a)
{
	a->used = 0;
	a->oom  = 0;
}

static inline void
arena_destroy(arena *a)
{
	free(a->buf);
	a->buf  = NULL;
	a->used = 0;
	a->cap  = 0;
	a->peak = 0;
}

#endif
