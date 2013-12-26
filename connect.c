#include "popper.h"


char TimeStamp[1024];


void ChildDie (int SigNo)
{ int PID, t;

  if(SigNo==SIGCHLD)
    { PID=wait(&t);
      signal(SIGCHLD, SIG_ACK);
    }
}

int WaitConn(void)
{ int Sk, clSk;
  int i;
  struct sockaddr_in SAddr, Client;
  struct servent * pop3Port;
  struct hostent * ClAddr;
  char Host[128];
/*  time_t Time;*/
  int Res;
  

  Sk=socket(AF_INET, SOCK_STREAM, 0);
  if(Sk<0)
    { perror("Can't create socket");
      return(-1);
    }
  i=1;
  if(setsockopt(Sk, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i))!=0)
    { perror("Can't set socket options");
      return(-1);
    }

  pop3Port=getservbyname("pop3", NULL);
  if(pop3Port==NULL)
    { perror("pop3 : unknown service");
      return(-1);
    }
  memset(&SAddr, 0, sizeof(struct sockaddr_in));
  SAddr.sin_port=pop3Port->s_port;
  SAddr.sin_family=AF_INET;
  SAddr.sin_addr.s_addr=INADDR_ANY;
  if(bind(Sk, (struct sockaddr*) &SAddr, sizeof(struct sockaddr_in))!=0)
    { perror("Can't bind socket");
      return(-1);
    }

  if(listen(Sk, 5)!=0)
    { perror("Can't set listening socket");
      return(-1);
    }

  printf("Accepting connections on port %u\n", ntohs(SAddr.sin_port));

  signal(SIGCHLD, ChildDie);

  do { 
       do { i=sizeof(struct sockaddr_in);
            clSk=accept(Sk, (struct sockaddr*)&Client, &i);
          } while((clSk<0) && (errno==EINTR));
       if(clSk<0)
         { /*perror("Can't accept incoming connection");*/
           continue;
         }
       ClAddr=gethostbyaddr((char*)&Client.sin_addr.s_addr, sizeof(Client.sin_addr.s_addr), AF_INET);
       Host[0]=0;
       if(ClAddr!=NULL)
         { strncpy(Host, ClAddr->h_name, 120);
           Host[120]=0;
           ePrintTime();
           fprintf(stderr, "Incoming connection from %s [%s]\n", Host, inet_ntoa(Client.sin_addr));
         }

       fflush(stdout);
       fflush(stderr);
       Res=fork();
       if(Res<0)
         { perror("Panic : can't start child process ");
           close(clSk);
           continue;
         }
       if(Res==0)
         { /*New connection - process it*/

           if(close(Sk)!=0)
             { perror("Can't close master socket (child)");
               return(-1);
             }
           DoCommand(clSk);
           ePrintTime();
           fprintf(stderr, "%s POP3 session ends.\n", Host);
           exit(0);
         }
        else
         { if(close(clSk)!=0)
             { perror("Can't close child socket (parent)");
               return(-1);
             }
         }
     }while(1);

  return(0);
}


int SkGetS (int Sk, char * Buf)
{ int i=0;

  i=Buf[0]=0;
  do { if(read(Sk, &Buf[i], 1)!=1) return(-1);
       if(i>2040) return(-1);
       if(Buf[i]==0x0A)
         { Buf[i]=0;
           break;
         }
/*       fprintf(stderr, "[%c][%2.2X]\n", (unsigned int)Buf[i], (unsigned int)Buf[i]);*/
       i++;
     } while(1);

/*  fprintf(stderr, "\n");*/
  DelEndCRLF(Buf);
  ePrintTime();
  fprintf(stderr, "%s\n", Buf);
  return(0);
}

