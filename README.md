# arena

A single-header, fixed-capacity bump allocator for C11. One `malloc` per arena; nothing
is freed on its own, everything goes at once with `arena_reset` or `arena_destroy`.

## Use

Fetch `arena.h` at a tag into your vendor directory, never from a branch, and pin its
SHA-256 next to the tag. A tag can be moved; with the digest pinned, the bytes can only
change by editing the pin.

```sh
ARENA_TAG="v0.1.0"
ARENA_SHA256="4a242717d983a6a1a13b5bd8ae52a63004b41253ad056298488a1ce62e5ccf2c"
curl -sfL "https://raw.githubusercontent.com/foamdino/arena/${ARENA_TAG}/arena.h" \
     -o vendor/arena.h
echo "${ARENA_SHA256}  vendor/arena.h" | shasum -a 256 -c -
```

| Tag | `arena.h` SHA-256 |
|---|---|
| `v0.1.0` | `4a242717d983a6a1a13b5bd8ae52a63004b41253ad056298488a1ce62e5ccf2c` |

When you bump the tag, bump the pin with it: `curl -sfL <url> | shasum -a 256`.

Then `#include "arena.h"` with `-Ivendor`. Everything is `static inline`; there is no
`.c` to compile.

```c
arena a;
item *items;

arena_init(&a, 1 << 20);              /* exits if malloc fails */
items = arena_push_array(&a, item, n); /* zeroed, 8-byte aligned */
arena_check_oom(&a, "loading items");  /* exits if the arena is full */
/* ... */
arena_reset(&a);                       /* reuse for the next unit of work */
arena_destroy(&a);
```

## API

| Call | Does |
|---|---|
| `arena_init(a, cap)` | Allocates `cap` bytes. Prints a diagnostic and exits if `malloc` fails. |
| `arena_push(a, size)` | Next `size` bytes, rounded up to 8. On a full arena, returns NULL and sets `a->oom`. |
| `arena_push_zero(a, size)` | `arena_push`, then zeroes the bytes. |
| `arena_push_array(a, T, n)` | `arena_push_zero` for `n` of type `T`, cast to `T *`. |
| `arena_check_oom(a, context)` | If `a->oom` is set, prints `context` and usage, and exits 1. Call it after each push and there is no NULL path to handle. |
| `arena_reset(a)` | Empties the arena and clears `oom`. Keeps the buffer and `peak`. |
| `arena_destroy(a)` | Frees the buffer and zeroes the struct. |

`a->peak` is the most the arena has held since `arena_init`, kept across `arena_reset`.
Tests can assert it against a hand-written capacity formula, so the formula can't drift
away from the real allocations.

The capacity check can't wrap: a size near `SIZE_MAX` fails instead of slipping through.

## Build and test

```sh
./bootstrap.sh   # fetches nob.h, builds ./nob
./nob            # builds and runs test/test_arena.c under ASan and UBSan
```

## License

[0BSD](LICENSE).
