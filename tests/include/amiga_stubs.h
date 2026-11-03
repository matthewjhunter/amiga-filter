/*
 * Minimal host stand-ins for the AmigaOS types and calls Filter uses, so the
 * filtering code can be compiled and tested on a Unix host under
 * AddressSanitizer. Types follow the NDK where it matters (BPTR is a LONG,
 * not a pointer), so mistakes the real compiler would flag show up here too.
 *
 * DOS file handles are host FILE pointers stored in a BPTR.
 */
#ifndef AMIGA_STUBS_H
#define AMIGA_STUBS_H

#include <stddef.h>
#include <stdio.h>

typedef long LONG;
typedef unsigned long ULONG;
typedef short BOOL;
typedef unsigned char UBYTE;
typedef long BPTR;

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif
#define DOSTRUE (-1L)
#define DOSFALSE 0L

#define MEMF_CLEAR (1L << 16)
#define SHARED_LOCK (-2L)
#define EXCLUSIVE_LOCK (-1L)
#define ACCESS_READ SHARED_LOCK
#define ACCESS_WRITE EXCLUSIVE_LOCK
#define MODE_OLDFILE 1005
#define MODE_NEWFILE 1006
#define MODE_READWRITE 1004
#define SIGBREAKF_CTRL_C (1L << 12)
#define NRF_SEND_SIGNAL 2

struct Task { int unused; };

struct NotifyRequest {
	char *nr_Name;
	ULONG nr_Flags;
	union {
		struct {
			struct Task *nr_Task;
			UBYTE nr_SignalNum;
		} nr_Signal;
	} nr_stuff;
};

struct WBArg { BPTR wa_Lock; char *wa_Name; };
struct WBStartup { LONG sm_NumArgs; struct WBArg *sm_ArgList; };
struct DiskObject { char **do_ToolTypes; };
struct RDArgs;
struct AppMessage;

/* exec.library */
void *AllocVec(ULONG size, ULONG flags);
void FreeVec(void *mem);
LONG AllocSignal(LONG num);
void FreeSignal(LONG num);
ULONG Wait(ULONG mask);
void Signal(struct Task *task, ULONG mask);
struct Task *FindTask(const char *name);

/* dos.library */
BOOL StartNotify(struct NotifyRequest *nr);
void EndNotify(struct NotifyRequest *nr);
void Delay(LONG ticks);
BPTR Lock(const char *name, LONG mode);
void UnLock(BPTR lock);
BPTR Open(const char *name, LONG mode);
LONG Close(BPTR fh);
char *FGets(BPTR fh, char *buf, ULONG len);
LONG FPrintf(BPTR fh, const char *fmt, ...);
LONG DeleteFile(const char *name);
LONG Rename(const char *oldname, const char *newname);
BPTR CurrentDir(BPTR lock);
struct RDArgs *ReadArgs(const char *tmpl, LONG *array, struct RDArgs *args);
void FreeArgs(struct RDArgs *args);
LONG ParsePatternNoCase(const char *src, char *dst, LONG len);
BOOL MatchPatternNoCase(const char *pat, const char *str);

/* icon.library */
struct DiskObject *GetDiskObject(const char *name);
void FreeDiskObject(struct DiskObject *obj);
char *FindToolType(char **toolTypes, const char *name);

/* SAS/C library extras */
int strnicmp(const char *a, const char *b, size_t n);
void strins(char *to, const char *from);
char *amiga_mktemp(char *tmpl);
#define mktemp amiga_mktemp

/* Counters the tests inspect. */
extern int stub_close_zero_calls;

#endif
