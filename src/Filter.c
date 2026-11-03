#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <exec/types.h>
#include <exec/memory.h>
#include <dos/dos.h>
#include <dos/notify.h>
#include <dos/rdargs.h>
#include <clib/dos_protos.h>
#include <clib/exec_protos.h>
#include <clib/icon_protos.h>
#include <workbench/startup.h>
#include <workbench/workbench.h>
#include <unistd.h>

#include "user.h"

#include "filter.h"
#include "Filter_rev.h"

#define DEFAULTDELAY 3

// Disable SAS/C control-c handling
// int CXBRK(void) { return(0); }
// int chkabort(void) { return(0); }

// Version string
const static char VersTag[] = VERSTAG;

// Variable declarations
char buff[500], CFGName[255],MBoxName[255], Home[255], Default[255], DefDir[255];
int msgc, InHeader, length;
long int temp;
struct NotifyRequest *note;
struct AppMessage *appmsg;
struct Email emsg;
ULONG signals;
long signum;
FILE *mailbox,*config;

BOOL isMailStartLine(char *line);

// Copy a header line into a fixed-size field, always terminated.
static void SetField(char *field, size_t size, const char *line)
{
   strncpy(field, line, size - 1);
   field[size - 1] = 0;
}

// Mark every header field of a message as not seen yet.
static void ResetEmail(struct Email *msg)
{
   strcpy(msg->From, "None");
   strcpy(msg->Sender, "None");
   strcpy(msg->CC, "None");
   strcpy(msg->To, "None");
   strcpy(msg->Subject, "None");
   strcpy(msg->Newsgroups, "None");
   strcpy(msg->Dest, "None");
   strcpy(msg->ReplyTo, "None");
}

// Strip a trailing newline.  Returns TRUE if the line had one, FALSE if
// fgets() stopped early because the line was too long for the buffer.
static BOOL chomp(char *s)
{
   size_t n = strlen(s);

   if ((n > 0) && (s[n-1] == '\n'))
   {
      s[n-1] = 0;
      return(TRUE);
   }
   return(FALSE);
}


// Start notification and intiatialize variables
void main(int argc, char *argv[])
{
   long delay = DEFAULTDELAY;
   // long *delayptr = NULL;
   BOOL once,FromWB;
   char User[255+1];

   // Find HOME directory
   GetEnvHomeDir(Home);

/*
   ptr = getenv("HOME");
   strcpy(Home, ptr);
   if (Home[strlen(Home)-1] != ':')
   {
      if (Home[strlen(Home)-1] != '/') strcat(Home, "/");
   }
   free(ptr);
*/

   strcpy(CFGName, ".filterrc");
   strcpy(MBoxName, "uumail:");

   GetEnvUser(User);
   strcat(MBoxName, User);

   strcpy(Default, Home);
   strcat(Default, "mail/");

   strcat(Default, User);

   strcpy(DefDir, Home);
   strcat(DefDir, "mail/");

   once = FALSE;

#ifdef DEBUG
   printf("Default configuration:\n");
   printf("Home directory is: %s\n", Home);
   printf("Configuration  in: %s\n", CFGName);
   printf("Incoming  Mailbox: %s\n", MBoxName);
   printf("Default   Mailbox: %s\n", Default);
   printf("Default   MailDir: %s\n", DefDir);
   printf("Delay time (secs): %d\n", delay);
   if (once) printf("Run only once.\n");
   else printf("Install in RAM.\n");
#endif

   // Argument parsing
   if (argc == 0)
   {
      // Workbench startup code
      struct WBStartup *wb_startup;
      struct WBArg *wb_arg;
      BPTR olddir;

      FromWB = TRUE;

      wb_startup = (struct WBStartup *)argv;
      wb_arg = wb_startup->sm_ArgList;

#ifdef DEBUG
      printf("Started from Workbench...\n");
      printf("Number of Arguments: %d\n", wb_startup->sm_NumArgs);
      printf("Arguments: %s\n", wb_startup->sm_ArgList->wa_Name);
#endif

      if (0 != wb_arg->wa_Lock)
      {
         struct DiskObject *dobj;
         char **toolarray;
         char *string;

         olddir = CurrentDir(wb_arg->wa_Lock);
         if ((wb_arg->wa_Name) && (dobj=GetDiskObject(wb_arg->wa_Name)))
         {
            // Parsing tooltypes
            toolarray = (char **)dobj->do_ToolTypes;

            if (FindToolType(toolarray,"ONCE") != NULL) once = TRUE;
            if (string = (char *)FindToolType(toolarray,"DELAY")) sscanf(string, "%d", &delay);
            if (string = (char *)FindToolType(toolarray,"INBOX")) strcpy(MBoxName, string);
            if (string = (char *)FindToolType(toolarray,"CONFIG")) strcpy(CFGName, string);
            if (string = (char *)FindToolType(toolarray,"DEFAULT")) strcpy(Default, string);
            if (string = (char *)FindToolType(toolarray,"DEFAULTDIR")) strcpy(DefDir, string);

            FreeDiskObject(dobj);
         } else if (!(*wb_arg->wa_Name))
         {
            printf("Disk or drawer icon??!  Are you nuts?  I can't take that.\n");
         } else
         {
            printf("No icon... I wonder how you managed that.\n");
         }
         CurrentDir(olddir);
      } else
      {
         printf("Locks not supported!  I'm fucked, sorry.\n");
      }

   } else if (argc == 1)
   {
      // CLI startup with no arguments, using defaults
      FromWB = FALSE;
#ifdef DEBUG
      printf("No arguments, using defaults.");
#endif
   } else if (argc > 1)
   {
      // CLI startup code
      struct RDArgs *rdargs;
      char Template[] = {"DELAY/N,INBOX/K,CONFIG/K,DEFAULT/K,DEFAULTDIR,ONCE/S"};
      long args[6] = {0,0,0,0,0,0};

      FromWB = FALSE;
#ifdef DEBUG
      printf("Started from CLI\n");
      printf("Template: %s\n", Template);
#endif

      rdargs = ReadArgs(Template, args, NULL);
      if (rdargs == NULL)
      {
         printf("Error parsing command line!\n");
         FreeArgs(rdargs);
         exit(20);
      }
      if (args[0] != 0)
      {
         delay = *(long *)args[0];
      }
      if (args[1] != 0) strcpy(MBoxName, (char *)args[1]);
      if (args[2] != 0) strcpy(CFGName, (char *)args[2]);
      if (args[3] != 0) strcpy(Default, (char *)args[3]);
      if (args[4] != 0) strcpy(DefDir, (char *)args[4]);
      if (args[5] != 0) once = TRUE;
      FreeArgs(rdargs);
   }

   if (!FromWB) printf(VSTRING);

#ifdef DEBUG
   printf("Configuration after args:\n");
   printf("Home directory is: %s\n", Home);
   printf("Configuration  in: %s\n", CFGName);
   printf("Incoming  Mailbox: %s\n", MBoxName);
   printf("Default   Mailbox: %s\n", Default);
   printf("Default   MailDir: %s\n", DefDir);
   printf("Delay time (secs): %d\n", delay);
   if (once) printf("Run only once.\n");
   else printf("Install in RAM.\n");
#endif
#ifndef DEBUG
   if ((!FromWB) && (!once)) printf("Watching file: %s\n", MBoxName);
#endif

   if (once == TRUE)
   {
      FilterMail();
   } else do
   {
      BNotify();
      signals = Wait((1L << signum) | SIGBREAKF_CTRL_C);
      if (signals & (1L << signum))
      {
         if (!FromWB) printf("Received notify.\n");
         EndNotify(note);
         FreeSignal(signum);
         FreeVec(note);
         Delay(delay * 50);
         FilterMail();
      } else
      if (signals & SIGBREAKF_CTRL_C)
      {
         EndNotify(note);
         FreeSignal(signum);
         FreeVec(note);
         if (!FromWB) printf("Exiting with no errors.\n");
         exit(0);
      }
   } while (TRUE);
}

void FilterMail(void)
{
   BPTR file = 0;
   long int checkfile;
   int count;
   BOOL whole, prevwhole = TRUE;
   struct ConfigList *first;

   for (count = 0; count <= 100; count++)
   {
      if ((file = Lock(MBoxName, ACCESS_WRITE)) != 0) break;
      Delay(300);
      if (count == 100) return;
   }

   if (file == 0) return;
   UnLock(file);

   if ((mailbox = fopen(MBoxName, "r")) == NULL)
   {
      fprintf(stderr,"Could not open mailbox.\n");
      exit(20);
   }
   ResetEmail(&emsg);

   emsg.Start = 0;
   emsg.EndHead = 0;
   emsg.End = 0;

   fseek(mailbox, 0, SEEK_END);
   checkfile = ftell(mailbox);
   fseek(mailbox, 0, SEEK_SET);

   if ((checkfile - ftell(mailbox)) <= 2) return;

   if ((first = LoadConfigList(CFGName)) == NULL)
   {
      strcpy(CFGName, Home);
      strcat(CFGName, ".filterrc");
      if ((first = LoadConfigList(CFGName)) == NULL)
      {
         strcpy(CFGName, "uulib:");
         strcat(CFGName, ".filterrc");
         if ((first = LoadConfigList(CFGName)) == NULL)
         {
            fprintf(stderr, "Error opening config file!\n");
            fprintf(stderr, "Either the file does not exist, or there is not enough memory to load it.\n");
            exit(20);
         }
      }
   }

#ifdef DEBUG
   ShowConfigList(first);
#endif

   msgc = 0;
   length = 0;
   InHeader = FALSE;

   for (;;)
   {
      temp = ftell(mailbox);
      if (fgets(buff, sizeof(buff), mailbox) == NULL) break;
      whole = chomp(buff);
      if (!prevwhole)
      {
         // Rest of an over-long line: not a header or a message start
         prevwhole = whole;
         length = 1;
         emsg.End = ftell(mailbox);
         continue;
      }
      prevwhole = whole;

#ifdef DEBUG
      puts(buff);
#endif
      if ((strncmp(buff,"From ", 5)==0) && (isMailStartLine(buff)) && (length == 0))
      {
         if (emsg.EndHead != 0)
         {
            strcpy(emsg.Dest, FindDest(emsg,Default,first));
            TakeAction(emsg,DefDir);
            ResetEmail(&emsg);
         }
         msgc++;
         emsg.Start = temp;
         InHeader = TRUE;
#ifdef DEBUG
         printf("---------------------------------------------------------------------\n");
#endif
      }
      if (InHeader == TRUE)
      {
         if (strncmp(buff, "Sender: ", 8) == 0)
         {
            SetField(emsg.Sender, sizeof(emsg.Sender), buff);
         } else
         if (strncmp(buff, "From: ", 6) == 0)
         {
            SetField(emsg.From, sizeof(emsg.From), buff);
         } else
         if (strncmp(buff, "To: ", 4) == 0)
         {
            SetField(emsg.To, sizeof(emsg.To), buff);
         } else
         if (strnicmp(buff, "cc: ", 4) == 0)
         {
            SetField(emsg.CC, sizeof(emsg.CC), buff);
         } else
         if (strnicmp(buff, "Subject: ", 9) == 0)
         {
            SetField(emsg.Subject, sizeof(emsg.Subject), buff);
         } else
         if (strnicmp(buff, "Newsgroups: ", 12) == 0)
         {
            SetField(emsg.Newsgroups, sizeof(emsg.Newsgroups), buff);
         } else
         if (strnicmp(buff, "Reply-To: ", 10) == 0)
         {
            SetField(emsg.ReplyTo, sizeof(emsg.ReplyTo), buff);
         } else
         if (strlen(buff) == 0)
         {
            InHeader = FALSE;
            emsg.EndHead = temp;
#ifdef DEBUG
            printf("---------------------------------------------------------------------\n");
            printf("%s\n%s\n%s\n%s\n%s\n", emsg.Sender, emsg.From, emsg.To, emsg.CC, emsg.Subject);
            printf("---------------------------------------------------------------------\n");
#endif
         }
      }
      length = strlen(buff);
      emsg.End = ftell(mailbox);
   }
   strcpy(emsg.Dest, FindDest(emsg,Default,first));
   TakeAction(emsg,DefDir);
   fclose(mailbox);
// if (!FromWB) printf("Processed %d messages.\n", msgc);
   if ((mailbox = fopen(MBoxName, "w")) == NULL)
   {
      fprintf(stderr, "Failed to overwrite mailbox file.\n");
      exit(20);
   }
   fclose(mailbox);
   FreeConfigList(first);
}

void MoveMsg(struct Email msg, char *DDir)
{
   /* Transfer msgs from inbox to Dest */
   int counter;
   char outname[80];
   long int msglength, temp;
   FILE *outfile;

   temp = ftell(mailbox);
   fseek(mailbox, msg.Start, SEEK_SET);
   msglength = (msg.End - msg.Start);
   if (msglength == 0) return;

#ifdef DEBUG
   printf("The message is %d bytes long.\n",msglength);
   printf("Default directory is: %s\n",DDir);
   printf("Delivering message to: %s\n",msg.Dest);
#endif

   if ((strstr(msg.Dest, ":") == NULL) && (strstr(msg.Dest, "/") == NULL))
   {
      strins(strcpy(outname, msg.Dest), DDir);
   } else strcpy(outname, msg.Dest);
   if ((outfile = fopen(outname, "a")) == NULL)
   {
      fprintf(stderr, "Error opening %s, exiting.\n", outname);
      exit(20);
   }
//   if (!FromWB) printf("Writing message to %s\n", outname);
//   fprintf(outfile, "\n"); Removed leading linefeed; hopefully won't have problems

   for (counter = 0; counter < msglength; counter++)
   {
      fputc(fgetc(mailbox), outfile);
   }
   fclose(outfile);

   fseek(mailbox,temp,SEEK_SET);
}

char *FindDest(struct Email msg, char *Default, struct ConfigList *first)
{
   struct ConfigList *curr;

#ifdef DEBUG
   printf("Entering message delivery routine.\n");
   printf("Available information:\n");
   printf("%s\n%s\n%s\n%s\n%s\n",msg.Sender,msg.From,msg.To,msg.CC,msg.Subject);
   printf("Current Destination: %s\n",msg.Dest);
#endif
   for (curr = first; (curr != NULL); curr = curr->next)
   {
      char tokbuf[500];

      if (strlen(curr->Pattern) == 0) break;
      if (strlen(curr->Action) == 0) break;
      if (ParsePatternNoCase(curr->Pattern, tokbuf, 499) == -1)
      {
         fprintf(stderr, "Pattern: %s\nDest: %s\n", curr->Pattern, curr->Action);
         fprintf(stderr, "Pattern parse failure in filter.cfg\n");
         exit(20);
      }
      if (strncmp(msg.Dest,"None",4) == 0)
      {
         if (MatchPatternNoCase(tokbuf, msg.Sender))
         {
            return(curr->Action);
         }
         if (MatchPatternNoCase(tokbuf, msg.From))
         {
            return(curr->Action);
         }
         if (MatchPatternNoCase(tokbuf, msg.To))
         {
            return(curr->Action);
         }
         if (MatchPatternNoCase(tokbuf, msg.CC))
         {
            return(curr->Action);
         }
         if (MatchPatternNoCase(tokbuf, msg.Subject))
         {
            return(curr->Action);
         }
         if (MatchPatternNoCase(tokbuf, msg.Newsgroups))
         {
            return(curr->Action);
         }
      }
   }
#ifdef DEBUG
   printf("No match found, using default of %s\n", Default);
#endif
   return(Default);
}

void TakeAction(struct Email msg, char *DefDir)
{
   if (strncmp(msg.Dest, "DELETE",7) == 0) return;
   else if (strncmp(msg.Dest, "FORWARD:",8) == 0)
   {
      char TempName[255],NewDest[255],TempTwoName[255],Command[255],*str;
      BPTR temp1,temp2;

      sprintf(TempName, "T:XXXXXXXfilter.temp");
      mktemp(TempName);

      str = &msg.Dest[8];
      strcpy(NewDest, str);

      if ((temp1 = Open(TempName, MODE_NEWFILE)) != 0)
      {
         char buff[500];
         FPrintf(temp1, "To: %s\n", NewDest);
         FPrintf(temp1, "Subject: Forwarded Message\n\n");
         FPrintf(temp1, "****************** Forwarded Message follows ********************\n");
         sprintf(TempTwoName, "T:XXXXXXXfilter.temp");
         mktemp(TempTwoName);
         strcpy(msg.Dest, TempTwoName);
         MoveMsg(msg,DefDir);
         if ((temp2 = Open(TempTwoName, MODE_OLDFILE)) != 0)
         {
            while(FGets(temp2,buff,500) != NULL)
            {
               buff[strlen(buff) - 1] = 0;
               FPrintf(temp1, "> %s\n", buff);
            }
         }
         Close(temp2);
         FPrintf(temp1, "\n***************** End of Forwarded Message ***********************\n");
      }
      Close(temp1);
      strcpy(Command, "smtppost ");
      strcat(Command, TempName);
      system(Command);
      DeleteFile(TempName);
      DeleteFile(TempTwoName);
   }
   else if (strncmp(msg.Dest, "COMMAND:",8) == 0)
   {
      char NewDest[255], Command[255], TempName[255], *str;

      str = &msg.Dest[8];
      strcpy(NewDest, str);
      sprintf(TempName, "T:XXXXXXXfilter.temp");
      mktemp(TempName);

      strcpy(msg.Dest, TempName);
      MoveMsg(msg, DefDir);
      strcpy(Command, NewDest);
      strcat(Command, " ");
      strcat(Command, TempName);
      system(Command);
      DeleteFile(TempName);
   }
   else if (strstr(msg.Dest,",") != NULL)
   {
      char *str,*oldstr,NewDest[255];
      strcpy(NewDest, msg.Dest);
      str = NewDest;
      oldstr = NewDest;
      while ((str = strstr(str, ",")) != NULL)
      {
         str[0] = 0;
         str = &str[1];
         strcpy(msg.Dest, oldstr);
         MoveMsg(msg, DefDir);
         oldstr = str;
      }
      strcpy(msg.Dest, oldstr);
      MoveMsg(msg, DefDir);
   } else MoveMsg(msg, DefDir);
}

void BNotify(void)
{
   if (note = AllocVec(sizeof(struct NotifyRequest), MEMF_CLEAR))
   {
      if ((signum = AllocSignal(-1L)) != -1)
      {
         note->nr_Name = MBoxName;
         note->nr_Flags = NRF_SEND_SIGNAL;
         note->nr_stuff.nr_Signal.nr_Task = (struct Task *)FindTask(NULL);
         note->nr_stuff.nr_Signal.nr_SignalNum = signum;
         if ((StartNotify(note)) != DOSTRUE)
         {
            printf("Notify failed, exiting.\n");
            FreeSignal(signum);
            FreeVec(note);
            exit(20);
         }
      } else
      {
         printf("Notify failed, exiting.\n");
         FreeSignal(signum);
         FreeVec(note);
         exit(20);
      }
   } else
   {
      printf("Notify failed, exiting.\n");
      FreeVec(note);
      exit(20);
   }
}

// Load the specified configuration file into memory

struct ConfigList *LoadConfigList(char *CFGName)
{
   BPTR config;
   char buff[500];
   struct ConfigList *curr,*first;

   first = malloc(sizeof(struct ConfigList));
   first->Pattern[0] = 0;
   first->Action[0] = 0;
   first->next = NULL;
   curr = first;

   if ((config = Open(CFGName, MODE_OLDFILE)) != 0)
   {
      while(FGets(config, buff, 500))
      {
         cleanbuff(buff);
         if (strlen(buff) > 1)
         {
            if (curr->Pattern[0] == 0)
            {
#ifdef DEBUG
               printf("Loaded pattern: %s\n", buff);
#endif
               strcpy(curr->Pattern, buff);
            }
            else if (curr->Action[0] == 0)
            {
#ifdef DEBUG
               printf("Loaded action: %s\n", buff);
#endif
               strcpy(curr->Action, buff);
            }
            else
            {
               curr->next = malloc(sizeof(struct ConfigList));
               curr = curr->next;
               curr->Pattern[0] = 0;
               curr->Action[0] = 0;
               curr->next = NULL;
               strcpy(curr->Pattern, buff);
            }
         } else continue;
      }
   } else return(NULL);
   Close(config);
   return(first);
}
/*
 Requires a null-terminated line.
 */

BOOL isMailStartLine(char *line)
{
   if (strncmp(line, "From ", 5) == 0) 
   {
      if (strstr(line, "Mon") != NULL) return TRUE;
      else if (strstr(line, "Tue") != NULL) return TRUE;
      else if (strstr(line, "Wed") != NULL) return TRUE;
      else if (strstr(line, "Thu") != NULL) return TRUE;
      else if (strstr(line, "Fri") != NULL) return TRUE;
      else if (strstr(line, "Sat") != NULL) return TRUE;
      else if (strstr(line, "Sun") != NULL) return TRUE;
      else if (strstr(line, "Jan") != NULL) return TRUE;
      else if (strstr(line, "Feb") != NULL) return TRUE;
      else if (strstr(line, "Mar") != NULL) return TRUE;
      else if (strstr(line, "Apr") != NULL) return TRUE;
      else if (strstr(line, "May") != NULL) return TRUE;
      else if (strstr(line, "Jun") != NULL) return TRUE;
      else if (strstr(line, "Jul") != NULL) return TRUE;
      else if (strstr(line, "Aug") != NULL) return TRUE;
      else if (strstr(line, "Sep") != NULL) return TRUE;
      else if (strstr(line, "Oct") != NULL) return TRUE;
      else if (strstr(line, "Nov") != NULL) return TRUE;
      else if (strstr(line, "Dec") != NULL) return TRUE;
      else return FALSE;
   } else return FALSE;
} 

// Save the specified configlist into the specified file

BOOL SaveConfigList(char *filename, struct ConfigList *first)
{
   FILE *config;
   struct ConfigList *curr;

   if ((config = fopen(filename, "w")) == NULL)
   {
      // Could not open file
#ifdef DEBUG
      printf("Could not open %s to save config\n", filename);
#endif
      return(FALSE);
   }

   for (curr = first; (curr != NULL); curr = curr->next)
   {
      fprintf(config, "%s\n%s\n", curr->Pattern, curr->Action);
   }

   fclose(config);
}

// Free the memory associated with a configlist

void FreeConfigList(struct ConfigList *first)
{
   struct ConfigList *next,*curr;

   for (curr = first; (curr != NULL); next = curr->next, free(curr), curr = next);
}

// Show a config list; for testing

void ShowConfigList(struct ConfigList *first)
{
   struct ConfigList *curr;

   for (curr = first; (curr != NULL); curr = curr->next)
   {
      puts(curr->Pattern);
      puts(curr->Action);
   }
}

// Removes the EOL character, comments, and trailing blanks
void cleanbuff(char *buff)
{
   char *temp;
   size_t n;

   chomp(buff);
   temp = strchr(buff, ';');
   if (temp != NULL) temp[0] = 0;
   n = strlen(buff);
   while ((n > 0) && ((buff[n-1] == ' ') || (buff[n-1] == '\t'))) buff[--n] = 0;
}
