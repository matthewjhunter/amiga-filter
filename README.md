# Filter for the Amiga

Filter is a mail filter for AmigaOS. It watched a mailbox file with DOS file-change notification and sorted incoming messages into other files by matching headers (From:, To:, Sender:, Subject:, Newsgroups:). Matching messages could also be deleted, forwarded, or handed to a command. It was distributed on Aminet under comm/mail from 1996 to 1999.

It was the first real program I wrote, and my first open-source release. I started it in high school, at about 14, partly as a learning exercise; the first public release came a few years later.

This repository is an archive. Its first commit is the unmodified contents of `Filter15a.lzh`, the final 1.5a release from January 1999, which was also the first release to include source. Nothing here has been updated, and there is no plan to.

## Contents

- `src/` -- C source (SAS/C; see `SCOPTIONS` and `Filter.lnk`) and the object and binary files from the build
- `Filter.000`, `Filter.020`, `Filter.060` -- prebuilt binaries for the 68000, 68020, and 68060
- `Docs/` -- AmigaGuide manual, Aminet readme, and license
- `Prefs/` -- GUI preferences editor and a sample `.filterrc`
- `Install Filter` -- Amiga Installer script
- `*.info` -- Workbench icons
- `*~` -- editor backup files that were in the original archive

Requires AmigaOS 2.04 or later. The 1.5a release was packaged on a Linux machine and was never verified to compile in that form.

## License

My code is under the GNU GPL, version 2; see `Docs/COPYING`. As that file notes, two things in this archive are not mine and are not covered by the GPL: the preferences editor in `Prefs/` (written by "TTT", binary only), and the `.info` icons. They are included because they were part of the original distribution.
