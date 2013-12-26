#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>
#include <stdlib.h>
#include <sys/types.h>
#include <io.h>
#include <fcntl.h>
#include <getopt.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <share.h>
#include <signal.h>
#include <time.h>

int IsSpace( char Ch);
int IsEmpty (char * Str);
void DelEndSpaces( char * Str);
void DelBegSpaces( char * Str);
void DelSpaces (char * Str);
int IsCRLF (char Ch);
void DelEndCRLF (char * Str);
unsigned char cDOS2KOI (unsigned char Ch);

struct Pwd
 { char User[19];
   char Pass[19];
 };

enum Status 
 { None=0,
   Unreaded=1,
   Readed=2,
   Old=4,
   ROld=6
 };

struct Letter
 { long Begin;
   long End;
   long Size;
   char Deleted;
   char Is866;
   long cBeg;
   long cEnd;
   char * msgID;
   char noID;

   int Lines;

   enum Status Status;
   enum Status oStatus;
   long sBeg;
   long sEnd;
   long hEnd;

   char NoCharset;

/*
   long lineFirst;
   long lineLast;
   long lineEndHead;
   long lineSubject;
   long lineContent;
*/
 };

int ReadUser (struct Pwd * uPw, char * Passwd);

int SkGetS (int Sk, char * Buf);
int WaitConn(void);

int DoCommand(int Socket);

extern int Debug;

int CopyBlock (FILE * fFrom, FILE * fTo, long From, long To);
int AddBlock (FILE * fTo, char * Str);

extern char MyName[];
extern char TimeStamp[];



extern int Debug;

extern struct Letter  *mBox;
extern long msgCount;
extern long TotalLen;
extern long uMsgCount;
extern long uTotalLen;
extern char PwdPath[];
extern char BoxPath[];

void PrintTime (void);
void ePrintTime (void);


int CheckBox (char * rBox, char * tBox);
int RescanBox(FILE * fBox);
int PackBox(char * User, int DoDelete);





