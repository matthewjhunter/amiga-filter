#ifndef USER_H
#define USER_H 1

#include <exec/types.h>
#include "messagebase.h"

extern char *GetNNTPSpoolDir(char *, char *);
extern char *GetEnvUser(char *);
extern char *GetEnvHomeDir(char *);
extern BOOL AddSig(MsgIndexType *, char *);

#endif

