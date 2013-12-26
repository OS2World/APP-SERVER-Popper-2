#include "popper.h"

#define NEWLETTER  "From "
#define MSGID      "Message-Id:"
#define CHARSET    "Content-Type: text/"
#define STATUS     "Status:"
#define C866       "x-cp866"
#define CTYPE      "Content-Type:"
/*Content-Type: text/plain; charset=x-cp866*/

int RescanBox(FILE * fBox)
{ long i, Begin, cBegin;
  char *Buffer;
  long line;
  time_t Timer;
  unsigned long Time;
  unsigned int PID;

/*Allocate temporary buffer and check it*/
  Buffer=(char*)malloc(10240);
  if(Buffer==NULL)
    { return(-1);
    }

/*Clear old index if any*/
  for(i=0;i<msgCount;i++)
   { free(mBox[i].msgID);
   }
  if(mBox!=NULL) free(mBox);
  msgCount=0;
  TotalLen=0l;
  mBox=NULL;
  fseek(fBox, 0, SEEK_SET);

/*Build new index*/
  do { /*Skip to next From*/
       do { Begin=ftell(fBox);
            if(fgets(Buffer, 10230, fBox)==NULL)
              { if(feof(fBox)) break;
                free(Buffer);
                for(i=0;i<msgCount;i++)
                 { free(mBox[i].msgID);
                 }
                if(mBox!=NULL) free(mBox);
                return(-1);
              }
            DelEndCRLF(Buffer);
            if(msgCount>0) mBox[msgCount-1].Lines++;
          } while(memcmp(Buffer, NEWLETTER, strlen(NEWLETTER))!=0);
       if(feof(fBox)) 
         { break;
         }
       line=0;

/*New message found, allocating memory for it*/

       if(msgCount==0) mBox=(struct Letter*)malloc(sizeof(struct Letter));
        else mBox=(struct Letter*)realloc(mBox, sizeof(struct Letter)*(msgCount+1));
       if(mBox==NULL)
         { return(-1);
         }

/*Let's set values*/
       mBox[msgCount].Begin=Begin;
       if(msgCount!=0)
         { mBox[msgCount-1].End=mBox[msgCount].Begin-1;
           mBox[msgCount-1].Size=mBox[msgCount-1].End-mBox[msgCount-1].Begin+1;
           TotalLen+=mBox[msgCount-1].Size;
         }
       mBox[msgCount].Deleted=0;
       mBox[msgCount].Lines=0;
       mBox[msgCount].Is866=0;
       mBox[msgCount].NoCharset=1;
       mBox[msgCount].msgID=(char*)malloc(1);
       if(mBox[msgCount].msgID==NULL)
         { for(i=0;i<msgCount;i++)
            { free(mBox[i].msgID);
            }
           if(mBox!=NULL) free(mBox);
           return(-1);
         }
       mBox[msgCount].msgID[0]=0;
       mBox[msgCount].noID=1;
       mBox[msgCount].Status=None;
       mBox[msgCount].sBeg=0;
       mBox[msgCount].sEnd=0;

/*Scanning header for the CHARSET and MSGID*/
       do { cBegin=ftell(fBox);
            if(fgets(Buffer, 10230, fBox)==NULL)
              { if(feof(fBox)) break;
                free(Buffer);
                for(i=0;i<msgCount;i++)
                 { free(mBox[i].msgID);
                 }
                if(mBox!=NULL) free(mBox);
                return(-1);
              }
            DelSpaces(Buffer);
            /*Content-Type: text/plain; charset=x-cp866*/
            /*C866*/
            if(memicmp(Buffer, CHARSET, strlen(CHARSET))==0)
              { if(strstr(Buffer, C866)!=NULL)
                  { mBox[msgCount].Is866=1;
                    mBox[msgCount].cBeg=cBegin;
                    mBox[msgCount].cEnd=ftell(fBox);
                  }
                mBox[msgCount].NoCharset=0;
                continue;
              }
            if(memicmp(Buffer, CTYPE, strlen(CTYPE))==0)
              { mBox[msgCount].NoCharset=0;
                continue;
              }
            if(memicmp(Buffer, MSGID, strlen(MSGID))==0)
              { 
                DelSpaces(&Buffer[strlen(MSGID)]);
                mBox[msgCount].msgID=(char*)realloc(mBox[msgCount].msgID, strlen(&Buffer[strlen(MSGID)])+1);
                strcpy(mBox[msgCount].msgID, &Buffer[strlen(MSGID)]);
                if(mBox[msgCount].msgID[0]=='<') mBox[msgCount].msgID[0]=' ';
                if(mBox[msgCount].msgID[strlen(mBox[msgCount].msgID)-1]=='>') mBox[msgCount].msgID[strlen(mBox[msgCount].msgID)-1]=' ';
                DelSpaces(mBox[msgCount].msgID);
                mBox[msgCount].noID=0;
                continue;
              }
            if(memicmp(Buffer, STATUS, strlen(STATUS))==0)
              { DelSpaces(&Buffer[strlen(STATUS)]);
                mBox[msgCount].sBeg=cBegin;
                mBox[msgCount].sEnd=ftell(fBox);
                if(stricmp(&Buffer[strlen(STATUS)], "U")==0)
                  { mBox[msgCount].Status=Unreaded;
                    continue;
                  }
                if(stricmp(&Buffer[strlen(STATUS)], "R")==0)
                  { mBox[msgCount].Status=Readed;
                    continue;
                  }
                if(stricmp(&Buffer[strlen(STATUS)], "O")==0)
                  { mBox[msgCount].Status=Old;
                    continue;
                  }
                if(stricmp(&Buffer[strlen(STATUS)], "RO")==0)
                  { mBox[msgCount].Status=ROld;
                    continue;
                  }
                mBox[msgCount].Status=Unreaded;
              }
          } while(!IsEmpty(Buffer));
       mBox[msgCount].hEnd=cBegin;
       mBox[msgCount].oStatus=mBox[msgCount].Status;

/*************************************************************************/
/*
  time_t Timer;
  unsigned long Time;
  unsigned int PID;
*/
       if(mBox[msgCount].noID==1)
         { time(&Timer);
           Time=Timer;
           PID=getpid();
           mBox[msgCount].msgID=(char*)realloc(mBox[msgCount].msgID, 64);
           sprintf(mBox[msgCount].msgID, "%lu.%lu.%u.localhost", Time, msgCount, PID);
         }
/*************************************************************************/

       if(msgCount>0) 
         { if(mBox[msgCount-1].Lines>1)
             { mBox[msgCount-1].Lines-=2; /*do not count from and empty line*/
             }
            else
             { mBox[msgCount-1].Lines=0;
             }
         }

       msgCount++;
     }while(!feof(fBox));

  fseek(fBox, 0, SEEK_END);
  if(msgCount>0)
    { 
      /*mBox[msgCount-1].Lines++; Increase 'cause last letter doesn't have From line at end ?*/
      fseek(fBox, 0, SEEK_END);
      mBox[msgCount-1].End=ftell(fBox)-1;
/*      printf("Last = %lu\n", mBox[msgCount-1].End);*/
      mBox[msgCount-1].Size=mBox[msgCount-1].End-mBox[msgCount-1].Begin+1;
      TotalLen+=mBox[msgCount-1].Size;
    }

  uTotalLen=TotalLen;
  uMsgCount=msgCount;

  free(Buffer);

  if(msgCount==0) 
    { mBox=(struct Letter*)malloc(sizeof(struct Letter));
      if(mBox==NULL) return(-1);
    }
  return(0);
}

