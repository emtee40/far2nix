/*
  PDF.CPP

  Second-level plugin module for FAR Manager and MultiArc plugin

  Copyright (c) 1996 Eugene Roshal
  Copyrigth (c) 2000 FAR group
*/

#include <windows.h>
#include <utils.h>
#include <string.h>
#include <pluginold.hpp>

/*
#include "./PDF-Writer-master/PDFWriter/PDFParser.h"
#include "./PDF-Writer-master/PDFWriter/InputFile.h"
#include "./PDF-Writer-master/PDFWriter/PDFObject.h"
#include "./PDF-Writer-master/PDFWriter/PDFDictionary.h"
#include "./PDF-Writer-master/PDFWriter/PDFObjectCast.h"
#include "./PDF-Writer-master/PDFWriter/PDFIndirectObjectReference.h"
#include "./PDF-Writer-master/PDFWriter/PDFArray.h"
#include "./PDF-Writer-master/PDFWriter/PDFDictionary.h"
#include "./PDF-Writer-master/PDFWriter/PDFStreamInput.h"

using namespace PDFHummus;

*/
using namespace oldfar;
#include "fmt.hpp"


#if defined(__BORLANDC__)
  #pragma option -a1
#elif defined(__GNUC__) || (defined(__WATCOMC__) && (__WATCOMC__ < 1100)) || defined(__LCC__)
  #pragma pack(1)
  #if defined(__LCC__)
    #define _export __declspec(dllexport)
  #endif
#else
  #pragma pack(push,1)
  #if _MSC_VER
    #define _export
  #endif
#endif


/*
#ifdef _MSC_VER
#if _MSC_VER < 1310
#pragma comment(linker, "/ignore:4078")
#pragma comment(linker, "/merge:.data=.")
#pragma comment(linker, "/merge:.rdata=.")
#pragma comment(linker, "/merge:.text=.")
#pragma comment(linker, "/section:.,RWE")
#endif
#endif
*/

static HANDLE ArcHandle;
static DWORD NextPosition,FileSize;


void WINAPI UnixTimeToFileTime( DWORD time, FILETIME * ft );

void  WINAPI _export PDF_SetFarInfo(const struct PluginStartupInfo *Info)
{
  ;
}

BOOL WINAPI _export PDF_IsArchive(const char *Name,const unsigned char *Data,int DataSize)
{
  if (DataSize<26 || Data[0]!='%' || Data[1]!='P' || Data[2]!='D' || Data[3]!='F' || Data[4]!='-')
    return(FALSE);
  //char Type=Data[7];
  //if ((Type>'8' && Type<'1'))
  //  return(FALSE);
  return(TRUE);
}


BOOL WINAPI _export PDF_OpenArchive(const char *Name,int *Type)
{
  ArcHandle=WINPORT(CreateFile)(MB2Wide(Name).c_str(),GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                       NULL,OPEN_EXISTING,FILE_FLAG_SEQUENTIAL_SCAN,NULL);
  if (ArcHandle==INVALID_HANDLE_VALUE)
    return(FALSE);

  *Type=0;

  FileSize=WINPORT(GetFileSize)(ArcHandle,NULL);

  NextPosition=4;
/*
  InputFile pdfFile;
  PDFParser parser;

  EStatusCode status = PDFHummus::eSuccess; //pdfFile.OpenFile(MB2Wide(Name));
  if(status != PDFHummus::eSuccess)
    return(FALSE);

  status = parser.StartPDFParsing(pdfFile.GetInputStream());
*/
  return(TRUE);
}


int WINAPI _export PDF_GetArcItem(struct PluginPanelItem *Item,struct ArcItemInfo *Info)
{
  struct PdfHeader
  {
    BYTE Type;
    DWORD PackSize;
    DWORD UnpSize;
    DWORD CRC;
    DWORD FileTime;
  } Header;
  DWORD ReadSize;
  NextPosition=WINPORT(SetFilePointer)(ArcHandle,NextPosition,NULL,FILE_BEGIN);
  if (NextPosition==0xFFFFFFFF)
    return(GETARC_READERROR);
  if (NextPosition>FileSize)
    return(GETARC_UNEXPEOF);
  if (!WINPORT(ReadFile)(ArcHandle,&Header,sizeof(Header),&ReadSize,NULL))
    return(GETARC_READERROR);
  if (ReadSize==0)
    return(GETARC_EOF);
  char Path[3*NM],Name[NM];
  if (!WINPORT(ReadFile)(ArcHandle,Path,sizeof(Path),&ReadSize,NULL) || ReadSize==0)
    return(GETARC_READERROR);
  Path[NM-1]=0;
  int PathLength=strlen(Path)+1;
  strncpy(Name,Path+PathLength,sizeof(Name));
  int Length=PathLength+strlen(Name)+1;
  DWORD PrevPosition=NextPosition;
  NextPosition+=sizeof(Header)+Length+Path[Length]+1+Header.PackSize;
  if (PrevPosition>=NextPosition)
    return(GETARC_BROKEN);
  char *EndSym=strrchr(Path,255);
  if (EndSym!=NULL)
    *EndSym=0;
  if (*Path)
    strcat(Path,"/");
  strcat(Path,Name);
  for (int I=0;Path[I]!=0;I++)
    if ((unsigned char)Path[I]==0xff)
      Path[I]='/';
  strncpy(Item->FindData.cFileName,Path,sizeof(Item->FindData.cFileName));
  Item->FindData.dwFileAttributes=(Header.Type & 0xf)==0xe ? FILE_ATTRIBUTE_DIRECTORY:0;
  Item->CRC32=Header.CRC;
  UnixTimeToFileTime(Header.FileTime,&Item->FindData.ftLastWriteTime);
  Item->FindData.nFileSizeLow=Header.UnpSize;
  Item->FindData.nFileSizeHigh=0;
  Item->PackSize=Header.PackSize;
  return(GETARC_SUCCESS);
}


BOOL WINAPI _export PDF_CloseArchive(struct ArcInfo *Info)
{
  return(WINPORT(CloseHandle)(ArcHandle));
}

BOOL WINAPI _export PDF_GetFormatName(int Type,char *FormatName,char *DefaultExt)
{
  if (Type==0)
  {
    strcpy(FormatName,"PDF");
    strcpy(DefaultExt,"pdf");
    return(TRUE);
  }
  return(FALSE);
}


BOOL WINAPI _export PDF_GetDefaultCommands(int Type,int Command,char *Dest)
{
  if (Type==0)
  {
    static const char *Commands[]={
    /*Extract               */"hap xay %%a %%FMQ",
    /*Extract without paths */"hap eay %%a %%FMQ",
    /*Test                  */"hap t %%a %%FMQ",
    /*Delete                */"hap d %%a %%FMQ",
    /*Comment archive       */"",
    /*Comment files         */"",
    /*Convert to SFX        */"",
    /*Lock archive          */"",
    /*Protect archive       */"",
    /*Recover archive       */"",
    /*Add files             */"hap as2{%%S} %%a %%FQ",
    /*Move files            */"hap asm2{%%S} %%a %%FQ",
    /*Add files and folders */"hap asr2{%%S} %%a %%FMQ",
    /*Move files and folders*/"hap asmr2{%%S} %%a %%FMQ",
    /*"All files" mask      */"*.*"
    };
    if (Command<(int)(ARRAYSIZE(Commands)))
    {
      strcpy(Dest,Commands[Command]);
      return(TRUE);
    }
  }
  return(FALSE);
}
