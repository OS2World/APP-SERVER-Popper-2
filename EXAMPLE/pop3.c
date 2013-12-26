#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>


int DEBUGGING=0;

int IsSpace( char Ch)
{ switch(Ch)
   { case 0x20 :
     case 0x0D :
     case 0x0A :
     case 0x08 : break;
     default   : return(0);
   }
  return(1);
}

int IsCRLF( char Ch)
{ switch(Ch)
   { case 0x0D :
     case 0x0A : break;
     default   : return(0);
   }
  return(1);
}
void DelEndCRLF (char * Str)
{ int End;

  if(Str[0]==0) return;

  for(End=strlen(Str);((IsCRLF(Str[End-1])==1) && (End>=0));End--);
  Str[End]=0;
}

void DelBegCRLF (char * Str)
{ int Beg, i;

  if(Str[0]==0) return;

  for(Beg=0;IsCRLF(Str[Beg])==1;Beg++);
  for(i=0;Str[i+Beg-1]!=0;i++)
   { Str[i]=Str[i+Beg];
   }
}
void DelCRLF (char * Str)
{ DelBegCRLF(Str);
  DelEndCRLF(Str);
}

int IsEmpty (char * Str)
{ register int i;
  for(i=0;i<strlen(Str);i++)
   { if(!IsSpace(Str[i])) return(0);
   }
  return(1);
}

void DelEndSpaces( char * Str)
{ int End;

  if(Str[0]==0) return;

  for(End=strlen(Str);((IsSpace(Str[End-1])==1) && (End>=0));End--);
  Str[End]=0;
}

void DelBegSpaces( char * Str)
{ int Beg, i;

  if(Str[0]==0) return;

  for(Beg=0;IsSpace(Str[Beg])==1;Beg++);
  for(i=0;Str[i+Beg-1]!=0;i++)
   { Str[i]=Str[i+Beg];
   }
}
void DelSpaces (char * Str)
{ DelBegSpaces(Str);
  DelEndSpaces(Str);
}


void SplitAddr(char * Me, char * User, char * Domain)
{ int i;
  char fAddr[256];

  strcpy(fAddr, Me);

  for(i=0;fAddr[i]!=0;i++)
   { if(fAddr[i]=='@') break;
   }
  fAddr[i]=0;
  strcpy(User, fAddr);
  strcpy(Domain, &fAddr[i+1]);

}



int IsIP (char * Str)
{ int Res,i;

  for(Res=0,i=0;Str[i]!=0;i++)
   { if(Str[i]=='.') Res++;
   }
  
  if(Res==3) return(1);
  return(0);
}


int GetHost (struct sockaddr_in *Dst, char * Name)
{ unsigned long ip;
  struct hostent *Host;

  memset (Dst, 0, sizeof (*Dst));

  ip = inet_addr(Name);
  if(ip != INADDR_NONE && IsIP(Name))
    { Dst->sin_family = AF_INET;
      Dst->sin_addr.s_addr = ip;
    }
   else
    { Host=gethostbyname(Name);
      if (Host==NULL)
        { perror ("Host not found (gethostbyname)");
          return(-1);
        }
      Dst->sin_family = Host->h_addrtype;
      memcpy (&Dst->sin_addr, Host->h_addr, Host->h_length);
    }

  return(0);
}


unsigned long MakeIP (unsigned char O1, unsigned char O2, unsigned char O3, unsigned char O4)
{ unsigned long Res;

  Res=O4*0x01000000l+O3*0x00010000l+O2*0x00000100l+O1*0x00000001l;

  return(Res);
}

int ReadReplyLine (int Sock, char * Buffer, int Len)
{ int i;
  int Res;

  for(i=0;i<(Len-1);i++)
   { Res=recv(Sock, &Buffer[i], 1, 0);
     if(Res<0)
       { perror("Can't get response");
         return(-1);
       }
     if(Buffer[i]==0x0A) 
       { i--;
         break;
       }
   }
  Buffer[i+1]=0;

  return(0);
}

int ReadReply (int Sock, char * Buffer)
{ int Res;

  Res=ReadReplyLine(Sock, Buffer, 2048);
  if(Res<0)
    { perror("Can't receive data");
      return(-1);
    }
/*  DelEndSpaces(Buffer);*/
  if(DEBUGGING)
    printf("%s\n", Buffer);
  
  return(Buffer[0]=='+'?0:1);
}


int SendCommand (int Sock, char * Command)
{ int Res;
 
  Res=send(Sock, Command, strlen(Command), 0);
  if(Res<0)
    { perror("Can't send data");
      return(-1);
    }
  
  if(DEBUGGING)
    printf("%s", Command);
  return(0);
}


int DoLogin (char * Host)
{ int Sk;
  struct sockaddr_in sAddrFrom, sAddrTo;
  struct servent * servName;
  char Str[3000];
  int i;

  Sk=socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
  if(Sk<0)
    { perror("Can't create control socket");
      return(-1);
    }

  i=0;
  if(setsockopt(Sk, SOL_SOCKET, SO_REUSEADDR, &i, sizeof (i))<0)
    { perror ("Can't set sock option");
      return(-1);
    }

  memset(&sAddrFrom, 0, sizeof(sAddrFrom));
  sAddrFrom.sin_family=AF_INET;
  sAddrFrom.sin_addr.s_addr=INADDR_ANY;
  sAddrFrom.sin_port = 0;
  if(bind(Sk, (struct sockaddr*)&sAddrFrom, sizeof(sAddrFrom))<0)
    { perror("Can't bind socket");
      return(-1);
    }

  GetHost (&sAddrTo, Host);

  servName=getservbyname("pop3", NULL);
  if(servName==NULL)
    { fprintf(stderr, "Warning : unable to get service by name, default used\n");
      sAddrTo.sin_port=htons(110);
    }
   else
    { sAddrTo.sin_port=servName->s_port;
    }

  if(connect(Sk, (struct sockaddr*)&sAddrTo, sizeof(sAddrTo))<0)
    { perror("Can't connect socket");
      return(-1);
    }

  return(Sk);
}


int DoMail (int Sock, char * UName, char * UPass, char * FileName, char * Pop3)
{ FILE * mBox;
  int Res;
  unsigned long msgCount, msgSize, i;
  char Buff   [3000];
  char Tmp    [128];
  char Tmp1   [128];
  int FirstLine;
  time_t Timer;
  

  Res=ReadReply(Sock, Buff);
  if(Res<0) return(Res);
  if(Res!=0)
    { fprintf(stderr, " Can't connect : are we banned?;)\n");
      return(-1);
    }
  
  sprintf(Buff, "USER %s\n", UName);
  SendCommand(Sock, Buff);
  if(ReadReply(Sock, Buff)!=0)
    { fprintf(stderr, " Login failed : user rejected\n");
      return(-1);
    }

  sprintf(Buff, "NOOP\n", UName);
  SendCommand(Sock, Buff);
  if(ReadReply(Sock, Buff)!=0) /*Password required ?*/
    { fprintf(stderr, "Password required for %s\n", UName);
      sprintf(Buff, "PASS %s\n", UPass);
      SendCommand(Sock, Buff);
      if(ReadReply(Sock, Buff)!=0)
        { fprintf(stderr, " Login failed : user rejected\n");
          return(-1);
        }
    }

  sprintf(Buff, "NOOP\n", UName);
  SendCommand(Sock, Buff);
  if(ReadReply(Sock, Buff)!=0) /*Check is we logged in?*/
    { fprintf(stderr, " Login failed : user rejected\n");
      return(-1);
    }

  sprintf(Buff, "STAT\n");
  SendCommand(Sock, Buff);
  if(ReadReply(Sock, Buff)!=0)
    { fprintf(stderr, " Mail transfer failed : can't retrieve mailbox statistic\n");
      return(-1);
    }

  sscanf(Buff,"+OK %lu %lu", &msgCount, &msgSize);

  printf("%lu messages for %s\n", msgCount, UName);


  for(i=1;i<=msgCount;i++)
   { printf("Retrieving message %lu                \x0D", i);
     sprintf(Buff, "RETR %lu\n", i);
     SendCommand(Sock, Buff);
     if(ReadReply(Sock, Buff)!=0)
       { fprintf(stderr, " Mail transfer failed : can't retrieve message                       \n");
         break;
       }
/*     sscanf(Buff, "+OK %lu ", &msgSize);*/

     mBox=fopen(FileName, "a+b");
     if(mBox==NULL)
       { perror("Can't open mailbox");
         return(-1);
       }
     FirstLine=1;

     /*Retrieve message body here*/
     do { if(ReadReply(Sock, Buff)<0)
            { fprintf(stderr, " Mail transfer failed : can't retrieve message body                      \n");
              break;
            }
 
          DelCRLF (Buff);
          if(Buff[0]=='.')
            { if(Buff[1]==0) break;
              Buff[0]=0x0A;
              DelCRLF(Buff);
            }

          if(FirstLine)
            { FirstLine=0;

              time(&Timer);
              strcpy(Tmp1, ctime(&Timer));
              DelSpaces(Tmp1);
              sprintf(Tmp, "From %s %s GMT\x0D\x0A", Pop3, Tmp1);
              fputs(Tmp, mBox);
              sprintf(Tmp, "Received: from %s\x0D\x0A"
                           "          by localhost (GetMail 0.0.2 (C) VR vik@lada.kiev.ua) with POP3\x0D\x0A", Pop3);
              fputs(Tmp, mBox);
              sprintf(Tmp, "          for %s@localhost\x0D\x0A", UName);
              fputs(Tmp, mBox);
              
              memcpy(Tmp, Buff, 120);
              Tmp[120]=0;
              if(memicmp(Tmp, "FROM ", 5)==0) 
                { strcpy(Buff, "Received: from ");
                  DelCRLF(Tmp);
                  strcat(Buff, &Tmp[5]);
                  DelCRLF(Buff);
                } 
            }

          fputs(Buff, mBox);
          fputs("\x0D\x0A", mBox);

        } while(1);
     fputs("\x0D\x0A", mBox);
     fclose(mBox);
     printf("Message retrieved                        \x0D");
     sprintf(Buff, "DELE %lu\n", i);
     SendCommand(Sock, Buff);
     if(ReadReply(Sock, Buff)!=0)
       { fprintf(stderr, " Mail transfer failed : can't delete message                       \n");
         break;
       }
   }

  printf("Done.                                                   \n");

  return(0);
}

int DoLogout(int Sock)
{ char Buff[1024];

  SendCommand(Sock, "QUIT\x0A");
  ReadReply(Sock, Buff);

  close(Sock);

  return(0);
}

int main (int ArgC, char * ArgV[])
{ int Socket;

  if(ArgC<5)
    { printf("Usage : %s <POP3 host> <UserName> <UserPass> <Local Mailbox>\n", ArgV[0]);
      return(1);
    }

  if(ArgC>5) DEBUGGING=1;

  Socket=DoLogin (ArgV[1]);
  if(Socket<0) return(1);

  if(DoMail (Socket, ArgV[2], ArgV[3], ArgV[4], ArgV[1])<0) return(1);

  return(DoLogout(Socket));
}