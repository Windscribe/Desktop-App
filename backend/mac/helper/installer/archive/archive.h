#pragma once

//LZMA SDK it is taken from here https://www.7-zip.org/a/lzma1805.7z

#ifdef _WIN32
#include <Windows.h>
#endif


#include <stdio.h>
#include <string.h>

#include "LZMA_SDK/C/7z.h"
#include "LZMA_SDK/C/7zBuf.h"
#include "LZMA_SDK/C/7zFile.h"
#include "LZMA_SDK/C/7zVersion.h"

//#include "../../Utils/logger.h"



typedef struct
{
 #ifdef _WIN32
 BYTE* pData;           //the pointer on the first byte of the file.7z
 #else
 unsigned char* pData;
 #endif

 Int64 Position; //number of byte in an array from which will be will be executed reading
 unsigned long Length;   //it is long the file.7z
} CSzFile1;



#ifndef USE_WINDOWS_FILE
/* for mkdir */
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#include <errno.h>
#endif
#endif


#define kInputBufSize (static_cast<size_t>(1) << 18)


#include <string>
#include <list>
#include <fstream>

typedef struct
{
  ISeekInStream vt;
  CSzFile1 file;
} CFileInStream1;

#ifdef _WIN32
#define MY_FILE_CODE_PAGE_PARAM ,g_FileCodePage
#else
#define MY_FILE_CODE_PAGE_PARAM
#endif


#define PERIOD_4 (4 * 365 + 1)
#define PERIOD_100 (PERIOD_4 * 25 - 1)
#define PERIOD_400 (PERIOD_100 * 4 + 1)



#if defined __APPLE__
#define E_ABORT    0x80004004L
#define S_OK       0L

#include <unistd.h>
#endif


class Archive
{
 private:
    #ifdef _WIN32
    HGLOBAL hGlobal;
    #endif

    unsigned long file_size;
    unsigned char* pData;

    uid_t userId_;
    gid_t groupId_;

    UInt64 importantTotalUnpacked;
    UInt64 Total;
    UInt64 Completed;
    SRes res;

    std::list<std::wstring> file_list;
    std::list<std::wstring> path_list;

    CSzArEx db;
    ISzAlloc allocImp;
    ISzAlloc allocTempImp;

    CFileInStream1 archiveStream;
    CLookToRead2 lookStream;

    UInt16 *temp;
    size_t tempSize;

    UInt32 blockIndex; /* it can have any value before first call (if outBuffer = 0) */
    Byte *outBuffer; /* it must be 0 before first call for each new archive. */
    size_t outBufferSize;  /* it can have any value before first call (if outBuffer = 0) */

    UInt64 val;
    UInt64 max_percent;

    std::string path_to_the_log;

    std::wstring last_error;

    void Print(const char *s);
    int Buf_EnsureSize(CBuf *dest, size_t size);

    #ifndef _WIN32
    #define _USE_UTF8
    #endif

   /* #define _USE_UTF8 */
    #ifdef _USE_UTF8
    #define _UTF8_START(n) (0x100 - (1 << (7 - (n))))
    #define _UTF8_RANGE(n) (((UInt32)1) << ((n) * 5 + 6))
    #define _UTF8_HEAD(n, val) ((Byte)(_UTF8_START(n) + (val >> (6 * (n)))))
    #define _UTF8_CHAR(n, val) ((Byte)(0x80 + (((val) >> (6 * (n))) & 0x3F)))

    size_t Utf16_To_Utf8_Calc(const UInt16 *src, const UInt16 *srcLim);
    Byte *Utf16_To_Utf8(Byte *dest, const UInt16 *src, const UInt16 *srcLim);
    SRes Utf16_To_Utf8Buf(CBuf *dest, const UInt16 *src, size_t srcLen);
    #endif

    SRes Utf16_To_Char(CBuf *buf, const UInt16 *s
    #ifndef _USE_UTF8
        , UINT codePage
    #endif
    );


    WRes MyCreateDir(const UInt16 *name);
    WRes OutFile_OpenUtf16(CSzFile *p, const UInt16 *name);
    SRes PrintString(const UInt16 *s);
    void UInt64ToStr(UInt64 value, char *s, int numDigits);
    char *UIntToStr(char *s, unsigned value, int numDigits);
    void UIntToStr_2(char *s, unsigned value);
    void ConvertFileTimeToString(const CNtfsFileTime *nt, char *s);
    void PrintLF();
    void PrintError(const char *s);
    void GetAttribString(UInt32 wa, bool isDir, char *s);

    static WRes File_Read1(CSzFile1 *p, void *data, size_t *size);
    static SRes FileInStream_Read1(ISeekInStreamPtr pp, void *buf, size_t *size);
    static WRes File_Seek1(CSzFile1 *p, Int64 *pos, ESzSeek origin);
    static SRes FileInStream_Seek1(ISeekInStreamPtr pp, Int64 *pos, ESzSeek origin);

    void FileInStream_CreateVTable1(CFileInStream1 *p);

    std::string ConvertToString(const std::wstring &str);
    void ConvertUInt32ToString(UInt32 val, char *s);
    void ConvertUInt64ToString(UInt64 val, char *s);


    std::u16string getFileName(const UInt32 &i, UInt16 *&temp, size_t &tempSize);
    void printPercent(const size_t &processedSize);

 public:
    Archive(const std::wstring &name, uid_t userId, gid_t groupId);
    ~Archive();

    bool isCorrect() const;

   //list of files
    SRes fileList(std::list<std::wstring> &file_list);

   //extract files[i] to path paths[i]
    void calcTotal(const std::list<std::wstring> &files, const std::list<std::wstring> &paths);
    UInt32 getNumFiles();
    SRes extractionFile(const UInt32 &i);
    UInt64 getPercent();
    UInt64 getMaxPercent();
    bool is_finish();
    SRes finish();
    std::wstring getLastError();
};
