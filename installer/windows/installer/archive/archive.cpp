#include "archive.h"

static const ISzAlloc g_Alloc = { SzAlloc, SzFree };

#ifdef _WIN32
#ifndef USE_WINDOWS_FILE
static UINT g_FileCodePage = CP_ACP;
#endif
#endif


#ifndef _WIN32
//It is taken from here https://stackoverflow.com/questions/34641373/how-do-i-embed-the-contents-of-a-binary-file-in-an-executable-on-mac-os-x
asm(
    ".global _data_start_somefile\n\t"
    ".global _data_end_somefile\n\t"
    "_data_start_somefile:\n\t"
    ".incbin \"resources/windscribe.7z\"\n\t"
    "_data_end_somefile:\n\t"
);

extern char data_start_somefile, data_end_somefile;
#endif

using namespace std;

Archive::Archive(const wstring& name)
{
    importantTotalUnpacked = 0;
    Total = 0;
    Completed = 0;
    res = SZ_OK;

#ifdef _WIN32
    LPCWSTR name1 = name.c_str();

    HRSRC hResource = FindResource(nullptr, name1, RT_RCDATA);
    if (!hResource)
    {

    }

    hGlobal = LoadResource(nullptr, hResource);
    if (!hGlobal)
    {

    }


    pData = static_cast<BYTE*>(LockResource(hGlobal));
    if (!pData)
    {

    }


    file_size = SizeofResource(nullptr, hResource);
#else
    pData = (unsigned char*)&data_start_somefile;

    file_size = int(&data_end_somefile - &data_start_somefile);
#endif


    SzArEx_Init(&db);

#if defined(_WIN32) && !defined(USE_WINDOWS_FILE) && !defined(UNDER_CE)
    g_FileCodePage = AreFileApisANSI() ? CP_ACP : CP_OEMCP;
#endif

    allocImp = g_Alloc;
    allocTempImp = g_Alloc;


    //We work with the file as with an array of bytes
    archiveStream.file.pData = pData;
    archiveStream.file.Position = 0;
    archiveStream.file.Length = file_size;

    FileInStream_CreateVTable1(&archiveStream);
    LookToRead2_CreateVTable(&lookStream, False);
    lookStream.buf = nullptr;
    lookStream.buf = static_cast<Byte*>(ISzAlloc_Alloc(&allocImp, kInputBufSize));

    if (!lookStream.buf)
    {
        res = SZ_ERROR_MEM;
    }
    else
    {
        lookStream.bufSize = kInputBufSize;
        lookStream.realStream = &archiveStream.vt;
        LookToRead2_Init(&lookStream);
    }


    CrcGenerateTable();

    if (res == SZ_OK)
    {
        res = SzArEx_Open(&db, &lookStream.vt, &allocImp, &allocTempImp);
    }

    temp = nullptr;
    tempSize = 0;

    blockIndex = 0xFFFFFFFF;
    outBuffer = nullptr;
    outBufferSize = 0;

    val = 0;
    max_percent = 90;
}

Archive::~Archive()
{
#ifdef _WIN32
    FreeResource(hGlobal); // done with data
#endif

    SzArEx_Free(&db, &allocImp);

    if (lookStream.buf != nullptr)
    {
        ISzAlloc_Free(&allocImp, lookStream.buf);
    }

    SzFree(nullptr, temp);
}

void Archive::Print(const char* s)
{
#ifndef GUI
    fputs(s, stdout);
#else
    Log::instance().trace("(archive) " + string(s));
#endif
}


int Archive::Buf_EnsureSize(CBuf* dest, size_t size)
{
    if (dest->size >= size)
        return 1;
    Buf_Free(dest, &g_Alloc);
    return Buf_Create(dest, size, &g_Alloc);
}

#ifndef _WIN32
#define _USE_UTF8
#endif

/* #define _USE_UTF8 */

#ifdef _USE_UTF8

#define _UTF8_START(n) (0x100 - (1 << (7 - (n))))

#define _UTF8_RANGE(n) (((UInt32)1) << ((n) * 5 + 6))

#define _UTF8_HEAD(n, val) ((Byte)(_UTF8_START(n) + (val >> (6 * (n)))))
#define _UTF8_CHAR(n, val) ((Byte)(0x80 + (((val) >> (6 * (n))) & 0x3F)))

size_t Archive::Utf16_To_Utf8_Calc(const UInt16* src, const UInt16* srcLim)
{
    size_t size = 0;
    for (;;)
    {
        UInt32 val;
        if (src == srcLim)
            return size;

        size++;
        val = *src++;

        if (val < 0x80)
            continue;

        if (val < _UTF8_RANGE(1))
        {
            size++;
            continue;
        }

        if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
        {
            UInt32 c2 = *src;
            if (c2 >= 0xDC00 && c2 < 0xE000)
            {
                src++;
                size += 3;
                continue;
            }
        }

        size += 2;
    }
}

Byte* Archive::Utf16_To_Utf8(Byte* dest, const UInt16* src, const UInt16* srcLim)
{
    for (;;)
    {
        UInt32 val;
        if (src == srcLim)
            return dest;

        val = *src++;

        if (val < 0x80)
        {
            *dest++ = (char)val;
            continue;
        }

        if (val < _UTF8_RANGE(1))
        {
            dest[0] = _UTF8_HEAD(1, val);
            dest[1] = _UTF8_CHAR(0, val);
            dest += 2;
            continue;
        }

        if (val >= 0xD800 && val < 0xDC00 && src != srcLim)
        {
            UInt32 c2 = *src;
            if (c2 >= 0xDC00 && c2 < 0xE000)
            {
                src++;
                val = (((val - 0xD800) << 10) | (c2 - 0xDC00)) + 0x10000;
                dest[0] = _UTF8_HEAD(3, val);
                dest[1] = _UTF8_CHAR(2, val);
                dest[2] = _UTF8_CHAR(1, val);
                dest[3] = _UTF8_CHAR(0, val);
                dest += 4;
                continue;
            }
        }

        dest[0] = _UTF8_HEAD(2, val);
        dest[1] = _UTF8_CHAR(1, val);
        dest[2] = _UTF8_CHAR(0, val);
        dest += 3;
    }
}

SRes Archive::Utf16_To_Utf8Buf(CBuf* dest, const UInt16* src, size_t srcLen)
{
    size_t destLen = Utf16_To_Utf8_Calc(src, src + srcLen);
    destLen += 1;
    if (!Buf_EnsureSize(dest, destLen))
        return SZ_ERROR_MEM;
    *Utf16_To_Utf8(dest->data, src, src + srcLen) = 0;
    return SZ_OK;
}

#endif

SRes Archive::Utf16_To_Char(CBuf* buf, const UInt16* s
#ifndef _USE_UTF8
    , UINT codePage
#endif
)
{
    unsigned len = 0;
    for (len = 0; s[len] != 0; len++)
    {

    }

#ifndef _USE_UTF8
    {
        unsigned size = len * 3 + 100;
        if (!Buf_EnsureSize(buf, size))
            return SZ_ERROR_MEM;
        {
            buf->data[0] = 0;
            if (len != 0)
            {
                char defaultChar = '_';
                BOOL defUsed;
                unsigned numChars = 0;
                numChars = static_cast<unsigned int>(WideCharToMultiByte(codePage, 0, reinterpret_cast<LPCWSTR>(s), static_cast<int>(len), reinterpret_cast<char*>(buf->data), static_cast<int>(size), &defaultChar, &defUsed));
                if (numChars == 0 || numChars >= size)
                    return SZ_ERROR_FAIL;
                buf->data[numChars] = 0;
            }
            return SZ_OK;
        }
    }
#else
    return Utf16_To_Utf8Buf(buf, s, len);
#endif
}



WRes Archive::MyCreateDir(const UInt16* name)
{
#ifdef USE_WINDOWS_FILE

    return CreateDirectoryW((LPCWSTR)name, nullptr) ? 0 : GetLastError();

#else

    CBuf buf;
    WRes res;
    Buf_Init(&buf);


    SRes __result__ = Utf16_To_Char(&buf, name MY_FILE_CODE_PAGE_PARAM);
    if (__result__ != 0)
    {
        return static_cast<WRes>(__result__);
    }

    res = static_cast<WRes>(
#ifdef _WIN32
        _mkdir(reinterpret_cast<const char*>(buf.data))
#else
        mkdir(reinterpret_cast<const char*>(buf.data), 0777)
#endif
        == 0 ? 0 : errno);
    Buf_Free(&buf, &g_Alloc);

    return res;

#endif
}

WRes Archive::OutFile_OpenUtf16(CSzFile* p, const UInt16* name)
{
#ifdef USE_WINDOWS_FILE
    return OutFile_OpenW(p, (LPCWSTR)name);
#else
    CBuf buf;
    WRes res;
    Buf_Init(&buf);

    SRes __result__ = Utf16_To_Char(&buf, name MY_FILE_CODE_PAGE_PARAM);
    if (__result__ != 0)
    {
        return static_cast<WRes>(__result__);
    }

    res = OutFile_Open(p, reinterpret_cast<const char*>(buf.data));
    Buf_Free(&buf, &g_Alloc);
    return res;
#endif
}


SRes Archive::PrintString(const UInt16* s)
{
    CBuf buf;
    SRes res;
    Buf_Init(&buf);
    res = Utf16_To_Char(&buf, s
#ifndef _USE_UTF8
        , CP_OEMCP
#endif
    );
    if (res == SZ_OK)
    {
        Print(reinterpret_cast<const char*>(buf.data));
    }
    Buf_Free(&buf, &g_Alloc);
    return res;
}

void Archive::UInt64ToStr(UInt64 value, char* s, int numDigits)
{
    char temp[32];
    int pos = 0;
    do
    {
        temp[pos++] = static_cast<char>('0' + static_cast<unsigned>(value % 10));
        value /= 10;
    } while (value != 0);

    for (numDigits -= pos; numDigits > 0; numDigits--)
        *s++ = ' ';

    do
        *s++ = temp[--pos];
    while (pos);
    *s = '\0';
}

char* Archive::UIntToStr(char* s, unsigned value, int numDigits)
{
    char temp[16];
    int pos = 0;
    do
        temp[pos++] = static_cast<char>('0' + (value % 10));
    while (value /= 10);

    for (numDigits -= pos; numDigits > 0; numDigits--)
        *s++ = '0';

    do
        *s++ = temp[--pos];
    while (pos);
    *s = '\0';
    return s;
}

void Archive::UIntToStr_2(char* s, unsigned value)
{
    s[0] = static_cast<char>('0' + (value / 10));
    s[1] = static_cast<char>('0' + (value % 10));
}

#define PERIOD_4 (4 * 365 + 1)
#define PERIOD_100 (PERIOD_4 * 25 - 1)
#define PERIOD_400 (PERIOD_100 * 4 + 1)

void Archive::ConvertFileTimeToString(const CNtfsFileTime* nt, char* s)
{
    unsigned year, mon, hour, min, sec;
    Byte ms[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    unsigned t;
    UInt32 v;
    UInt64 v64 = nt->Low | (static_cast<UInt64>(nt->High) << 32);
    v64 /= 10000000;
    sec = static_cast<unsigned>(v64 % 60); v64 /= 60;
    min = static_cast<unsigned>(v64 % 60); v64 /= 60;
    hour = static_cast<unsigned>(v64 % 24); v64 /= 24;

    v = static_cast<UInt32>(v64);

    year = static_cast<unsigned>(1601 + v / PERIOD_400 * 400);
    v %= PERIOD_400;

    t = v / PERIOD_100; if (t == 4) t = 3; year += t * 100; v -= t * PERIOD_100;
    t = v / PERIOD_4;   if (t == 25) t = 24; year += t * 4;   v -= t * PERIOD_4;
    t = v / 365;        if (t == 4) t = 3; year += t;       v -= t * 365;

    if (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0))
        ms[1] = 29;
    for (mon = 0;; mon++)
    {
        unsigned d = ms[mon];
        if (v < d)
            break;
        v -= d;
    }
    s = UIntToStr(s, year, 4); *s++ = '-';
    UIntToStr_2(s, mon + 1); s[2] = '-'; s += 3;
    UIntToStr_2(s, static_cast<unsigned>(v) + 1); s[2] = ' '; s += 3;
    UIntToStr_2(s, hour); s[2] = ':'; s += 3;
    UIntToStr_2(s, min); s[2] = ':'; s += 3;
    UIntToStr_2(s, sec); s[2] = 0;
}

void Archive::PrintLF()
{
    Print("\n");
}

void Archive::PrintError(const char* s)
{
    Print("\nERROR: ");
    Print(s);
    PrintLF();

    string s1 = s;
    last_error = wstring(s1.begin(), s1.end());
}

wstring Archive::getLastError()
{
    return last_error;
}

void Archive::GetAttribString(UInt32 wa, Bool isDir, char* s)
{
#ifdef USE_WINDOWS_FILE
    s[0] = (char)(((wa & FILE_ATTRIBUTE_DIRECTORY) != 0 || isDir) ? 'D' : '.');
    s[1] = (char)(((wa & FILE_ATTRIBUTE_READONLY) != 0) ? 'R' : '.');
    s[2] = (char)(((wa & FILE_ATTRIBUTE_HIDDEN) != 0) ? 'H' : '.');
    s[3] = (char)(((wa & FILE_ATTRIBUTE_SYSTEM) != 0) ? 'S' : '.');
    s[4] = (char)(((wa & FILE_ATTRIBUTE_ARCHIVE) != 0) ? 'A' : '.');
    s[5] = 0;
#else
    s[0] = static_cast<char>(((wa & (1 << 4)) != 0 || isDir) ? 'D' : '.');
    s[1] = 0;
#endif
}


WRes Archive::File_Read1(CSzFile1* p, void* data, size_t* size)
{
    size_t originalSize = *size;
    if (originalSize == 0)
        return 0;

    memcpy(data, p->pData + p->Position, originalSize);
    p->Position = p->Position + originalSize;
    *size = originalSize;
    //*size = fread(data, 1, originalSize, p->file);

    if (*size == originalSize)
        return 0;

    //return ferror(p->file);
    return 1;
}

SRes Archive::FileInStream_Read1(ISeekInStream* pp, void* buf, size_t* size)
{
    char* p0 = reinterpret_cast<char*>((pp)-MY_offsetof(CFileInStream1, vt));

    CFileInStream1* p = reinterpret_cast<CFileInStream1*>(p0);

    //CFileInStream1 *p = CONTAINER_FROM_VTBL(pp, CFileInStream1, vt);

    return (File_Read1(&p->file, buf, size) == 0) ? SZ_OK : SZ_ERROR_READ;
}


WRes Archive::File_Seek1(CSzFile1* p, Int64* pos, ESzSeek origin)
{
    int moveMethod;
    WRes res = 0;

    switch (origin)
    {
    case SZ_SEEK_SET:
        moveMethod = SEEK_SET;
        p->Position = *pos;
        break;
    case SZ_SEEK_CUR:
        moveMethod = SEEK_CUR;
        p->Position = p->Position + *pos;
        *pos = p->Position;
        break;

    case SZ_SEEK_END:
        moveMethod = SEEK_END;
        p->Position = p->Length;
        *pos = p->Position;

        break;
    default: res = 1;
    }



    //res = fseek(p->file, (long)*pos, moveMethod);
    //*pos = ftell(p->file);


    return res;
}
SRes Archive::FileInStream_Seek1(ISeekInStream* pp, Int64* pos, ESzSeek origin)
{
    char* p0 = reinterpret_cast<char*>((pp)-MY_offsetof(CFileInStream1, vt));

    CFileInStream1* p = reinterpret_cast<CFileInStream1*>(p0);

    //CFileInStream1 *p = CONTAINER_FROM_VTBL(pp, CFileInStream1, vt);
    return static_cast<SRes>(File_Seek1(&p->file, pos, origin));
}

void Archive::FileInStream_CreateVTable1(CFileInStream1* p)
{
    p->vt.Read = FileInStream_Read1;
    p->vt.Seek = FileInStream_Seek1;
}


#define CONVERT_INT_TO_STR(charType, tempSize) \
  unsigned char temp[tempSize]; unsigned i = 0; \
  while (val >= 10) { temp[i++] = static_cast<unsigned char>('0' + static_cast<unsigned>(val % 10)); val /= 10; } \
  *s++ = static_cast<charType>('0' + static_cast<unsigned>(val)); \
  while (i != 0) { i--; *s++ = static_cast<char>(temp[i]); } \
  *s = 0;

void Archive::ConvertUInt32ToString(UInt32 val, char* s)
{
    CONVERT_INT_TO_STR(char, 16);
}

void Archive::ConvertUInt64ToString(UInt64 val, char* s)
{
    if (val <= static_cast<UInt32>(0xFFFFFFFF))
    {
        ConvertUInt32ToString(static_cast<UInt32>(val), s);
        return;
    }
    CONVERT_INT_TO_STR(char, 24);
}


u16string Archive::getFileName(const UInt32& i, UInt16*& temp, size_t& tempSize)
{
    u16string file_name;
    size_t len = SzArEx_GetFileNameUtf16(&db, i, nullptr);

    if (len > tempSize)
    {
        SzFree(nullptr, temp);
        tempSize = len;
        temp = reinterpret_cast<UInt16*>(SzAlloc(nullptr, tempSize * sizeof(temp[0])));
        if (temp == nullptr)
        {
            res = SZ_ERROR_MEM;
        }

    }

    if (res != SZ_ERROR_MEM)
    {
        size_t len1 = SzArEx_GetFileNameUtf16(&db, i, temp);

        file_name = u16string(reinterpret_cast<char16_t*>(temp), len1);
    }

    return file_name;
}

void Archive::printPercent(const size_t& processedSize)
{
    //Print percent
    char s[32];
    unsigned size;
    char c = '%';

    if (Total != 0)
    {
        Completed += processedSize;
        val = Completed * max_percent / Total;
    }
    ConvertUInt64ToString(val, s);
    size = static_cast<unsigned>(strlen(s));
    s[size++] = c;
    s[size] = 0;

    Print(" (");
    Print(s);
    Print(")");

}


UInt64 Archive::getPercent()
{
    return val;
}

UInt64 Archive::getMaxPercent()
{
    return max_percent;
}

SRes Archive::fileList(std::list<std::wstring>& file_list)
{
    Print("\n7z Decoder " MY_VERSION_CPU1 " : " MY_COPYRIGHT_DATE1 "\n\n");

    for (UInt32 i = 0; i < db.NumFiles; i++)
    {
        unsigned isDir = SzArEx_IsDir(&db, i);

        u16string file_name = getFileName(i, temp, tempSize);

        if (file_name.empty() == true) break;

        char attr[8], s[32], t[32];
        UInt64 fileSize;

        GetAttribString(SzBitWithVals_Check(&db.Attribs, i) ? db.Attribs.Vals[i] : 0, static_cast<bool>(isDir), attr);

        fileSize = SzArEx_GetFileSize(&db, i);
        UInt64ToStr(fileSize, s, 10);

        if (SzBitWithVals_Check(&db.MTime, i))
        {
            ConvertFileTimeToString(&db.MTime.Vals[i], t);
        }
        else
        {
            size_t j;
            for (j = 0; j < 19; j++)
                t[j] = ' ';
            t[j] = '\0';
        }

        Print(t);
        Print(" ");
        Print(attr);
        Print(" ");

        Print(s);


        Print("  ");
        res = PrintString(temp);

        if (res != SZ_OK)
            break;


        file_list.push_back(wstring(file_name.begin(), file_name.end()));


        if (isDir)
            Print("/");
        PrintLF();
        continue;

    }//end for (i = 0; i < db.NumFiles; i++)


    return finish();
}



void Archive::calcTotal(const std::list<std::wstring>& files, const std::list<std::wstring>& paths)
{
    Print("\n7z Decoder " MY_VERSION_CPU1 " : " MY_COPYRIGHT_DATE1 "\n\n");

    this->file_list = files;
    this->path_list = paths;

    //Calculation of the size of the unpacked files
    for (UInt32 i = 0; i < db.NumFiles; i++)
    {
        UInt64 fileSize;
        fileSize = SzArEx_GetFileSize(&db, i);

        u16string file_name = getFileName(i, temp, tempSize);

        if (file_name.empty() == true) break;

        //We look for in the list
        wstring  file_name1 = wstring(file_name.begin(), file_name.end());
        if (std::find(file_list.begin(), file_list.end(), file_name1) != file_list.end())
        {
            importantTotalUnpacked += fileSize;
        }

    }

    Total = importantTotalUnpacked; // the size of the unpacked archive


   /*
     if you need cache, use these 3 variables.
     if you use external function, you can make these variable as static.
    */
    blockIndex = 0xFFFFFFFF; /* it can have any value before first call (if outBuffer = 0) */
    outBuffer = nullptr; /* it must be 0 before first call for each new archive. */
    outBufferSize = 0;  /* it can have any value before first call (if outBuffer = 0) */

    return;
}



SRes Archive::extractionFile(const UInt32& i)
{
    size_t offset = 0;
    size_t outSizeProcessed = 0;

    unsigned isDir = SzArEx_IsDir(&db, i);

    u16string file_name = getFileName(i, temp, tempSize);

    if (file_name.empty() == true)
    {
        return res;
    }

    //The file which needs to be derived is found
    bool extract = false;
    unsigned int n = 0;
    u16string source1;

    //We look for in the list
    wstring  file_name1 = wstring(file_name.begin(), file_name.end());

    for (std::list<wstring>::iterator it = file_list.begin(); it != file_list.end(); ++it)
    {
        if (*it == file_name1)
        {
            extract = true;

            break;
        }
        n++;
    }


    if (extract == true)
    {
        Print("Extracting ");
        res = PrintString(temp);
        if (res != SZ_OK)
        {
            return res;
        }

        if (isDir)
        {
            Print("/");
        }
        else
        {
            res = SzArEx_Extract(&db, &lookStream.vt, i,
                &blockIndex, &outBuffer, &outBufferSize,
                &offset, &outSizeProcessed,
                &allocImp, &allocTempImp);

            if (res != SZ_OK)
            {
                return res;
            }
        }

        if (outSizeProcessed == 0) return res;

        CSzFile outFile;
        size_t processedSize;
        size_t j;
        const UInt16* destPath;
        std::u16string  destination1;


        //To take file name from a path

        std::u16string file;

        file = file_name.substr(file_name.rfind('/') + 1);

        std::u16string str;


        std::list<wstring>::iterator it1 = path_list.begin();
        std::advance(it1, n);
        str = std::u16string((*it1).begin(), (*it1).end());

        str.resize(str.length() + 1);
        str.back() = '/';

        destination1 = str + file;

        //To create folders
        for (j = 0; j < destination1.length(); j++)
        {
            if ((destination1.at(j) == '/') ||
                (destination1.at(j) == '\\'))
            {
                destination1.at(j) = 0;
                MyCreateDir(reinterpret_cast<const UInt16*>(destination1.c_str()));
                destination1.at(j) = CHAR_PATH_SEPARATOR;
            }
        }

        destPath = reinterpret_cast<const UInt16*>(destination1.c_str());

        if ((isDir) && (path_list.empty() == true))
        {
            MyCreateDir(destPath);
            PrintLF();
            return res;
        }
        else
        {
            if (OutFile_OpenUtf16(&outFile, destPath))
            {
                PrintString(destPath);
                PrintError("can not open output file");
                res = SZ_ERROR_FAIL;
                return res;
            }
        }

        processedSize = outSizeProcessed;

        printPercent(processedSize);

        //Saving of the unpacked file
        if ((File_Write(&outFile, outBuffer + offset, &processedSize) != 0) || (processedSize != outSizeProcessed))
        {
            PrintError("can not write output file");
            res = SZ_ERROR_FAIL;
            return res;
        }


        if (File_Close(&outFile))
        {
            PrintError("can not close output file");
            res = SZ_ERROR_FAIL;
            return res;
        }


        PrintLF();

    }

    return res;
}

bool Archive::is_finish()
{
    bool ret = false;
    if ((Total > 0) && (Completed == Total))
    {
        ret = true;
    }

    return ret;
}

SRes Archive::finish()
{
    ISzAlloc_Free(&allocImp, outBuffer);

    if (res == SZ_OK)
    {
        Print("\nEverything is Ok\n");

        return res;
    }

    if (res == SZ_ERROR_UNSUPPORTED)
        PrintError("decoder doesn't support this archive");
    else if (res == SZ_ERROR_MEM)
        PrintError("can not allocate memory");
    else if (res == SZ_ERROR_CRC)
        PrintError("CRC error");
    else
    {
        char s[32];
        UInt64ToStr(static_cast<UInt64>(res), s, 0);
        PrintError(s);
    }

    val = max_percent;

    return res;
}

UInt32 Archive::getNumFiles()
{
    return db.NumFiles;
}



string Archive::ConvertToString(const wstring& str)
{
    const UInt16* s = reinterpret_cast<const UInt16*>(str.c_str());

    string str1;
    CBuf buf;
    SRes res;
    Buf_Init(&buf);
    res = Utf16_To_Char(&buf, s
#ifndef _USE_UTF8
        , CP_OEMCP
#endif
    );
    if (res == SZ_OK)
    {
        str1 = reinterpret_cast<char*>(buf.data);
    }
    Buf_Free(&buf, &g_Alloc);
    return str1;
}

