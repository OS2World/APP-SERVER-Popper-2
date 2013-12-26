#include "popper.h"

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

int IsCRLF (char Ch)
{ if((Ch==0x0D) || (Ch==0x0A)) return(1);
  return(0);
}

void DelEndCRLF (char * Str)
{ register int i;

  for(i=strlen(Str)-1;(i>0) && IsCRLF(Str[i]);i--)
   Str[i]=0;
}

static char cTbl[]="áâ÷çäåöúéê"
                   "ëìíîïğòóôõ"
                   "æèãşûıÿùøü"
                   "àñÁÂ×ÇÄÅÖÚ"
                   "ÉÊËÌÍÎÏĞ‘"
                   "’‡²´§¦µ¡¨"
                   "®­¬ƒ„‰ˆ†€Š"
                   "¯°«¥»¸± ¾¹"
                   "º¶·ª©¢¤½¼…"
                   "‚Œ‹ÒÓÔÕ"
                   "ÆÈÃŞÛİßÙØÜ"
                   "ÀÑ³£™˜“›Ÿ—"
                   "œ•–¿” ";



unsigned char cDOS2KOI (unsigned char Ch)
{
 if(Ch<128) return(Ch);
 return(cTbl[Ch-128]);
};



void _PrintTime (FILE * Fl)
{ time_t Timer;
  struct tm * Time;

  time(&Timer);
  Time=localtime(&Timer);

  fprintf(Fl, " %2.2u/%2.2u/%2.2u %2.2u:%2.2u ", (unsigned int)Time->tm_mday
                                               , (unsigned int)Time->tm_mon+1
                                               , (unsigned int)Time->tm_year
                                               , (unsigned int)Time->tm_hour
                                               , (unsigned int)Time->tm_min);

}

void PrintTime(void)
{ _PrintTime(stdout);
}

void ePrintTime(void)
{ _PrintTime(stderr);
}





