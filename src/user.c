#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "messagebase.h"
#include "user.h"

#define DEFAULT_USER "mhunter"
#define DEFAULT_HOME "Workbench:Internet/usr/mhunter"

char *GetEnvUser(char *Dest)
{
   char *ptr;

   ptr = getenv("USER");
   if (ptr == NULL) strcpy(Dest, DEFAULT_USER);
   else strcpy(Dest, ptr);
   free(ptr);
   return(Dest);
}

// Returns a pointer to the provided string storage space,
// which now contains the home directory specified in the
// global environment -- _always_ with a trailing '/'.

char *GetEnvHomeDir(char *Dest)
{
   char *ptr;
   char Home[255+1];

   ptr = getenv("HOME");
   if (ptr == NULL) strcpy(Home, DEFAULT_HOME);
   else strcpy(Home, ptr);

   if (Home[strlen(Home)-1] != ':')
   {
      if (Home[strlen(Home)-1] != '/') strcat(Home, "/");
   }
   free(ptr);
   strcpy(Dest, Home);
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
   char sigfile[255+1], buff[100+1];

   GetEnvHomeDir(sigfile);
   strcat(sigfile, ".signature");

   for (Body = msg->Body; (Body->next != NULL); Body = Body->next)
   {
      if (strncmp(Body->Line, "-- ", 3) == 0) return(TRUE);
   }

   if ((sig = fopen(sigfile, "r")) == NULL)
   {
      // Could not open sigfile
      return(FALSE);
   }

   sprintf(buff, "\n-- ");
   Body = AddBodyAfter(Body, buff);

   while (fgets(buff, 500, sig) != NULL)
   {
      buff[strlen(buff)-1] = 0;
      Body = AddBodyAfter(Body, buff);
   }
   return(TRUE);
}


