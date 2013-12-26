#include "popper.h"


char MyName[514];



int main (int ArgC, char * ArgV[])
{ /*int Res;
  struct hostent MyHost;
  struct in_addr MyAddr;
  unsigned long * lMyAddr;*/

  printf("vPopper/2. 0.995beta\n(C)'97-99 VR vik@lada.kiev.ua 2:463/262 2:463/278\nPlease mail bug reports to vik@avi.kiev.ua\n");
  printf("Compiled %s at %s\n", __DATE__, __TIME__);

  if((ArgC>1) && (ArgV[1][1]=='?'))
    { fprintf(stderr, "POP3 server/2.\nUsage : %s [mailboxes path [passwd file]]\n", ArgV[0]);
      fprintf(stderr, "Default values : mailbox path = ./\n"
             "                 passwd file  = passwd\n");
      return(0);
    }
  if(ArgC>1)
    { strncpy(BoxPath, ArgV[1], 510);
      BoxPath[510]=0;
      if((BoxPath[strlen(BoxPath)-1]!='\\') && (BoxPath[strlen(BoxPath)-1]!='/'))
        { strcat(BoxPath, "\\");
        }
    }
  if(ArgC>2)
    { strncpy(PwdPath, ArgV[2], 511);
      PwdPath[511]=0;
    }

  MyName[0]=0;
  gethostname(MyName, 511);

/*  memcpy(&MyHost, gethostbyname(MyName), sizeof(MyHost));*/

/*  memcpy(&MyAddr.s_addr, MyHost->h_addr_list[0], MyHost->h_length);*/
/*  fprintf(stderr, "POP3@%s [%s]\n", MyName, inet_ntoa(MyAddr));*/
  fprintf(stderr, "POP3@%s\n", MyName);
  fprintf(stderr, "Using [%s] as passwd\nand   [%s] as mail/boxes\n", PwdPath, BoxPath);

  fprintf(stderr, "Initializing\n");
  fflush(stdout);
  fflush(stderr);
  WaitConn();

  fprintf(stderr, " \nTerminated\n\n");
  return(0);
}

