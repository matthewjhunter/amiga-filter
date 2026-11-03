/*
 * Host implementations of the AmigaOS calls declared in amiga_stubs.h.
 * Only as much behavior as Filter's tests need; see the notes on each.
 */
#define _GNU_SOURCE
#include <ctype.h>
#include <fnmatch.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "amiga_stubs.h"

int stub_close_zero_calls = 0;

static struct Task the_task;

static FILE *fh_file(BPTR fh) { return (FILE *)(intptr_t)fh; }
static BPTR file_fh(FILE *f) { return (BPTR)(intptr_t)f; }

void *AllocVec(ULONG size, ULONG flags)
{
	return (flags & MEMF_CLEAR) ? calloc(1, size) : malloc(size);
}

void FreeVec(void *mem) { free(mem); }
LONG AllocSignal(LONG num) { (void)num; return 16; }
void FreeSignal(LONG num) { (void)num; }
ULONG Wait(ULONG mask) { return mask & SIGBREAKF_CTRL_C; }
void Signal(struct Task *task, ULONG mask) { (void)task; (void)mask; }
struct Task *FindTask(const char *name) { (void)name; return &the_task; }

BOOL StartNotify(struct NotifyRequest *nr) { (void)nr; return DOSTRUE; }
void EndNotify(struct NotifyRequest *nr) { (void)nr; }
void Delay(LONG ticks) { (void)ticks; }

/* No real locking on the host: a lock succeeds if the object exists. */
BPTR Lock(const char *name, LONG mode)
{
	(void)mode;
	return access(name, F_OK) == 0 ? 1 : 0;
}

void UnLock(BPTR lock) { (void)lock; }

BPTR Open(const char *name, LONG mode)
{
	const char *m;

	switch (mode) {
	case MODE_OLDFILE: m = "r"; break;
	case MODE_NEWFILE: m = "w"; break;
	case MODE_READWRITE: m = "a+"; break; /* creates, never truncates */
	default: return 0;
	}
	return file_fh(fopen(name, m));
}

LONG Close(BPTR fh)
{
	if (fh == 0) {
		stub_close_zero_calls++;
		return DOSTRUE;
	}
	fclose(fh_file(fh));
	return DOSTRUE;
}

char *FGets(BPTR fh, char *buf, ULONG len)
{
	return fgets(buf, (int)len, fh_file(fh));
}

LONG FPrintf(BPTR fh, const char *fmt, ...)
{
	va_list ap;
	int n;

	va_start(ap, fmt);
	n = vfprintf(fh_file(fh), fmt, ap);
	va_end(ap);
	return n;
}

LONG DeleteFile(const char *name) { return remove(name) == 0 ? DOSTRUE : DOSFALSE; }

/* AmigaDOS Rename() fails if the destination already exists. */
LONG Rename(const char *oldname, const char *newname)
{
	if (access(newname, F_OK) == 0) return DOSFALSE;
	return rename(oldname, newname) == 0 ? DOSTRUE : DOSFALSE;
}

BPTR CurrentDir(BPTR lock) { return lock; }
struct RDArgs *ReadArgs(const char *t, LONG *a, struct RDArgs *r) { (void)t; (void)a; (void)r; return NULL; }
void FreeArgs(struct RDArgs *args) { (void)args; }

/*
 * AmigaDOS requires the token buffer to hold 2 * strlen(src) + 2 bytes and
 * returns -1 otherwise; that check is reproduced. The "token" is just the
 * source pattern, matched by MatchPatternNoCase below.
 */
LONG ParsePatternNoCase(const char *src, char *dst, LONG len)
{
	size_t need = 2 * strlen(src) + 2;

	if ((size_t)len < need) return -1;
	strcpy(dst, src);
	return strpbrk(src, "#?[(|~%") != NULL ? 1 : 0;
}

/*
 * Covers the subset of AmigaDOS patterns the tests use: #? (any string),
 * ? (any character), [] classes, everything else literal.
 */
BOOL MatchPatternNoCase(const char *pat, const char *str)
{
	char *glob = malloc(2 * strlen(pat) + 1);
	char *g = glob;
	int rc;

	while (*pat) {
		if (pat[0] == '#' && pat[1] == '?') {
			*g++ = '*';
			pat += 2;
		} else {
			if (*pat == '*' || *pat == '\\') *g++ = '\\';
			*g++ = *pat++;
		}
	}
	*g = 0;
	rc = fnmatch(glob, str, FNM_CASEFOLD);
	free(glob);
	return rc == 0;
}

struct DiskObject *GetDiskObject(const char *name) { (void)name; return NULL; }
void FreeDiskObject(struct DiskObject *obj) { (void)obj; }
char *FindToolType(char **tt, const char *name) { (void)tt; (void)name; return NULL; }

int strnicmp(const char *a, const char *b, size_t n) { return strncasecmp(a, b, n); }

/* SAS/C strins(): insert `from` at the front of `to`. */
void strins(char *to, const char *from)
{
	size_t fl = strlen(from);

	memmove(to + fl, to, strlen(to) + 1);
	memcpy(to, from, fl);
}

/* SAS/C mktemp() replaces the X characters wherever they are. */
char *amiga_mktemp(char *tmpl)
{
	static unsigned counter;
	unsigned n = (unsigned)getpid() * 1000u + counter++;
	char *p;

	for (p = tmpl; *p; p++) {
		if (*p == 'X') {
			*p = (char)('0' + n % 10);
			n /= 10;
		}
	}
	return tmpl;
}
