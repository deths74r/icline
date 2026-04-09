# icline: a portable readline alternative with modern Unicode

icline is a pure C library for rich line editing, forked from [Isocline](https://github.com/daanx/isocline) by Daan Leijen.

The key change: **Unicode 17.0** character width via [gstr.h](https://github.com/deths74r/gstr), replacing the original Unicode 5.0 (2007) width tables. This means correct cursor positioning for modern emoji, ZWJ sequences, CJK, and all scripts added in the last 17 years of Unicode.

- Small: less than 12k lines and can be compiled as a single C file without
  any dependencies or configuration (e.g. `gcc -c src/isocline.c`).

- Portable: works on Unix, Windows, and macOS, and uses a minimal
  subset of ANSI escape sequences.

- Features: extensive multi-line editing mode (`shift-tab`), (24-bit) color, history, completion, unicode,
  undo/redo, incremental history search, inline hints, syntax highlighting, brace matching,
  closing brace insertion, auto indentation, graceful fallback, support for custom allocators, etc.

- Unicode: **Unicode 17.0** character widths via gstr.h with grapheme-cluster-aware display width.
  Correct handling of ZWJ emoji (👨‍👩‍👧 = width 2), flag sequences (🇺🇸 = width 2),
  emoji presentation (VS16), combining marks, and all CJK blocks.

- License: MIT.

# What Changed from Isocline

- Replaced `src/wcwidth.c` (Markus Kuhn, Unicode 5.0, 292 lines) with `src/gstr.h` (Unicode 17.0, grapheme-aware)
- All isocline API and behavior is preserved — drop-in compatible
- The `ic_` function prefix is unchanged
- Single-file build is unchanged: `gcc -c src/isocline.c`

# Demo

![recording](doc/record-macos.svg)

Shows in order: unicode, syntax highlighting, brace matching, jump to matching brace, auto indent, multiline editing, 24-bit colors, inline hinting, filename completion, and incremental history search.
<sub>(screen capture was made with [termtosvg] by Nicolas Bedos)</sub>

# Usage

Include the isocline header in your C or C++ source:
```C
#include <include/isocline.h>
```

and call `ic_readline` to get user input with rich editing abilities:
```C
char* input;
while( (input = ic_readline("prompt")) != NULL ) { // ctrl+d/c or errors return NULL
  printf("you typed:\n%s\n", input); // use the input
  free(input);
}
```

See the [example] for a full example with completion, syntax highlighting, history, etc.

# Run the Example

You can compile and run the [example] as:
```
$ gcc -o example -Iinclude test/example.c src/isocline.c
$ ./example
```

# Run the Unicode Width Tests

```
$ gcc -o test_width test/test_width.c -std=c99
$ ./test_width
```

Tests cover ASCII, CJK, Hangul, combining marks, ZWJ emoji, flag sequences, keycap sequences, and VS16 presentation.

# Editing with icline

icline tries to be as compatible as possible with standard [GNU Readline] key bindings.

### Overview:
```apl
       home/ctrl-a       cursor     end/ctrl-e
         ┌─────────────────┼───────────────┐    (navigate)
         │     ctrl-left   │  ctrl-right   │
         │         ┌───────┼──────┐        │    ctrl-r   : search history
         ▼         ▼       ▼      ▼        ▼    tab      : complete word
  prompt> it is the quintessential language     shift-tab: insert new line
         ▲         ▲              ▲        ▲    esc      : delete input, done
         │         └──────────────┘        │    ctrl-z   : undo
         │    alt-backsp        alt-d      │
         └─────────────────────────────────┘    (delete)
       ctrl-u                          ctrl-k
```

<sub>Note: on macOS, the meta (alt) key is not directly available in most terminals.
Terminal/iTerm2 users can activate the meta key through
`Terminal` &rarr; `Preferences` &rarr; `Settings` &rarr; `Use option as meta key`.</sub>

### Key Bindings

These are also shown when pressing `F1` on a prompt. We use `^` as a shorthand for `ctrl-`:

| Navigation        |                                                 |
|-------------------|-------------------------------------------------|
| `left`,`^b`       | go one character to the left |
| `right`,`^f   `   | go one character to the right |
| `up           `   | go one row up, or back in the history |
| `down         `   | go one row down, or forward in the history |
| `^left        `   | go to the start of the previous word |
| `^right       `   | go to the end the current word |
| `home`,`^a    `   | go to the start of the current line |
| `end`,`^e     `   | go to the end of the current line |
| `pgup`,`^home `   | go to the start of the current input |
| `pgdn`,`^end  `   | go to the end of the current input |
| `alt-m        `   | jump to matching brace |
| `^p           `   | go back in the history |
| `^n           `   | go forward in the history |
| `^r`,`^s      `   | search the history starting with the current word |


| Deletion        |                                                 |
|-------------------|-------------------------------------------------|
| `del`,`^d     `   | delete the current character |
| `backsp`,`^h  `   | delete the previous character |
| `^w           `   | delete to preceding white space |
| `alt-backsp   `   | delete to the start of the current word |
| `alt-d        `   | delete to the end of the current word |
| `^u           `   | delete to the start of the current line |
| `^k           `   | delete to the end of the current line |
| `esc          `   | delete the current input, or done with empty input |


| Editing           |                                                 |
|-------------------|-------------------------------------------------|
| `enter        `   | accept current input |
| `^enter`,`^j`,`shift-tab` | create a new line for multi-line input |
| `^l           `   | clear screen |
| `^t           `   | swap with previous character (move character backward) |
| `^z`,`^_      `   | undo |
| `^y           `   | redo |
| `tab          `   | try to complete the current input |


| Completion menu   |                                                 |
|-------------------|-------------------------------------------------|
| `enter`,`left`    | use the currently selected completion |
| `1` - `9`         | use completion N from the menu |
| `tab, down    `   | select the next completion |
| `shift-tab, up`   | select the previous completion |
| `esc          `   | exit menu without completing |
| `pgdn`,`^enter`,`^j`   | show all further possible completions |


| Incremental history search        |                                                 |
|-------------------|-------------------------------------------------|
| `enter        `   | use the currently found history entry |
| `backsp`,`^z  `   | go back to the previous match (undo) |
| `tab`,`^r`,`up`   | find the next match |
| `shift-tab`,`^s`,`down`  | find an earlier match |
| `esc          `   | exit search |


# Build the Library

### Build as a Single Source

Copy the sources (in `include` and `src`) into your project, or add the library as a [submodule]:
```
$ git submodule add https://github.com/deths74r/icline
```
and add `icline/src/isocline.c` to your build rules -- no configuration is needed.

### Build with CMake

Clone the repository and run cmake to build a static library (`.a`/`.lib`):
```
$ git clone https://github.com/deths74r/icline
$ cd icline
$ mkdir -p build/release
$ cd build/release
$ cmake ../..
$ cmake --build .
```
This builds a static library `libisocline.a` (or `isocline.lib` on Windows)
and the example program:
```
$ ./example
```

# API Reference

See the [C API reference][docapi] and the [example] for example usage of history, completion, etc.

# Formatted Output

icline exposes functions for rich terminal output
as `ic_print` (and `ic_println` and `ic_printf`).
Inspired by the (Python) [Rich][RichBBcode] library,
this supports a form of [bbcode]'s to format the output:
```c
ic_println( "[b]bold [red]and red[/red][/b]" );
```

Predefined styles include `b` (bold),
`u` (underline), `i` (italic), and `r` (reverse video), but
you can (re)define any style yourself as:
```c
ic_style_def("warning", "crimson u");
ic_println( "[warning]this is a warning![/]" );
```

# Origin

icline is a fork of [Isocline](https://github.com/daanx/isocline) v1.0.9 (2022-01-15) by Daan Leijen.
The original was created for use in the [Koka] interactive compiler.

Changes from upstream:
- Unicode 17.0 character width via [gstr.h](https://github.com/deths74r/gstr) (replacing Unicode 5.0 wcwidth.c)
- Unicode width test suite

# Releases

* `2026-04-09`: v1.1.0: Replace wcwidth.c (Unicode 5.0) with gstr.h (Unicode 17.0) for modern emoji, ZWJ, and CJK support.
* `2022-01-15`: v1.0.9: (isocline) fix missing `ic_completion_arg`, fix null ptr check, fix /dev/null crash.
* `2021-08-20`: v1.0.0: (isocline) initial release.


[GNU readline]: https://tiswww.case.edu/php/chet/readline/rltop.html
[koka]: http://www.koka-lang.org
[submodule]: https://git-scm.com/book/en/v2/Git-Tools-Submodules
[example]: https://github.com/deths74r/icline/blob/main/test/example.c
[termtosvg]: https://github.com/nbedos/termtosvg
[Rich]: https://github.com/willmcgugan/rich
[RichBBcode]: https://rich.readthedocs.io/en/latest/markup.html
[bbcode]: https://en.wikipedia.org/wiki/BBCode
[htmlcolors]: https://en.wikipedia.org/wiki/Web_colors#HTML_color_names
[ansicolors]: https://en.wikipedia.org/wiki/Web_colors#Basic_colors
[ansicolor256]: https://en.wikipedia.org/wiki/ANSI_escape_code#8-bit
[docapi]: https://daanx.github.io/isocline
