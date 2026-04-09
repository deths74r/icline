#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "../src/gstr.h"

/*
 * Test that gstr.h width functions return correct values for
 * the character classes that matter for terminal cursor positioning.
 */

static int failures = 0;

static void check_width(const char *label, const char *s, size_t expected) {
    size_t got = gstrwidth(s, strlen(s));
    if (got != expected) {
        printf("FAIL: %s — expected %zu, got %zu\n", label, expected, got);
        failures++;
    } else {
        printf("  ok: %s — width %zu\n", label, got);
    }
}

static void check_cpwidth(const char *label, uint32_t cp, int expected) {
    int got = utf8_cpwidth(cp);
    if (got != expected) {
        printf("FAIL: %s (U+%04X) — expected %d, got %d\n", label, cp, expected, got);
        failures++;
    } else {
        printf("  ok: %s (U+%04X) — width %d\n", label, cp, got);
    }
}

int main(void) {
    printf("=== Codepoint width (utf8_cpwidth) ===\n");

    /* ASCII */
    check_cpwidth("space", 0x20, 1);
    check_cpwidth("A", 0x41, 1);
    check_cpwidth("~", 0x7E, 1);
    check_cpwidth("NUL", 0x00, 0);
    check_cpwidth("BEL", 0x07, -1);
    check_cpwidth("DEL", 0x7F, -1);

    /* CJK Unified Ideographs (wide) */
    check_cpwidth("CJK 你", 0x4F60, 2);
    check_cpwidth("CJK 好", 0x597D, 2);
    check_cpwidth("CJK 世", 0x4E16, 2);
    check_cpwidth("CJK 界", 0x754C, 2);

    /* Hangul Syllables (wide) */
    check_cpwidth("Hangul 가", 0xAC00, 2);
    check_cpwidth("Hangul 힣", 0xD7A3, 2);

    /* Fullwidth Latin (wide) */
    check_cpwidth("Fullwidth A", 0xFF21, 2);
    check_cpwidth("Fullwidth ~", 0xFF5E, 2);

    /* Combining marks (zero-width) */
    check_cpwidth("combining acute", 0x0301, 0);
    check_cpwidth("combining tilde", 0x0303, 0);
    check_cpwidth("ZWJ", 0x200D, 0);
    check_cpwidth("ZWNJ", 0x200C, 0);
    check_cpwidth("VS16 (emoji presentation)", 0xFE0F, 0);

    /* Emoji (wide — presentation default or commonly rendered wide) */
    check_cpwidth("thumbs up", 0x1F44D, 2);
    check_cpwidth("grinning face", 0x1F600, 2);
    check_cpwidth("red heart", 0x2764, 1);  /* text presentation default */

    /* Hangul Jamo medial — old isocline hardcoded as width 0, but Unicode
     * East Asian Width is "N" (Neutral) = width 1. gstr follows the standard. */
    check_cpwidth("Jamo medial", 0x1160, 1);

    printf("\n=== String width (gstrwidth) ===\n");

    /* ASCII */
    check_width("hello", "hello", 5);
    check_width("empty", "", 0);

    /* CJK */
    check_width("你好", "你好", 4);
    check_width("你好世界", "你好世界", 8);

    /* Mixed ASCII + CJK */
    check_width("hi你好", "hi你好", 6);

    /* Emoji — single codepoint */
    check_width("thumbs up 👍", "👍", 2);
    check_width("grinning 😀", "😀", 2);

    /* Emoji — ZWJ family sequence (should be 1 grapheme, width 2) */
    check_width("family 👨‍👩‍👧", "👨\u200D👩\u200D👧", 2);

    /* Emoji — flag sequence (2 regional indicators = 1 flag, width 2) */
    check_width("flag 🇺🇸", "🇺🇸", 2);

    /* Combining marks — e + acute = 1 grapheme, width 1 */
    check_width("e + acute", "e\xCC\x81", 1);
    check_width("a + tilde", "a\xCC\x83", 1);

    /* Combining after CJK — 你 + combining = still width 2 */
    check_width("你 + combining", "你\xCC\x81", 2);

    /* Multiple combining marks */
    check_width("e + 2 combiners", "e\xCC\x81\xCC\x83", 1);

    /* VS16 emoji presentation */
    check_width("heart + VS16", "❤\xEF\xB8\x8F", 2);

    /* Keycap sequence: digit + VS16 + combining enclosing keycap */
    check_width("keycap 1️⃣", "1\xEF\xB8\x8F\xE2\x83\xA3", 2);

    printf("\n=== Results ===\n");
    if (failures == 0) {
        printf("All tests passed.\n");
    } else {
        printf("%d test(s) FAILED.\n", failures);
    }
    return failures ? 1 : 0;
}
