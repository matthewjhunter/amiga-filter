#ifndef MESSAGEBASE_H
#define MESSAGEBASE_H

#include <exec/types.h>

// Maybe I should combine headers and body lines into one?
// If so, use a structure with a NULL data pointer and a valid next pointer as divider

#define MAX_HEADER_LENGTH 750

struct HeaderType
{
	char Header[MAX_HEADER_LENGTH+1];
	struct HeaderType *next;
};
typedef struct HeaderType HeaderType;

#define MAX_LINE_LENGTH 500

struct BodyType
{
	char Line[MAX_LINE_LENGTH+1];
	struct BodyType *next;
};
typedef struct BodyType BodyType;

// CLength == content-length, may not be necessary
// Local   == TRUE if the message body & complete headers are stored locally

#define MAX_FROM_LENGTH 255
#define MAX_TO_LENGTH 255
#define MAX_SUBJECT_LENGTH 80
#define MAX_DATE_LENGTH 50
#define MAX_MSGID_LENGTH 50

#define MSG_SENT 0
#define MSG_READY 1
#define MSG_HELD 2
#define MSG_EMPTY 3

struct MsgIndexType
{
	char From[MAX_FROM_LENGTH+1];
	char To[MAX_TO_LENGTH+1];
	char Subject[MAX_SUBJECT_LENGTH+1];
	char Date[MAX_DATE_LENGTH+1];
	char MsgID[MAX_MSGID_LENGTH];
	unsigned int CLength;
	BOOL Local;
	int Status;
	HeaderType *Header;
	BodyType *Body;
	struct MsgIndexType *next;
};
typedef struct MsgIndexType MsgIndexType;

// Subject == Subject of the thread, used for group index displays
// NumMsgs == Number of messages in the thread to date
// first   == pointer to the first message in the thread.
// next    == pointer to next thread

struct ThreadIndexType
{
	char Subject[MAX_SUBJECT_LENGTH+1];
	int NumMsgs;
	MsgIndexType *first;
	struct ThreadIndexType *next;
};
typedef struct ThreadIndexType ThreadIndexType;

// Name == name of the group
// NumMsgs == number of currently available messages in the group
// Local == if TRUE, entire group is locally stored
// InterfaceType == the type of interface used for the group
//    0 == LocalOnly: accessible only through this messagebase
//    1 == Internet Email: this group is really a mailbox or mailing list
//    2 == Internet News: Usenet, probably NNTP
//    3 == Fidonet
//    Future types will be supported.

struct GroupIndexType
{
	char Name[255+1];
	int NumMsgs;
	BOOL Local;
	int InterfaceType;
	ThreadIndexType *first;
	struct GroupIndexType *next;
};
typedef struct GroupIndexType GroupIndexType;

HeaderType *FindMsgHeader(MsgIndexType *, char *);
MsgIndexType *FindMsg(GroupIndexType *, char *);
BodyType *AddBodyAfter(BodyType *, char *);
HeaderType *AddHeaderAfter(HeaderType *, char *);
BOOL AddMsgLocal(MsgIndexType *);
BOOL AddMsgIndex(MsgIndexType *);
void ShowMsg(MsgIndexType *);
void InitMsgIndexType(MsgIndexType *);
void FreeMsgIndexType(MsgIndexType *);
BOOL AddBodyLine(MsgIndexType *);
BOOL AddHeader(MsgIndexType *);
void ShowHeaderEntry(HeaderType *);
void ShowBodyEntry(BodyType *);
void ShowMsgIndex(MsgIndexType *);
BOOL CheckDupMsgID(GroupIndexType *group, char *MsgID);
BOOL AddMsgToGroup(MsgIndexType *msg, GroupIndexType *group);
BOOL AddMsgToThread(MsgIndexType *msg, ThreadIndexType *thread);
BOOL NewThread(MsgIndexType *msg, GroupIndexType *group);
#endif
