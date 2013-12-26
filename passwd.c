#include "popper.h"


int ReadUser (struct Pwd * uPw, char * Passwd)
{ char * Buffer;
  FILE * fPasswd;
  register int i;

  strcpy(uPw->Pass, "*");

  Buffer=(char *)malloc(4096);
  if(Buffer==NULL)
    {
      return(-1);
    }
  fPasswd=fopen(Passwd, "rb");
  if(fPasswd==NULL)
    {
      free(Buffer);
      return(-1);
    }

  do { fgets(Buffer, 4095, fPasswd);
       DelEndCRLF(Buffer);
       DelSpaces(Buffer);

       if(Buffer[0]=='#') continue;
       if((memcmp(Buffer, uPw->User, strlen(uPw->User))==0) && (Buffer[strlen(uPw->User)]==':'))
         { for(i=strlen(uPw->User)+1;(Buffer[i]!=0) && (Buffer[i]!=':');i++)
            { if(i>18) break;
            }
           Buffer[i]=0;
           strcpy(uPw->Pass, &Buffer[strlen(uPw->User)+1]);
           if(IsEmpty(uPw->Pass))
             { strcpy(uPw->Pass, "*");
             }
           break;
         }
     } while(!feof(fPasswd));

  free(Buffer);

  return(0);
}

