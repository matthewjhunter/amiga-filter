// Type and function definitions for Filter

#define DEFAULTDELAY 3

struct Email
{
   char From[256];
   char Sender[256];
   char CC[256];
   char To[256];
   char Subject[256];
   char Newsgroups[256];
   char Dest[256];
   char ReplyTo[256];
   long int Start,EndHead,End;
};

struct ConfigList
{
   char Pattern[256];
   char Action[300];
   struct ConfigList *next;
};

void TakeAction(struct Email, char *);
void FreeConfigList(struct ConfigList *);
void ShowConfigList(struct ConfigList *);
void cleanbuff(char *);
char *CleanUp(char *);
void MoveMsg(struct Email, char *);
char *FindDest(struct Email, char *, struct ConfigList *);
void FilterMail(void);
void BNotify(void);
struct ConfigList *LoadConfigList(char *filename);
BOOL SaveConfigList(char *filename, struct ConfigList *);
