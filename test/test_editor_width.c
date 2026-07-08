#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Compile the library into this translation unit to reach internal
 * (ic_private) functions; do not link against libicline as well. */
#include "../src/icline.c"

/*
 * Editor-path width and iteration tests: these call the editor's own
 * str_column_width / str_next_ofs / str_prev_ofs — not gstr.h directly —
 * so a gap between the width engine and the editor (the original fork
 * bug) cannot pass unnoticed again.
 */

static int failures = 0;

static void check(const char *label, long got, long expected) {
    if (got != expected) {
        printf("FAIL: %s — expected %ld, got %ld\n", label, expected, got);
        failures++;
    } else {
        printf("  ok: %s — %ld\n", label, got);
    }
}

/* Test strings (UTF-8 bytes spelled out where invalid/partial sequences matter). */
static const char *FAMILY  = "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7"; /* 👨‍👩‍👧 18 bytes */
static const char *FLAG    = "\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8"; /* 🇺🇸 8 bytes */
static const char *HEART16 = "\xE2\x9D\xA4\xEF\xB8\x8F";         /* ❤️ 6 bytes */
static const char *KEYCAP  = "1\xEF\xB8\x8F\xE2\x83\xA3";        /* 1️⃣ 7 bytes */
static const char *HAN_LVT = "\xE1\x84\x92\xE1\x85\xA1\xE1\x86\xAB"; /* decomposed 한, 9 bytes */

int main(void) {
    printf("=== str_column_width (the readme claims, on the editor path) ===\n");
    check("family ZWJ width", str_column_width(FAMILY), 2);
    check("flag width", str_column_width(FLAG), 2);
    check("heart + VS16 width", str_column_width(HEART16), 2);
    check("keycap width", str_column_width(KEYCAP), 2);

    printf("\n=== single units from str_next_ofs ===\n");
    ssize_t w;
    check("family is one unit", str_next_ofs(FAMILY, 18, 0, &w), 18);
    check("family unit width", w, 2);
    check("flag is one unit", str_next_ofs(FLAG, 8, 0, &w), 8);
    check("flag unit width", w, 2);
    check("decomposed Hangul is one unit", str_next_ofs(HAN_LVT, 9, 0, &w), 9);
    check("decomposed Hangul width", w, 2);

    printf("\n=== Hangul ===\n");
    check("composed 한", str_column_width("\xED\x95\x9C"), 2);
    check("decomposed L+V", str_column_width("\xE1\x84\x92\xE1\x85\xA1"), 2);
    check("lone medial jamo U+1160", str_column_width("\xE1\x85\xA0"), 0);
    check("lone leading jamo U+1112", str_column_width("\xE1\x84\x92"), 2);

    printf("\n=== combining marks ===\n");
    check("e + acute is one unit", str_next_ofs("e\xCC\x81", 3, 0, &w), 3);
    check("e + acute width", w, 1);
    check("CJK + mark width", str_column_width("\xE4\xBD\xA0\xCC\x81"), 2);
    check("skin-tone thumbs up", str_column_width("\xF0\x9F\x91\x8D\xF0\x9F\x8F\xBD"), 2);

    printf("\n=== escape sequences ===\n");
    check("CSI colored 'ab' width", str_column_width("a\x1B[31mb"), 2);
    check("CSI is one unit", str_next_ofs("\x1B[31mx", 6, 0, &w), 5);
    check("CSI unit width", w, 0);
    check("OSC with BEL", str_next_ofs("\x1B]0;t\x07x", 7, 0, &w), 6);
    check("OSC with ST", str_next_ofs("\x1B]0;t\x1B\\x", 8, 0, &w), 7);
    check("escape then cluster does not merge", str_next_ofs("\x1B[31m\xE2\x9D\xA4\xEF\xB8\x8F", 11, 5, &w), 6);

    printf("\n=== controls (CR+LF must NOT cluster: row layout finds \\n) ===\n");
    check("unit at 'a'", str_next_ofs("a\r\nb", 4, 0, NULL), 1);
    check("unit at CR is 1 byte", str_next_ofs("a\r\nb", 4, 1, NULL), 1);
    check("unit at LF is 1 byte", str_next_ofs("a\r\nb", 4, 2, NULL), 1);
    check("tab width", str_column_width("\t"), 0);

    printf("\n=== degenerate clusters ===\n");
    check("lone ZWJ", str_column_width("\xE2\x80\x8D"), 0);
    check("a + ZWJ", str_column_width("a\xE2\x80\x8D"), 1);
    check("single regional indicator", str_column_width("\xF0\x9F\x87\xBA"), 1);
    check("three RIs: pair + lone", str_column_width("\xF0\x9F\x87\xBA\xF0\x9F\x87\xB8\xF0\x9F\x87\xBA"), 3);
    check("lone VS16", str_column_width("\xEF\xB8\x8F"), 0);

    printf("\n=== invalid utf-8 (must terminate with progress) ===\n");
    check("lone 0xFF advances", str_next_ofs("\xFF" "a", 2, 0, NULL) > 0, 1);
    check("truncated 4-byte seq advances", str_next_ofs("\xF0\x9F\x91", 3, 0, NULL) > 0, 1);

    printf("\n=== str_prev_ofs (backward navigation) ===\n");
    check("back over family", str_prev_ofs("a" "\xF0\x9F\x91\xA8\xE2\x80\x8D\xF0\x9F\x91\xA9\xE2\x80\x8D\xF0\x9F\x91\xA7", 19, NULL), 18);
    check("back over ascii", str_prev_ofs("ab", 2, NULL), 1);
    check("back over e+acute", str_prev_ofs("e\xCC\x81", 3, NULL), 3);
    check("back over LF (CRLF split)", str_prev_ofs("a\r\n", 3, NULL), 1);

    /* round-trip: prev(next(pos)) == pos over a mixed corpus */
    {
        char corpus[256];
        snprintf(corpus, sizeof(corpus), "he\xCC\x81llo %s wo\t%s\x1B[1m%s end", FAMILY, FLAG, HAN_LVT);
        ssize_t len = (ssize_t)strlen(corpus);
        int roundtrip_ok = 1;
        ssize_t pos = 0;
        while (pos < len) {
            ssize_t next = str_next_ofs(corpus, len, pos, NULL);
            if (next <= 0) { roundtrip_ok = 0; break; }
            /* escapes are excluded: str_prev_ofs is documented not to skip back over CSI */
            if (corpus[pos] != '\x1B') {
                ssize_t back = str_prev_ofs(corpus, pos + next, NULL);
                if (back != next) { roundtrip_ok = 0; break; }
            }
            pos += next;
        }
        check("prev/next round-trip over mixed corpus", roundtrip_ok, 1);
    }

    /* zalgo: bounded backtrack must terminate and make progress */
    {
        char zalgo[512];
        zalgo[0] = 'a';
        for (int i = 0; i < 200; i++) { zalgo[1+2*i] = '\xCC'; zalgo[2+2*i] = '\x81'; }
        zalgo[401] = 0;
        ssize_t p = 401;
        int steps = 0;
        while (p > 0 && steps < 500) {
            ssize_t back = str_prev_ofs(zalgo, p, NULL);
            if (back <= 0) break;
            p -= back; steps++;
        }
        check("zalgo backward walk reaches start", p, 0);
    }

    printf("\n=== str_next_cp_ofs stays codepoint-based ===\n");
    check("first codepoint of family", str_next_cp_ofs(FAMILY, 18, 0, NULL), 4);
    check("cp iterator still skips escapes", str_next_cp_ofs("\x1B[31mx", 6, 0, NULL), 5);

    printf("\n=== Results ===\n");
    if (failures == 0) {
        printf("All tests passed.\n");
    } else {
        printf("%d test(s) FAILED.\n", failures);
    }
    return failures ? 1 : 0;
}
