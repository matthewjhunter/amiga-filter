# Filter for the Amiga

Filter is a mail filter for AmigaOS. It watched a mailbox file with DOS file-change notification and sorted incoming messages into other files by matching headers (From:, To:, Sender:, Subject:, Newsgroups:). Matching messages could also be deleted, forwarded, or handed to a command. It was distributed on Aminet under comm/mail from 1996 to 1999.

It was the first real program I wrote, and my first open-source release. I started it in high school, at about 14, partly as a learning exercise; the first public release came a few years later.

The first commit in this repository is the unmodified contents of `Filter15a.lzh`, the final 1.5a release from January 1999 and the first to include source. It is tagged `v1.5a`, and the original archive is attached to that release. Everything after it is 1.5b, a 2026 bugfix update that fixes the old code's problems without changing how Filter works.

## Contents

- `src/` -- C source (SAS/C; see `SCOPTIONS` and `Filter.lnk`) and the object and binary files from the build
- `Filter.000`, `Filter.020`, `Filter.060` -- prebuilt binaries for the 68000, 68020, and 68060
- `Docs/` -- AmigaGuide manual, Aminet readme, and license
- `Prefs/` -- GUI preferences editor and a sample `.filterrc`
- `Install Filter` -- Amiga Installer script
- `*.info` -- Workbench icons
- `*~` -- editor backup files that were in the original archive
- `tests/` -- host test harness and Amiga cross-build script, added in 1.5b

Requires AmigaOS 2.04 or later.

## 1.5b

The 1.5a release was packaged on a Linux machine and was never verified to compile in that form, and it doesn't: `src/Filter.c` contains `strncpy(emsg.Reply-To, buff, 256)`, which C parses as a subtraction, while the struct field in `filter.h` is `ReplyTo`. The source files are dated November 1998, after the February 1998 binaries were built, so the binaries came from an earlier version of the code.

1.5b fixes that and the other bugs that could be fixed without redesigning anything:

- Mail delivered while Filter was working could be lost. Filter emptied the mailbox after filtering it, erasing anything appended in the meantime. It now renames the mailbox to `<mailbox>.filter`, leaves an empty mailbox in its place, and filters the renamed file.
- Buffer overflows in header handling, config parsing, delivery paths, and the `COMMAND:` and `FORWARD:` actions, and a one-byte underflow on every mailbox that ended in a blank line.
- Messages were copied one byte short, so folders lost the blank line that separates messages.
- A message with no Newsgroups: header was matched against the previous message's.
- A memory leak on each delivery when the config file was not in the first place Filter looked, and heavy stack use from passing 2 KB structures by value.

The commit messages give the details.

## Building and testing

`make -C tests` compiles the filtering code on a Unix host against stand-ins for the AmigaOS calls and runs the tests under AddressSanitizer. `tests/cross-build.sh` builds 68000, 68020, and 68060 binaries with bebbo's m68k-amigaos-gcc in Docker.

That is the only testing 1.5b has had. It has not been built with SAS/C or run on an Amiga. The binaries and object file in the top directory and `src/` are still the original 1.5 builds.

## License

My code is under the GNU GPL, version 2; see `Docs/COPYING`. As that file notes, two things in this archive are not mine and are not covered by the GPL: the preferences editor in `Prefs/` (written by "TTT", binary only), and the `.info` icons. They are included because they were part of the original distribution.
