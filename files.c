#include "popper.h"

#define CACHESIZE 8192
/*Appending from fFrom to end of fTo beginning from fFrom[From] to fFrom[To]*/
int CopyBlock (FILE * fFrom, FILE * fTo, long From, long To)
{ char * Buffer;
  long Size;
  int Res;

  
  Buffer=(char *)malloc(CACHESIZE);
  if(Buffer==NULL) return(-1);

  fseek(fFrom, From, SEEK_SET);
  fseek(fTo, 0, SEEK_END);
  Size=To-From;

  do { if(Size<CACHESIZE)
         { Res=fread(Buffer, Size, 1, fFrom);
           if(Res<0)
             { free(Buffer);
               return(-1);
             }
           Res=fwrite(Buffer, Size, 1, fTo);
           if(Res<0)
             { free(Buffer);
               return(-1);
             }
           Size=0;
         }
        else
         { Res=fread(Buffer, CACHESIZE, 1, fFrom);
           if(Res<0)
             { free(Buffer);
               return(-1);
             }
           Res=fwrite(Buffer, CACHESIZE, 1, fTo);
           if(Res<0)
             { free(Buffer);
               return(-1);
             }
           Size-=CACHESIZE;
         }

     }while(Size>0);

  return(0);
}


int AddBlock (FILE * fTo, char * Str)
{ int Res;

  fseek(fTo, 0, SEEK_END);
  Res=fwrite(Str, strlen(Str), 1, fTo);
  if(Res<0)
    {
      return(-1);
    }

  return(0);
}






