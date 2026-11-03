#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messagebase.h"
#include "user.h"

#define DEFAULT_USER "mhunter"
#define DEFAULT_HOME "Workbench:Internet/usr/mhunter"

// Dest must hold 255+1 bytes.
char *GetEnvUser(char *Dest)
{
   char *ptr;

   ptr = getenv("USER");
   if ((ptr == NULL) || (*ptr == 0)) ptr = DEFAULT_USER;
   strncpy(Dest, ptr, 255);
   Dest[255] = 0;
   return(Dest);
}

// Returns a pointer to the provided string storage space,
// which now contains the home directory specified in the
// global environment -- _always_ with a trailing '/' or ':'.
// Dest must hold 255+1 bytes.

char *GetEnvHomeDir(char *Dest)
{
   char *ptr;
   size_t n;

   ptr = getenv("HOME");
   if ((ptr == NULL) || (*ptr == 0)) ptr = DEFAULT_HOME;
   strncpy(Dest, ptr, 254);   // leave room for the '/'
   Dest[254] = 0;

   n = strlen(Dest);
   if ((Dest[n-1] != ':') && (Dest[n-1] != '/')) strcat(Dest, "/");
   return(Dest);
}

// Dummy function to get default only
char *GetNNTPSpoolDir(char *Dest, char *User)
{
   GetEnvHomeDir(Dest);
   strcat(Dest, "NNTPSpool/");
   return(Dest);
}

// Function to add a signature to a message
// Will eventually take user preferences into account
// At present only takes from $HOME/.signature

BOOL AddSig(MsgIndexType *msg, char User[255+1])
{
   FILE *sig;
   BodyType *Body;
   char sigfile[255+1+10], buff[100+1];
   size_t n;

   GetEnvHomeDir(sigfile);
   strcat(sigfile, ".signature");

   if (msg->Body == NULL) return(FALSE);
   for (Body = msg->Body; ; Body = Body->next)
   {
      if (strncmp(Body->Line, "-- ", 3) == 0) return(TRUE);
      if (Body->next == NULL) break;
   }

   if ((sig = fopen(sigfile, "r")) == NULL)
   {
      // Could not open sigfile
      return(FALSE);
   }

   sprintf(buff, "\n-- ");
   Body = AddBodyAfter(Body, buff);

   while ((Body != NULL) && (fgets(buff, sizeof(buff), sig) != NULL))
   {
      n = strlen(buff);
      if ((n > 0) && (buff[n-1] == '\n')) buff[n-1] = 0;
      Body = AddBodyAfter(Body, buff);
   }
   fclose(sig);
   return(Body != NULL);
}


