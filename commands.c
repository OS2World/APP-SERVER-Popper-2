#include "popper.h"

int Debug=0;

struct Letter  *mBox=NULL;
long msgCount=0;
long TotalLen=0;
long uMsgCount=0;
long uTotalLen=0;
char PwdPath[512]="passwd";
char BoxPath[512]=".\\";

static char UserName[9]={0};
void CrashPack (void)
{ fprintf(stderr, "Session crashed for user %s, emergency packing mailbox\n", UserName);
  PackBox(UserName, 0);
}

int CheckUser(char * UserName)
{ struct Pwd Pw;

  strncpy(Pw.User, UserName, 16);
  Pw.User[16]=0;

  if(ReadUser(&Pw, PwdPath)!=0) return(0);
  if(Pw.Pass[0]=='*') return(0);

  return(1);
}
int CheckPass(char * User, char * Pass)
{ struct Pwd Pw;

  strncpy(Pw.User, User, 8);
  Pw.User[16]=0;

  if(ReadUser(&Pw, PwdPath)!=0) return(0);
  if(Pw.Pass[0]=='*') return(0);
  if(strcmp(Pw.Pass, Pass)==0) return(1);

  return(0);
}

int CheckBox (char * rBox, char * tBox)
{ long Len;
  FILE *rFile, *tFile;

  if(access(tBox, 0)!=0) return(0);

/*move tFile(tBox) to rFile(rBox)*/

  tFile=_fsopen(tBox, "rb", SH_DENYRW);
  if(tFile==NULL)
    { perror("Can't open old temporary maildrop");
      return(-1);
    }

  rFile=_fsopen(rBox, "a+b", SH_DENYRW);
  if(rFile==NULL)
    { perror("Can't open maildrop");
      fclose(tFile);
      return(-1);
    }

  fseek(rFile, 0, SEEK_END);
  fseek(tFile, 0, SEEK_END);
  Len=ftell(tFile);
  fseek(tFile, 0, SEEK_SET);
  if(Len<0)
    { perror("Can't read temporary maildrop");
      return(-1);
    }
      
/*Appending from fFrom to end of fTo beginning from fFrom[From] to fFrom[To]*/
/*int CopyBlock (FILE * fFrom, FILE * fTo, long From, long To)*/
  if(CopyBlock(tFile, rFile, 0l, Len)!=0)
    { perror("Can't append temporary mailbox to main maildrop : some mails wasted");
    }
  fclose(tFile);
  fclose(rFile);
  unlink(tBox);

  return(0);
}


/***************************************************************/

#define USERCMD "USER"
#define PASSCMD "PASS"
#define QUITCMD "QUIT"
#define NOCMD   "NOOP"
#define STATCMD "STAT"
#define LISTCMD "LIST"
#define RETRCMD "RETR"
#define DELECMD "DELE"
#define LASTCMD "LAST"
#define RSETCMD "RSET"
#define TOPCMD  "TOP"
#define UIDLCMD "UIDL"

int DoCommand(int Socket)
{ char * Buffer;
  char * Buffer2;
  int Res;
  char Password[19]={0};
  FILE * fBox=NULL;
  register unsigned long i;
  long MsgNo, LastMsg=0;
  char Ch;
  int WasLF=0;
  int WasCR=0;
  int sRes;
  signed int Lines;
  char * Arg1;
  char * Arg2;
  int LoggedIn=0;

  Buffer=(char*)malloc(2048);
  if(Buffer==NULL) return(-1);

  Buffer2=(char*)malloc(2048);
  if(Buffer2==NULL)
    { free(Buffer);
      return(-1);
    }

  sprintf(Buffer2, "+OK POP3 server glad to meet you here ;)\x0D\x0A"/*TimeStamp*/);
  sRes=write(Socket, Buffer2, strlen(Buffer2));
  if(sRes<0)
    { free(Buffer);
      free(Buffer2);
      return(-1);
    }





/*Aut.*/
  memset(UserName, 0, 9);
  do {

/*Waiting for USER command*/
       do { sRes=SkGetS(Socket, Buffer);
            if(sRes!=0)
              { free(Buffer);
                free(Buffer2);
                if(fBox) fclose(fBox);
                return(-1);
              }
            DelEndCRLF(Buffer);
            DelSpaces(Buffer);

            if(memicmp(Buffer, USERCMD, strlen(USERCMD))==0)
              { DelSpaces(&Buffer[strlen(USERCMD)]);
                strncpy(UserName, &Buffer[strlen(USERCMD)], 16);
                UserName[16]=0;
                if(CheckUser(UserName))
                  { sprintf(Buffer2, "+OK Hello, %s! Please remember your password!\x0D\x0A", UserName);
                    sRes=write(Socket, Buffer2, strlen(Buffer2));
                    if(sRes<0)
                      { free(Buffer);
                        free(Buffer2);
                        return(-1);
                      }
                  }
                 else
                  { sprintf(Buffer2, "-ERR Don't know who is %s\x0D\x0A", UserName);
                    sRes=write(Socket, Buffer2, strlen(Buffer2));
                    if(sRes<0)
                      { free(Buffer);
                        free(Buffer2);
                        return(-1);
                      }
                    UserName[0]=0;
                    continue;
                  }
                break;
              }
            if(memicmp(Buffer, QUITCMD, strlen(QUITCMD))==0)
              { sprintf(Buffer2, "+OK POP3 server signing off\x0D\x0A");
                sRes=write(Socket, Buffer2, strlen(Buffer2));
                if(sRes<0)
                  { free(Buffer);
                    free(Buffer2);
                    return(-1);
                  }
                break;
              }
            sprintf(Buffer2, "-ERR Don't know how to do \"%s\"\x0D\x0A", Buffer);
            sRes=write(Socket, Buffer2, strlen(Buffer2));
            if(sRes<0)
              { free(Buffer);
                free(Buffer2);
                return(-1);
              }

          } while(1);
       if(memicmp(Buffer, QUITCMD, strlen(QUITCMD))==0) return(0);









/*Get password and check it*/
       LoggedIn=0;
       sRes=SkGetS(Socket, Buffer);
       if(sRes!=0)
         { free(Buffer);
           free(Buffer2);
           if(fBox) fclose(fBox);
           return(-1);
         }
       DelEndCRLF(Buffer);
       DelSpaces(Buffer);
     
       if(memicmp(Buffer, PASSCMD, strlen(PASSCMD))==0)
         { DelSpaces(&Buffer[strlen(PASSCMD)]);
           strncpy(Password, &Buffer[strlen(PASSCMD)], 16);
           Password[16]=0;
           if(!CheckPass(UserName, Password))
             { sprintf(Buffer2, "-ERR Login failed\x0D\x0A");
               sRes=write(Socket, Buffer2, strlen(Buffer2));
               if(sRes<0)
                 { free(Buffer);
                   free(Buffer2);
                   return(-1);
                 }

               Password[0]=0;
               UserName[0]=0;
               continue;
             }
            else
             { sprintf(Buffer,  "%s%s",    BoxPath, UserName);
               sprintf(Buffer2, "%s%s.mb", BoxPath, UserName);

/*Check previous box if any*/
               CheckBox (Buffer, Buffer2);

/*Open mailbox*/
               if(rename(Buffer, Buffer2)<0)
                 { sprintf(Buffer2, "-ERR Can't open maildrop for %s\x0D\x0A", UserName);
                   sRes=write(Socket, Buffer2, strlen(Buffer2));
                   if(sRes<0)
                     { free(Buffer);
                       free(Buffer2);
                       return(-1);
                     }

                   strcpy(Buffer, QUITCMD);
                   break;
                 }
               
               fBox=_fsopen(Buffer, "w+b", SH_DENYRW);
               if(fBox==NULL)
                 { sprintf(Buffer2, "-ERR Can't clear maildrop for %s\x0D\x0A", UserName);
                   sRes=write(Socket, Buffer2, strlen(Buffer2));
                   if(sRes<0)
                     { free(Buffer);
                       free(Buffer2);
                       return(-1);
                     }

                   strcpy(Buffer, QUITCMD);
                   break;
                 }
               fclose(fBox);
               fBox=_fsopen(Buffer2, "r+b", SH_DENYRW);
               if(fBox==NULL)
                 { sprintf(Buffer2, "-ERR Can't open maildrop for %s\x0D\x0A", UserName);
                   sRes=write(Socket, Buffer2, strlen(Buffer2));
                   if(sRes<0)
                     { free(Buffer);
                       free(Buffer2);
                       return(-1);
                     }

                   strcpy(Buffer, QUITCMD);
                   break;
                 }
               sRes=RescanBox(fBox);
               if(sRes)
                 { sprintf(Buffer2, "-ERR Can't process maildrop for %s\x0D\x0A", UserName);
                   sRes=write(Socket, Buffer2, strlen(Buffer2));
                   if(sRes<0)
                     { free(Buffer);
                       free(Buffer2);
                       if(fBox) fclose(fBox);
                       return(-1);
                     }

                   strcpy(Buffer, QUITCMD);
                   break;
                 }
               signal(SIGHUP,   (void*)&CrashPack);
               signal(SIGINT,   (void*)&CrashPack);
               signal(SIGQUIT,  (void*)&CrashPack);
               signal(SIGILL,   (void*)&CrashPack);
               signal(SIGTRAP,  (void*)&CrashPack);
               signal(SIGABRT,  (void*)&CrashPack);
               signal(SIGEMT,   (void*)&CrashPack);
               signal(SIGFPE,   (void*)&CrashPack);
               signal(SIGBUS,   (void*)&CrashPack);
               signal(SIGSEGV,  (void*)&CrashPack);
               signal(SIGSYS,   (void*)&CrashPack);
               signal(SIGPIPE,  (void*)&CrashPack);
               signal(SIGTERM,  (void*)&CrashPack);
               signal(SIGBREAK, (void*)&CrashPack);
               sprintf(Buffer2, "+OK %s's maildrop has %lu messages (%lu octets)\x0D\x0A", UserName, uMsgCount, uTotalLen);
               sRes=write(Socket, Buffer2, strlen(Buffer2));
               if(sRes<0)
                 { free(Buffer);
                   free(Buffer2);
                   if(fBox) fclose(fBox);
                   return(-1);
                 }
               LoggedIn=1;
             }
           break;
         }
       if(memicmp(Buffer, QUITCMD, strlen(QUITCMD))==0)
         { sprintf(Buffer2, "+OK POP3 server signing off\x0D\x0A");
           sRes=write(Socket, Buffer2, strlen(Buffer2));
           if(sRes<0)
             { free(Buffer);
               free(Buffer2);
               if(fBox) fclose(fBox);
               return(-1);
             }
           break;
         }
       sprintf(Buffer2, "-ERR Don't know how to do \"%s\"\x0D\x0A", Buffer);
       sRes=write(Socket, Buffer2, strlen(Buffer2));
       if(sRes<0)
         { free(Buffer);
           free(Buffer2);
           if(fBox) fclose(fBox);
           return(-1);
         }
     } while(LoggedIn==0);
  if(memicmp(Buffer, QUITCMD, strlen(QUITCMD))==0) return(0);














/*Transaction state*/

  do { sRes=SkGetS(Socket, Buffer);
       if(sRes!=0)
         { free(Buffer);
           free(Buffer2);
           if(fBox) fclose(fBox);
           PackBox(UserName, 0);
           return(-1);
         }
       DelEndCRLF(Buffer);
       DelSpaces(Buffer);



/*NOOP*/
       if(memicmp(Buffer, NOCMD, strlen(NOCMD))==0)
         { sprintf(Buffer2, "+OK It was so hard, but i've done it!\x0D\x0A");
           sRes=write(Socket, Buffer2, strlen(Buffer2));
           if(sRes<0)
             { free(Buffer);
               free(Buffer2);
               if(fBox) fclose(fBox);
               PackBox(UserName, 0);
               return(-1);
             }
           continue;
         }



/*STAT*/
       if(memicmp(Buffer, STATCMD, strlen(STATCMD))==0)
         { sprintf(Buffer2, "+OK %lu %lu\x0D\x0A", uMsgCount, uTotalLen);
           sRes=write(Socket, Buffer2, strlen(Buffer2));
           if(sRes<0)
             { free(Buffer);
               free(Buffer2);
               if(fBox) fclose(fBox);
               PackBox(UserName, 0);
               return(-1);
             }
           continue;
         }



/*LIST*/
       if(memicmp(Buffer, LISTCMD, strlen(LISTCMD))==0)
         { DelSpaces(&Buffer[strlen(LISTCMD)]);
           if(IsEmpty(&Buffer[strlen(LISTCMD)]))
             { sprintf(Buffer2, "+OK %lu messages (%lu octets)\x0D\x0A", uMsgCount, uTotalLen);
               sRes=write(Socket, Buffer2, strlen(Buffer2));
               if(sRes<0)
                 { free(Buffer);
                   free(Buffer2);
                   if(fBox) fclose(fBox);
                   PackBox(UserName, 0);
                   return(-1);
                 }
               for(i=0;i<msgCount;i++)
                { if(mBox[i].Deleted) continue;
                  sprintf(Buffer2, "%lu %lu\x0D\x0A", i+1, mBox[i].Size);
                  sRes=write(Socket, Buffer2, strlen(Buffer2));
                  if(sRes<0)
                    { free(Buffer);
                      free(Buffer2);
                      if(fBox) fclose(fBox);
                      PackBox(UserName, 0);
                      return(-1);
                    }
                }
               sprintf(Buffer2, ".\x0D\x0A");
               sRes=write(Socket, Buffer2, strlen(Buffer2));
               if(sRes<0)
                 { free(Buffer);
                   free(Buffer2);
                   if(fBox) fclose(fBox);
                   PackBox(UserName, 0);
                   return(-1);
                 }
             }
            else
             { MsgNo=atol(&Buffer[strlen(LISTCMD)])-1;
               if((MsgNo>=msgCount) || (MsgNo<0))
                 { sprintf(Buffer2, "-ERR no such message as %ld, just %lu messages in maildrop\x0D\x0A", MsgNo+1, msgCount);
                   sRes=write(Socket, Buffer2, strlen(Buffer2));
                   if(sRes<0)
                     { free(Buffer);
                       free(Buffer2);
                       if(fBox) fclose(fBox);
                       PackBox(UserName, 0);
                       return(-1);
                     }
                 }
                else
                 { if(mBox[MsgNo].Deleted)
                     { sprintf(Buffer2, "-ERR Message %lu was killed\x0D\x0A", MsgNo+1);
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                     }
                    else
                     { sprintf(Buffer2, "+OK %lu %lu\x0D\x0A", MsgNo+1, mBox[MsgNo].Size);
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                       LastMsg=MsgNo+1;
                     }
                 }
             }
           continue;
         }





/*UIDL*/
       if(memicmp(Buffer, UIDLCMD, strlen(UIDLCMD))==0)
         { DelSpaces(&Buffer[strlen(UIDLCMD)]);
           if(IsEmpty(&Buffer[strlen(UIDLCMD)]))
             { sprintf(Buffer2, "+OK There's message's names\x0D\x0A");
               sRes=write(Socket, Buffer2, strlen(Buffer2));
               if(sRes<0)
                 { free(Buffer);
                   free(Buffer2);
                   if(fBox) fclose(fBox);
                   PackBox(UserName, 0);
                   return(-1);
                 }
               for(i=0;i<msgCount;i++)
                { if(mBox[i].Deleted) continue;
                  sprintf(Buffer2, "%lu %s\x0D\x0A", i+1, mBox[i].msgID);

                  sRes=write(Socket, Buffer2, strlen(Buffer2));
                  if(sRes<0)
                    { free(Buffer);
                      free(Buffer2);
                      if(fBox) fclose(fBox);
                      PackBox(UserName, 0);
                      return(-1);
                    }
                }
               sprintf(Buffer2, ".\x0D\x0A");
               sRes=write(Socket, Buffer2, strlen(Buffer2));
               if(sRes<0)
                 { free(Buffer);
                   free(Buffer2);
                   if(fBox) fclose(fBox);
                   PackBox(UserName, 0);
                   return(-1);
                 }
             }
            else
             { MsgNo=atol(&Buffer[strlen(LISTCMD)])-1;
               if((MsgNo>=msgCount) || (MsgNo<0))
                 { sprintf(Buffer2, "-ERR no such message as %ld, just %lu messages in maildrop\x0D\x0A", MsgNo+1, msgCount);
                   sRes=write(Socket, Buffer2, strlen(Buffer2));
                   if(sRes<0)
                     { free(Buffer);
                       free(Buffer2);
                       if(fBox) fclose(fBox);
                       PackBox(UserName, 0);
                       return(-1);
                     }
                 }
                else
                 { if(mBox[MsgNo].Deleted)
                     { sprintf(Buffer2, "-ERR Message %lu was killed\x0D\x0A", MsgNo+1);
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                     }
                    else
                     { sprintf(Buffer2, "+OK %lu %s\x0D\x0A", MsgNo+1, mBox[MsgNo].msgID);
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                       LastMsg=MsgNo+1;
                     }
                 }
             }
           continue;
         }




/*RETR*/
       if(memicmp(Buffer, RETRCMD, strlen(RETRCMD))==0)
         { DelSpaces(&Buffer[strlen(RETRCMD)]);
           if(IsEmpty(&Buffer[strlen(RETRCMD)]))
             { sprintf(Buffer2, "-ERR Don't know which message you want :~(\x0D\x0A");
               sRes=write(Socket, Buffer2, strlen(Buffer2));
               if(sRes<0)
                 { free(Buffer);
                   free(Buffer2);
                   if(fBox) fclose(fBox);
                   PackBox(UserName, 0);
                   return(-1);
                 }
             }
            else
             { MsgNo=atol(&Buffer[strlen(LISTCMD)])-1;
               if((MsgNo>=msgCount) || (MsgNo<0))
                 { sprintf(Buffer2, "-ERR no such message as %ld, just %lu messages in maildrop\x0D\x0A", MsgNo+1, msgCount);
                   sRes=write(Socket, Buffer2, strlen(Buffer2));
                   if(sRes<0)
                     { free(Buffer);
                       free(Buffer2);
                       if(fBox) fclose(fBox);
                       PackBox(UserName, 0);
                       return(-1);
                     }
                 }
                else
                 { if(mBox[MsgNo].Deleted)
                     { sprintf(Buffer2, "-ERR Message %lu was killed\x0D\x0A", MsgNo+1);
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                     }
                    else
                     { sprintf(Buffer2, "+OK %u message's lines follow\x0D\x0A", mBox[MsgNo].Lines);
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                       fseek(fBox, mBox[MsgNo].Begin, SEEK_SET);
                       WasLF=0;
                       WasCR=0;
                       for(i=mBox[MsgNo].Begin;i<=mBox[MsgNo].End;i++)
                        { if(fread(&Ch, 1, 1, fBox)!=1) break;

                          if(Ch==0x0A)
                            { if(!WasCR)
                                { sprintf(Buffer2, "\x0D");
                                  sRes=write(Socket, Buffer2, strlen(Buffer2));
                                  if(sRes<0)
                                    { free(Buffer);
                                      free(Buffer2);
                                      if(fBox) fclose(fBox);
                                      PackBox(UserName, 0);
                                      return(-1);
                                    }
                                }
                            }
                          if(Ch==0x0D) WasCR=1;
                           else WasCR=0;

                          if((WasLF) && (Ch=='.'))
                            { sprintf(Buffer2, ".");
                              sRes=write(Socket, Buffer2, strlen(Buffer2));
                              if(sRes<0)
                                { free(Buffer);
                                  free(Buffer2);
                                  if(fBox) fclose(fBox);
                                  PackBox(UserName, 0);
                                  return(-1);
                                }
                            }
                          if(mBox[MsgNo].Is866)
                            { if(i==mBox[MsgNo].cBeg)
                                { 
                                  sprintf(Buffer2, "Content-Type: text/plain; charset=koi8-r\x0D\x0A");
                                  i=mBox[MsgNo].cEnd;
                                  fseek(fBox, i, SEEK_SET);
                                  /*i--;*/
                                }
                               else
                                { sprintf(Buffer2, "%c", cDOS2KOI(Ch));
                                }
                            }
                           else 
                            { if(mBox[MsgNo].NoCharset)
                                { if(i==mBox[MsgNo].hEnd)
                                    { 
                                      sprintf(Buffer2, "Content-Type: text/plain; charset=koi8-r\x0D\x0A");
                                      sRes=write(Socket, Buffer2, strlen(Buffer2));
                                      if(sRes<0)
                                        { free(Buffer);
                                          free(Buffer2);
                                          if(fBox) fclose(fBox);
                                          PackBox(UserName, 0);
                                          return(-1);
                                        }
                                    }
                                }
                              sprintf(Buffer2, "%c", Ch);
                            }
                          sRes=write(Socket, Buffer2, strlen(Buffer2));
                          if(sRes<0)
                            { free(Buffer);
                              free(Buffer2);
                              if(fBox) fclose(fBox);
                              PackBox(UserName, 0);
                              return(-1);
                            }
                          if((Ch==0x0D) || (Ch==0x0A)) WasLF=1;
                           else WasLF=0;
                        }

                       if(!WasLF && mBox[MsgNo].Lines)
                         { /*printf("Last - not CRLF, adding\n");*/
                           sprintf(Buffer2, "\x0D\x0A");
                           sRes=write(Socket, Buffer2, strlen(Buffer2));
                           if(sRes<0)
                             { free(Buffer);
                               free(Buffer2);
                               if(fBox) fclose(fBox);
                               PackBox(UserName, 0);
                               return(-1);
                             }
                         }
                       sprintf(Buffer2, ".\x0D\x0A");
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                       mBox[MsgNo].Status=ROld;
                       LastMsg=MsgNo+1;
                     }
                 }
             }
           continue;
         }



/*TOP*/

       if(memicmp(Buffer, TOPCMD, strlen(TOPCMD))==0)
         { Lines=0;
           MsgNo=0;
           Arg1=&Buffer[strlen(TOPCMD)];
           DelSpaces(Arg1);
           for(i=0;i<strlen(Arg1);i++)
            { if(IsSpace(Arg1[i])) break;
            }
           if(Arg1[i]==0) 
             { Arg2=&Arg1[i];
             }
            else
             { Arg2=&Arg1[i+1];
               Arg1[i]=0;
             }
           DelSpaces(Arg1);
           DelSpaces(Arg2);
           MsgNo=atol(Arg1)-1;
           Lines=atoi(Arg2);

           if(IsEmpty(Arg1))
             { sprintf(Buffer2, "-ERR Don't know which message you want :~(\x0D\x0A");
               sRes=write(Socket, Buffer2, strlen(Buffer2));
               if(sRes<0)
                 { free(Buffer);
                   free(Buffer2);
                   if(fBox) fclose(fBox);
                   PackBox(UserName, 0);
                   return(-1);
                 }
             }
            else
             { if((MsgNo>=msgCount) || (MsgNo<0))
                 { sprintf(Buffer2, "-ERR no such message as %ld, just %lu messages in maildrop\x0D\x0A", MsgNo+1, msgCount);
                   sRes=write(Socket, Buffer2, strlen(Buffer2));
                   if(sRes<0)
                     { free(Buffer);
                       free(Buffer2);
                       if(fBox) fclose(fBox);
                       PackBox(UserName, 0);
                       return(-1);
                     }
                 }
                else
                 { if(mBox[MsgNo].Deleted)
                     { sprintf(Buffer2, "-ERR Message %lu was killed\x0D\x0A", MsgNo+1);
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                     }
                    else
                     { if(Lines<0)
                         { sprintf(Buffer2, "-ERR Value %d is invalid for lines count\x0D\x0A", Lines);
                           sRes=write(Socket, Buffer2, strlen(Buffer2));
                           if(sRes<0)
                             { free(Buffer);
                               free(Buffer2);
                               if(fBox) fclose(fBox);
                               PackBox(UserName, 0);
                               return(-1);
                             }
                         }
                        else
                         { if(Lines>mBox[MsgNo].Lines) Lines=mBox[MsgNo].Lines;

                           sprintf(Buffer2, "+OK %u message's lines follow\x0D\x0A", Lines);
                           sRes=write(Socket, Buffer2, strlen(Buffer2));
                           if(sRes<0)
                             { free(Buffer);
                               free(Buffer2);
                               if(fBox) fclose(fBox);
                               PackBox(UserName, 0);
                               return(-1);
                             }
                           fseek(fBox, mBox[MsgNo].Begin, SEEK_SET);
                           WasLF=0;
                           
                           for(i=mBox[MsgNo].Begin;i<=mBox[MsgNo].End;i++)
                            { if(fread(&Ch, 1, 1, fBox)!=1) break;
                              if((WasLF) && (Ch=='.'))
                                { sprintf(Buffer2, ".");
                                  sRes=write(Socket, Buffer2, strlen(Buffer2));
                                  if(sRes<0)
                                    { free(Buffer);
                                      free(Buffer2);
                                      if(fBox) fclose(fBox);
                                      PackBox(UserName, 0);
                                      return(-1);
                                    }
                                }
                              if(mBox[MsgNo].Is866)
                                { if(i==mBox[MsgNo].cBeg)
                                    { 
                                      sprintf(Buffer2, "Content-Type: text/plain; charset=koi8-r\x0D\x0A");
                                      i=mBox[MsgNo].cEnd;
                                      fseek(fBox, i, SEEK_SET);
                                      /*i--;*/
                                    }
                                   else
                                    { sprintf(Buffer2, "%c", cDOS2KOI(Ch));
                                    }
                                }
                               else 
                                { if(mBox[MsgNo].NoCharset)
                                    { if(i==mBox[MsgNo].hEnd)
                                        { 
                                          sprintf(Buffer2, "Content-Type: text/plain; charset=koi8-r\x0D\x0A");
                                          sRes=write(Socket, Buffer2, strlen(Buffer2));
                                          if(sRes<0)
                                            { free(Buffer);
                                              free(Buffer2);
                                              if(fBox) fclose(fBox);
                                              PackBox(UserName, 0);
                                              return(-1);
                                            }
                                        }
                                    }
                                  sprintf(Buffer2, "%c", Ch);
                                }
                              sRes=write(Socket, Buffer2, strlen(Buffer2));
                              if(sRes<0)
                                { free(Buffer);
                                  free(Buffer2);
                                  if(fBox) fclose(fBox);
                                  PackBox(UserName, 0);
                                  return(-1);
                                }
                              if(Ch==0x0A) 
                                { WasLF=1;
                                  if(i>mBox[MsgNo].hEnd) 
                                    { Lines--;
                                    }
                                }
                               else 
                                { WasLF=0;
                                }
                              if((Lines<0) && (i>mBox[MsgNo].hEnd)) break;
                            }
                           if(!WasLF)
                             { sprintf(Buffer2, "\x0D\x0A");
                               sRes=write(Socket, Buffer2, strlen(Buffer2));
                               if(sRes<0)
                                 { free(Buffer);
                                   free(Buffer2);
                                   if(fBox) fclose(fBox);
                                   PackBox(UserName, 0);
                                   return(-1);
                                 }
                             }
                           sprintf(Buffer2, ".\x0D\x0A");
                           sRes=write(Socket, Buffer2, strlen(Buffer2));
                           if(sRes<0)
                             { free(Buffer);
                               free(Buffer2);
                               if(fBox) fclose(fBox);
                               PackBox(UserName, 0);
                               return(-1);
                             }
                           mBox[MsgNo].Status=ROld;
                           LastMsg=MsgNo+1;
                         }
                     }
                 }
             }
           continue;
         }



/*DELE*/
       if(memicmp(Buffer, DELECMD, strlen(DELECMD))==0)
         { DelSpaces(&Buffer[strlen(DELECMD)]);
           if(IsEmpty(&Buffer[strlen(DELECMD)]))
             { sprintf(Buffer2, "-ERR Don't know which message you want to delete\x0D\x0A");
               sRes=write(Socket, Buffer2, strlen(Buffer2));
               if(sRes<0)
                 { free(Buffer);
                   free(Buffer2);
                   if(fBox) fclose(fBox);
                   PackBox(UserName, 0);
                   return(-1);
                 }
             }
            else
             { MsgNo=atol(&Buffer[strlen(LISTCMD)])-1;
               if((MsgNo>=msgCount) || (MsgNo<0))
                 { sprintf(Buffer2, "-ERR no such message as %ld, just %lu messages in maildrop\x0D\x0A", MsgNo, msgCount);
                   sRes=write(Socket, Buffer2, strlen(Buffer2));
                   if(sRes<0)
                     { free(Buffer);
                       free(Buffer2);
                       if(fBox) fclose(fBox);
                       PackBox(UserName, 0);
                       return(-1);
                     }
                 }
                else
                 { if(mBox[MsgNo].Deleted)
                     { sprintf(Buffer2, "-ERR Pervert! You can't kill one message twice\x0D\x0A");
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                     }
                    else
                     { sprintf(Buffer2, "+OK message %lu killed\x0D\x0A", MsgNo+1);
                       sRes=write(Socket, Buffer2, strlen(Buffer2));
                       if(sRes<0)
                         { free(Buffer);
                           free(Buffer2);
                           if(fBox) fclose(fBox);
                           PackBox(UserName, 0);
                           return(-1);
                         }
                       mBox[MsgNo].Deleted=1;
                       uMsgCount--;
                       uTotalLen-=mBox[MsgNo].Size;
                     }
                 }
             }
           continue;
         }



/*LAST*/
       if(memicmp(Buffer, LASTCMD, strlen(LASTCMD))==0)
         { if(mBox[LastMsg-1].Deleted) LastMsg=0;
           if(LastMsg>msgCount) LastMsg=0;
           sprintf(Buffer2, "+OK %lu\x0D\x0A", LastMsg);
           sRes=write(Socket, Buffer2, strlen(Buffer2));
           if(sRes<0)
             { free(Buffer);
               free(Buffer2);
               if(fBox) fclose(fBox);
               PackBox(UserName, 0);
               return(-1);
             }
           continue;
         }



/*RSET*/
       if(memicmp(Buffer, RSETCMD, strlen(RSETCMD))==0)
         { for(i=0;i<msgCount;i++)
            mBox[i].Deleted=0;
           uMsgCount=msgCount;
           uTotalLen=TotalLen;

           sprintf(Buffer2, "+OK All changes was lost\x0D\x0A");
           sRes=write(Socket, Buffer2, strlen(Buffer2));
           if(sRes<0)
             { free(Buffer);
               free(Buffer2);
               if(fBox) fclose(fBox);
               PackBox(UserName, 0);
               return(-1);
             }
           continue;
         }



/*QUIT*/
       if(memicmp(Buffer, QUITCMD, strlen(QUITCMD))==0)
         { break;
         }
       sprintf(Buffer2, "-ERR Don't know how to do \"%s\"\x0D\x0A", Buffer);
       sRes=write(Socket, Buffer2, strlen(Buffer2));
       if(sRes<0)
         { free(Buffer);
           free(Buffer2);
           if(fBox) fclose(fBox);
           PackBox(UserName, 0);
           return(-1);
         }
     } while(1);

  free(Buffer);
  free(Buffer2);

  if(fBox!=NULL) fclose(fBox);
  if(PackBox(UserName, 1)==0)
    { sprintf(Buffer2, "+OK %lu messages (%lu octets) remains\x0D\x0A", uMsgCount, uTotalLen);
      sRes=write(Socket, Buffer2, strlen(Buffer2));
      if(sRes<0)
        { free(Buffer);
          free(Buffer2);
          if(fBox) fclose(fBox);
          return(-1);
        }
    }
   else
    { sprintf(Buffer2, "-ERR All changes has been discarded :(\x0D\x0A");
      sRes=write(Socket, Buffer2, strlen(Buffer2));
      if(sRes<0)
        { free(Buffer);
          free(Buffer2);
          if(fBox) fclose(fBox);
          return(-1);
        }
    }

  for(i=0;i<msgCount;i++)
   { free(mBox[i].msgID);
   }
  if(mBox!=NULL) free(mBox);

  return(Res);
}





