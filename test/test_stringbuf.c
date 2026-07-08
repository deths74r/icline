#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Compile the library into this translation unit to reach internal
 * (ic_private) functions; do not link against libicline as well. */
#include "../src/icline.c"

/*
 * Regression tests for sbuf_append_vprintf: exact-fit formatted output
 * used to be truncated by one char while count advanced past an
 * embedded NUL (vsnprintf was given `avail` although buf[buflen] is
 * reserved for the terminating zero).
 */

static int failures = 0;

static void check(const char *label, int cond) {
    if (!cond) {
        printf("FAIL: %s\n", label);
        failures++;
    } else {
        printf("  ok: %s\n", label);
    }
}

int main(void) {
    alloc_t mem = { &malloc, &realloc, &free };

    printf("=== sbuf_append_vprintf ===\n");

    /* Exact fit on first allocation: a fresh stringbuf grows to
     * buflen=120, so a 120-char result lands exactly on the boundary. */
    stringbuf_t *sb = sbuf_new(&mem);
    char s120[121];
    memset(s120, 'x', 120); s120[120] = 0;
    ssize_t n = sbuf_appendf(sb, "%s", s120);
    check("exact fit: count == 120", n == 120);
    check("exact fit: strlen == 120", strlen(sbuf_string(sb)) == 120);
    check("exact fit: content intact", memcmp(sbuf_string(sb), s120, 121) == 0);

    /* Exact fit against the boundary of a non-empty buffer. */
    ssize_t avail = sb->buflen - sbuf_len(sb);
    char *fill = (char *)malloc((size_t)avail + 1);
    memset(fill, 'y', (size_t)avail); fill[avail] = 0;
    ssize_t before = sbuf_len(sb);
    n = sbuf_appendf(sb, "%s", fill);
    check("boundary fit: count advanced by avail", n == before + avail);
    check("boundary fit: strlen == count", (ssize_t)strlen(sbuf_string(sb)) == n);
    free(fill);

    /* Overflow: needed > avail exercises the grow-and-retry path. */
    stringbuf_t *sb2 = sbuf_new(&mem);
    char big[501];
    memset(big, 'z', 500); big[500] = 0;
    n = sbuf_appendf(sb2, "[%s]", big);
    check("overflow: count == 502", n == 502);
    check("overflow: strlen == 502", strlen(sbuf_string(sb2)) == 502);
    check("overflow: delimiters intact",
          sbuf_string(sb2)[0] == '[' && sbuf_string(sb2)[501] == ']');

    /* Plain small append. */
    stringbuf_t *sb3 = sbuf_new(&mem);
    n = sbuf_appendf(sb3, "%d-%s", 42, "ok");
    check("small: count == 5", n == 5);
    check("small: content", strcmp(sbuf_string(sb3), "42-ok") == 0);

    sbuf_free(sb); sbuf_free(sb2); sbuf_free(sb3);

    printf("\n=== Results ===\n");
    if (failures == 0) {
        printf("All tests passed.\n");
    } else {
        printf("%d test(s) FAILED.\n", failures);
    }
    return failures ? 1 : 0;
}
