#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <clib/dos_protos.h>
#include "messagebase.h"

// Dummy functions

BOOL NewThread(MsgIndexType *msg, GroupIndexType *group)
{
	return(TRUE);
}


// Functions for managing a messagebase

// Enforcer hits??
void ShowMsg(MsgIndexType *msg)
{
	HeaderType *Head;
	BodyType *Body;

	printf("-----------------------My Header Information-------------------------\n");
	ShowMsgIndex(msg);
	printf("-----------------------Normal Header Information---------------------\n");

	for (Head = msg->Header; (Head != NULL); Head = Head->next)
	{
		ShowHeaderEntry(Head);
	}
	printf("---------------------------Message Body------------------------------\n");
	for (Body = msg->Body; (Body != NULL); Body = Body->next)
	{
		ShowBodyEntry(Body);
	}
	printf("---------------------------------------------------------------------\n");
}

void InitMsgIndexType(MsgIndexType *msg)
{
	strcpy(msg->From, "Empty");
	strcpy(msg->To, "Empty");
	strcpy(msg->Subject, "Empty");
	strcpy(msg->Date, "Empty");
	msg->Header = NULL;
	msg->Body = NULL;
	msg->CLength = 0;
	msg->Local = FALSE;
	msg->Status = MSG_EMPTY;
	return;
}

void FreeMsgIndexType(MsgIndexType *msg)
{
	BodyType *Body,*Bodynext;
	HeaderType *Header,*Headernext;

	for (Header = msg->Header; Header != NULL; Headernext = Header->next, free(Header), Header = Headernext);
	for (Body = msg->Body; Body != NULL; Bodynext = Body->next, free(Body), Body = Bodynext);

	strcpy(msg->From, "Empty");
	strcpy(msg->To, "Empty");
	strcpy(msg->Subject, "Empty");
	strcpy(msg->Date, "Empty");
	msg->Header = NULL;
	msg->Body = NULL;
	msg->CLength = 0;
	msg->Local = FALSE;
	free(msg);
	return;
}

BOOL AddBodyLineAtEnd(MsgIndexType *msg)
{
	BodyType *Body;

	for (Body = msg->Body; (Body->next != NULL); Body = Body->next);

	if ((Body->next = malloc(sizeof(BodyType))) == NULL)
	{
		// Out of memory
		return(FALSE);
	}
	Body = Body->next;
	Body->Line[0] = 0;
	Body->next = NULL;
	return(TRUE);
}

BodyType *AddBodyAfter(BodyType *Body, char *Line)
{
	BodyType *next;

	if ((next = malloc(sizeof(BodyType))) == NULL)
	{
		// Out of Memory
		return(NULL);
	} else if (Body == NULL)
	{
		// An empty list or listentry
		Body = next;
		strcpy(next->Line, Line);
		next->next = NULL;
	} else if (Body->next != NULL)
	{
		// Insert instead of appending
		next->next = Body->next;
		Body->next = next;
		strcpy(next->Line, Line);
	} else
	{
		Body->next = next;
		strcpy(next->Line, Line);
		next->next = NULL;
	}
	return(next);
}

HeaderType *AddHeaderAfter(HeaderType *Head, char *Header)
{
	HeaderType *next;

	if ((next = malloc(sizeof(HeaderType))) == NULL)
	{
		// Out of Memory
		return(NULL);
	} else if (Head == NULL)
	{
		// An empty list or listentry
		Head = next;
		strcpy(next->Header, Header);
		next->next = NULL;
	} else if (Head->next != NULL)
	{
		// Insert instead of appending
		next->next = Head->next;
		Head->next = next;
		strcpy(next->Header, Header);
	} else
	{
		Head->next = next;
		strcpy(next->Header, Header);
		next->next = NULL;
	}
	return(next);
}

// Need to add detection and handling of multiple headers on one line, ie,
// Newsgroups: comp.sys.amiga.tech,comp.sys.amiga.datacomm

BOOL AddHeaderAtEnd(MsgIndexType *msg)
{
	HeaderType *Head;

	for (Head = msg->Header; (Head->next != NULL); Head = Head->next);

	if ((Head->next = malloc(sizeof(HeaderType))) == NULL)
	{
		// Out of memory
		return(FALSE);
	}
	Head = Head->next;
	Head->Header[0] = 0;
	Head->next = NULL;

	return(TRUE);
}

void ShowHeaderEntry(HeaderType *Head)
{
	printf("Header: %s\n", Head->Header);
	printf("Last: ");
	if (Head->next == NULL)
	{
		printf("TRUE");
	} else printf("FALSE");
	printf("\n");
	return;
}

void ShowBodyEntry(BodyType *Body)
{
	printf("Line: %s\n", Body->Line);
	printf("Last: ");
	if (Body->next == NULL)
	{
		printf("TRUE");
	} else printf("FALSE");
	printf("\n");
	return;
}

void ShowMsgIndex(MsgIndexType *msg)
{
	printf("From: %s\n", msg->From);
	printf("To: %s\n", msg->To);
	printf("Subject: %s\n", msg->Subject);
	printf("Date: %s\n", msg->Date);
	printf("Content-Length: %d\n", msg->CLength);
	if (msg->Local) printf("Local: TRUE\n"); else printf("Local: FALSE\n");
}


