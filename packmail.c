#include "popper.h"

int PackBox(char * User, int DoDelete)
{ FILE *Tmp;
  FILE *fBox;
  FILE *rBox;
  char TmpPath[512];
  char mBoxPath[512];
  char rBoxPath[512];
  long i, Len;
  int Res;


  if(DoDelete==0)
    { for(i=0;i<msgCount;i++)
        mBox[i].Deleted=0;
      uMsgCount=msgCount;
      uTotalLen=TotalLen;
    }
  sprintf(TmpPath, "%s%s.tmp", BoxPath, User);
  sprintf(mBoxPath, "%s%s.mb", BoxPath, User);
  sprintf(rBoxPath, "%s%s", BoxPath, User);

  if(msgCount==0)
    { unlink(mBoxPath);
      return(0);
    }

  Tmp =_fsopen(TmpPath, "wb", SH_DENYRW);
  if(Tmp==NULL)
    { perror("Can't create temporary mailbox");
      return(-1);
    }
  fBox=_fsopen(mBoxPath, "rb", SH_DENYRW);
  if(fBox==NULL)
    { perror("Can't open mailbox");
      fclose(Tmp);
      return(-1);
    }

  for(i=0;i<msgCount;i++)
   { 
/*     fprintf(stderr,
             "Message %lu\n"
             "From %lX to %lX\n"
             "Header to %lX\n"
             "Status from %lX to %lX\n"
             "Status=%u, Old Status=%u\n",
             i,
             mBox[i].Begin, mBox[i].End,
             mBox[i].hEnd,
             mBox[i].sBeg, mBox[i].sEnd, mBox[i].Status, mBox[i].oStatus);*/

     if(mBox[i].Deleted) continue;

/*******************************************************************/
/*******************************************************************/
/*******************************************************************/
/*******************************************************************/


     if(mBox[i].oStatus==None)
       { if(CopyBlock(fBox, Tmp, mBox[i].Begin, mBox[i].hEnd)!=0)
           { perror("Mailbox -> TempBox copy failed");
             fclose(Tmp);
             unlink(TmpPath);
             fclose(fBox);
             return(-1);
           }
         switch(mBox[i].Status)
          { case Unreaded : Res=AddBlock(Tmp, "Status: U\x0D\x0A");
                            break;
            case Readed   : Res=AddBlock(Tmp, "Status: R\x0D\x0A");
                            break;
            case Old      : Res=AddBlock(Tmp, "Status: O\x0D\x0A");
                            break;
            case ROld     : Res=AddBlock(Tmp, "Status: RO\x0D\x0A");
                            break;
            default       : Res=AddBlock(Tmp, "Status: U\x0D\x0A");
                            break;
          }
         if(Res!=0)
           { perror("Can't add \"Status:\"");
             fclose(Tmp);
             unlink(TmpPath);
             fclose(fBox);
             return(-1);
           }

/*******************************************************************/
         if(mBox[i].noID==1)
           { Res=AddBlock(Tmp, "Message-ID: <");
             Res+=AddBlock(Tmp, mBox[i].msgID);
             Res+=AddBlock(Tmp, ">\x0D\x0A");
           }
         if(Res!=0)
           { perror("Can't add \"Message-ID:\"");
             fclose(Tmp);
             unlink(TmpPath);
             fclose(fBox);
             return(-1);
           }
/*******************************************************************/

         if(CopyBlock(fBox, Tmp, mBox[i].hEnd, mBox[i].End+1)!=0)
           { perror("Mailbox -> TempBox copy failed");
             fclose(Tmp);
             unlink(TmpPath);
             fclose(fBox);
             return(-1);
           }
       }
      else
       { if(CopyBlock(fBox, Tmp, mBox[i].Begin, mBox[i].sBeg)!=0)
           { perror("Mailbox -> TempBox copy failed");
             fclose(Tmp);
             unlink(TmpPath);
             fclose(fBox);
             return(-1);
           }
         switch(mBox[i].Status)
          { case Unreaded : Res=AddBlock(Tmp, "Status: U\x0D\x0A");
                            break;
            case Readed   : Res=AddBlock(Tmp, "Status: R\x0D\x0A");
                            break;
            case Old      : Res=AddBlock(Tmp, "Status: O\x0D\x0A");
                            break;
            case ROld     : Res=AddBlock(Tmp, "Status: RO\x0D\x0A");
                            break;
            default       : Res=AddBlock(Tmp, "Status: U\x0D\x0A");
                            break;
          }
         if(Res!=0)
           { perror("Can't add \"Status:\"");
             fclose(Tmp);
             unlink(TmpPath);
             fclose(fBox);
             return(-1);
           }
/*******************************************************************/
         if(mBox[i].noID==1)
           { Res=AddBlock(Tmp, "Message-ID: <");
             Res+=AddBlock(Tmp, mBox[i].msgID);
             Res+=AddBlock(Tmp, ">\x0D\x0A");
           }
         if(Res!=0)
           { perror("Can't add \"Message-ID:\"");
             fclose(Tmp);
             unlink(TmpPath);
             fclose(fBox);
             return(-1);
           }
/*******************************************************************/
         if(CopyBlock(fBox, Tmp, mBox[i].sEnd, mBox[i].End+1)!=0)
           { perror("Mailbox -> TempBox copy failed");
             fclose(Tmp);
             unlink(TmpPath);
             fclose(fBox);
             return(-1);
           }
       }
/************************************************/
   }
/*Packing done - mail from fBox (mBoxPath=user.mb) 
  was copied to Tmp(TmpPath=user.tmp)*/


  fclose(fBox);

/*Removing fBox (mBoxPath=user.mb)*/
  fBox=_fsopen(mBoxPath, "wb", SH_DENYRW);
  if(fBox==NULL)
    { perror("Can't reopen mailbox");
      fclose(Tmp);
      unlink(TmpPath);
      return(-1);
    }



  fclose(Tmp);
  Tmp=_fsopen(TmpPath, "r+b", SH_DENYRW);
  if(Tmp==NULL)
    { perror("Can't reopen temporary maildrop");
      fclose(fBox);
      return(-1);
    }


/*Copying tmp mailbox to mb*/
  fseek(Tmp, 0, SEEK_END);
  Len=ftell(Tmp);
  fseek(Tmp, 0, SEEK_SET);


  if(CopyBlock(Tmp, fBox, 0, Len)!=0)
    { perror("Can't copy TmpBox to mailbox");
      fclose(Tmp);
      unlink(TmpPath);
      fclose(fBox);
      return(-1);
    }

  fclose(Tmp);
  unlink(TmpPath);


/*Check for new mail*/
  if(access(rBoxPath, 0)==0)
    { /*If exist rBox (rBoxPath=user.) append it to maildrop, 
      currently in fBox (mBoxPath=user.mb)*/
      errno=0;
      rBox=_fsopen(rBoxPath, "r+b", SH_DENYRW);
      if(rBox==NULL)
        { perror("Can't open original maildrop");
          fclose(fBox);
          return(-1);
        }

      fseek(rBox, 0, SEEK_END);
      Len=ftell(rBox);
      fseek(rBox, 0, SEEK_SET);

      if(Len>0)
        { if(CopyBlock(rBox, fBox, 0, Len)!=0)
            { perror("Can't append maildrop to original");
              fclose(rBox);
              fclose(fBox);
              return(-1);
            }
        }
      fclose(rBox);
    }


  fclose(fBox);
  fBox=_fsopen(mBoxPath, "r+b", SH_DENYRW);
  if(fBox==NULL)
    { perror("Can't reopen mailbox");
      return(-1);
    }

/*Done, moving fBox to rBox*/
  rBox=_fsopen(rBoxPath, "wb", SH_DENYRW);
  if(rBox==NULL)
    { perror("Can't reopen original maildrop");
      fclose(fBox);
      return(-1);
    }

  fseek(fBox, 0, SEEK_END);
  Len=ftell(fBox);
  if(Len<0)
    { perror("Can't read maildrop");
      return(-1);
    }
      
  if(CopyBlock(fBox, rBox, 0l, Len)!=0)
    { perror("Can't append mailbox to main maildrop : some mails wasted");
    }
  fclose(rBox);
  fclose(fBox);
  unlink(mBoxPath);

  return(0);
}


