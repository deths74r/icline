#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Compile the library into this translation unit to reach internal
 * (ic_private) functions; do not link against libicline as well. */
#include "../src/icline.c"

/*
 * Editing-operation tests: backspace/delete/transpose and layout must
 * treat a grapheme cluster as one unit — deleting part of an emoji or
 * splitting a flag pair is never correct.
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

static const char *FAMILY  = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7"; /* 👨‍👩‍👧 18 bytes */
static const char *FLAG    = "\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8"; /* 🇺🇸 8 bytes */
static const char *HAN_LVT = "\xE1\x84\x92\xE1\x85\xA1\xE1\x86\xAB"; /* decomposed 한, 9 bytes */

int main(void) {
    alloc_t mem = { &malloc, &realloc, &free };

    printf("=== backspace (sbuf_delete_char_before) ===\n");
    {
        stringbuf_t *sb = sbuf_new(&mem);
        sbuf_append(sb, "a");
        sbuf_append(sb, FAMILY);
        ssize_t pos = sbuf_delete_char_before(sb, sbuf_len(sb));
        check("one backspace removes whole family emoji",
              pos == 1 && strcmp(sbuf_string(sb), "a") == 0);
        pos = sbuf_delete_char_before(sb, pos);
        check("second backspace empties the buffer",
              pos == 0 && sbuf_len(sb) == 0);
        sbuf_free(sb);
    }
    {
        stringbuf_t *sb = sbuf_new(&mem);
        sbuf_append(sb, HAN_LVT);
        ssize_t pos = sbuf_delete_char_before(sb, sbuf_len(sb));
        check("backspace removes whole decomposed Hangul syllable",
              pos == 0 && sbuf_len(sb) == 0);
        sbuf_free(sb);
    }

    printf("\n=== forward delete (sbuf_delete_char_at) ===\n");
    {
        stringbuf_t *sb = sbuf_new(&mem);
        sbuf_append(sb, FAMILY);
        sbuf_append(sb, "b");
        sbuf_delete_char_at(sb, 0);
        check("forward delete removes whole family emoji",
              strcmp(sbuf_string(sb), "b") == 0);
        sbuf_free(sb);
    }
    {
        stringbuf_t *sb = sbuf_new(&mem);
        sbuf_append(sb, FLAG);
        sbuf_delete_char_at(sb, 0);
        check("deleting a flag never leaves a lone regional indicator",
              sbuf_len(sb) == 0);
        sbuf_free(sb);
    }

    printf("\n=== transpose (sbuf_swap_char) ===\n");
    {
        stringbuf_t *sb = sbuf_new(&mem);
        sbuf_append(sb, "a");
        sbuf_append(sb, FAMILY);
        /* editline calls this with the cursor strictly between the two units */
        sbuf_swap_char(sb, 1);
        char expected[32];
        snprintf(expected, sizeof(expected), "%sa", FAMILY);
        check("transpose swaps cluster and ascii atomically",
              strcmp(sbuf_string(sb), expected) == 0);
        sbuf_free(sb);
    }

    printf("\n=== fit functions never split a cluster ===\n");
    check("take_while_fit(family, 1) == 0", str_take_while_fit(FAMILY, 1) == 0);
    check("take_while_fit(family, 2) == 18", str_take_while_fit(FAMILY, 2) == 18);
    {
        char s[32];
        snprintf(s, sizeof(s), "%sx", FAMILY);
        check("skip_until_fit lands on cluster boundary", str_skip_until_fit(s, 1) == 18);
    }

    printf("\n=== wrapping (wide cluster wraps atomically) ===\n");
    {
        stringbuf_t *sb = sbuf_new(&mem);
        sbuf_append(sb, "aa");
        sbuf_append(sb, FAMILY);
        rowcol_t rc;
        memset(&rc, 0, sizeof(rc));
        /* termw 5 leaves 4 usable columns (one is reserved for the cursor):
         * "aa" fits, the 2-col family cluster must wrap as a whole */
        sbuf_get_rc_at_pos(sb, 5, 0, 0, sbuf_len(sb), &rc);
        check("wide cluster wraps to next row", rc.row == 1);
        check("cluster occupies 2 columns on the wrapped row", rc.col == 2);
        /* mid-cluster position (a bad completer could produce one): degrades
         * gracefully — still resolves to the cluster's row */
        memset(&rc, 0, sizeof(rc));
        sbuf_get_rc_at_pos(sb, 5, 0, 0, 5, &rc);
        check("mid-cluster position resolves to the cluster's row",
              rc.row == 1 && rc.row_start == 2);
        sbuf_free(sb);
    }

    printf("\n=== public API: ic_next_char / ic_prev_char step clusters ===\n");
    {
        char s[32];
        snprintf(s, sizeof(s), "a%sb", FAMILY);
        check("ic_next_char jumps whole family", ic_next_char(s, 1) == 19);
        check("ic_prev_char jumps whole family", ic_prev_char(s, 19) == 1);
    }

    printf("\n=== Results ===\n");
    if (failures == 0) {
        printf("All tests passed.\n");
    } else {
        printf("%d test(s) FAILED.\n", failures);
    }
    return failures ? 1 : 0;
}
