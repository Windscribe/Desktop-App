#include "remove_directory.h"

using namespace std;

RemoveDirectory1::RemoveDirectory1()
{

}


bool RemoveDirectory1::DelTree(const bool DisableFsRedir, const wstring Path,
  const bool IsDir, const bool DeleteFiles, const bool DeleteSubdirsAlso, const bool breakOnError,
  const P_DeleteDirProc DeleteDirProc, const P_DeleteFileProc DeleteFileProc,
  const void *Param)
//{ Deletes the specified directory including all files and subdirectories in
//  it (including those with hidden, system, and read-only attributes). Returns
//  true if it was able to successfully remove everything. If breakOnError is
//  set to true it will stop and return false the first time a delete failed or
//  DeleteDirProc/DeleteFileProc returned false.  }
{
  wstring BasePath, FindSpec;
  HANDLE H;
  WIN32_FIND_DATA FindData;
  wstring S;

  bool Result = true;
  if ((DeleteFiles) &&
          (!IsDir || (redirection.IsDirectoryAndNotReparsePointRedir(DisableFsRedir, Path)==true)))
  {
    if (IsDir)
    {
     BasePath = path.AddBackslash(Path);
     FindSpec = BasePath + L"*";
    }
    else
    {
      BasePath = path.PathExtractPath(Path);
      FindSpec = Path;
    }
    H = redirection.FindFirstFileRedir(DisableFsRedir, FindSpec, FindData);
    if (H != INVALID_HANDLE_VALUE)
    {
      //try{
        while(1)
        {
          S = FindData.cFileName;
          if ((S != L".") && (S != L".."))
          {
              if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_READONLY) != 0)
              {
//              { Strip the read-only attribute if this is a file, or if it's a
//                directory and we're deleting subdirectories also }
               if (((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0) || (DeleteSubdirsAlso==true))
               {
                redirection.SetFileAttributesReDir(DisableFsRedir, BasePath + S, FindData.dwFileAttributes & static_cast<DWORD>(~FILE_ATTRIBUTE_READONLY));
               }
              }

            if ((FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
            {
              if (DeleteFileProc!=nullptr)
              {
                if (!DeleteFileProc(DisableFsRedir, BasePath + S)==true)
                {
                  Result = false;
                }
              }
              else
              {
                if (!redirection.DeleteFileRedir(DisableFsRedir, BasePath + S)==true)
                {
                  Result = false;
                }
              }
            }
            else
            {
              if(DeleteSubdirsAlso==true)
              {
                if(!DelTree(DisableFsRedir, BasePath + S, true, true, true, breakOnError,
                   DeleteDirProc, DeleteFileProc, Param)==true)
                {
                 Result = false;
                }
              }
            }
          }
         if((breakOnError && !Result) || !FindNextFile(H, &FindData)) break;
         }


    //          }
    //  __finally
     // {
        FindClose(H);
      //}
   }
  }
  if ((!breakOnError || Result) && IsDir)
  {
    if (DeleteDirProc!=nullptr)
    {
      if (DeleteDirProc(DisableFsRedir,Path)==false)
        Result = false;
    }
    else
    {
      if (redirection.RemoveDirectoryReDir(DisableFsRedir, Path)==false)
        Result = false;
    }
  }

  return Result;
}




bool RemoveDirectory1::RemoveDir(wstring dir)
 {
  bool ScriptFuncDisableFsRedir = false;
  return redirection.RemoveDirectoryReDir(ScriptFuncDisableFsRedir, dir);
 }
bool RemoveDirectory1::DeleteDir(const bool DisableFsRedir, const wstring DirName,
  std::list<wstring> *DirsNotRemoved, std::list<wstring> *RestartDeleteDirList)
{
  const wchar_t drfalse = '0';
  const wchar_t drtrue = '1';

  wchar_t DirsNotRemovedPrefix[2] = {drfalse, drtrue};

  DWORD Attribs, LastError;

  bool Result;


  Attribs = redirection.GetFileAttributesRedir(DisableFsRedir, DirName);
  //{ Does the directory exist? }
  if ((Attribs != 0xFFFFFFFF) && (Attribs && FILE_ATTRIBUTE_DIRECTORY != 0))
  {
    Log::instance().out(L"(remove_directory) Deleting directory: %s", DirName.c_str());


    // { If the directory has the read-only attribute, strip it first }
    if ((Attribs & FILE_ATTRIBUTE_READONLY) != 0)
    {
      if (((Attribs & FILE_ATTRIBUTE_REPARSE_POINT) != 0) || IsDirEmpty(DisableFsRedir, DirName))
      {
        if(redirection.SetFileAttributesReDir(DisableFsRedir, DirName, Attribs & !FILE_ATTRIBUTE_READONLY)==true)
        {
         Log::instance().out(L"(remove_directory) Stripped read-only attribute.");
        }
        else
        {
         Log::instance().out(L"(remove_directory) Failed to strip read-only attribute.");
        }
      }
      else
      {
       Log::instance().out(L"(remove_directory) Not stripping read-only attribute because the directory does not appear to be empty.");
      }
    }
    Result = redirection.RemoveDirectoryReDir(DisableFsRedir, DirName);

    if (Result==false)
    {
      LastError = GetLastError();
      if (DirsNotRemoved!=nullptr)
      {
        wchar_t buffer[100];
        swprintf_s(buffer, L"%lu",GetLastError());
        Log::instance().out(L"(remove_directory) Failed to delete directory "+ wstring(buffer)+L". Will retry later.");
        wstring str = wstring(&DirsNotRemovedPrefix[DisableFsRedir]) + DirName;
        if(std::find(DirsNotRemoved->begin(), DirsNotRemoved->end(), str) == DirsNotRemoved->end())
            {
             DirsNotRemoved->push_back(str);
            }

      }
      else
      {
       if((RestartDeleteDirList!=nullptr) &&
         ListContainsPathOrSubdir(RestartDeleteDirList, DirName))
        {
       // { Note: This code is NT-only; I don't think it's possible to
       //   restart-delete directories on Windows 9x. }

        wchar_t buffer[100];
        swprintf_s(buffer, L"%lu",LastError);
        Log::instance().out(L"(remove_directory) Failed to delete directory "+wstring(buffer)+L". Will delete on restart (if empty).");
        RestartDeleteDir(DisableFsRedir, DirName);
       }
       else
       {
        wchar_t buffer[100];
        swprintf_s(buffer, L"%lu",LastError);
        Log::instance().out(L"(remove_directory) Failed to delete directory "+wstring(buffer)+L".");
       }
      }


    }
  }
  else
  {
    Result = true;
  }

  return Result;
}


bool RemoveDirectory1::IsDirEmpty(const bool DisableFsRedir, const wstring Dir)
{
//{ Returns true if Dir contains no files or subdirectories.
//  Note: If Dir does not exist or lacks list permission, false will be
//  returned. }

  bool Result = true;
  HANDLE H;
  WIN32_FIND_DATA FindData;

  H = redirection.FindFirstFileRedir(DisableFsRedir, path.AddBackslash(Dir) + L"*", FindData);
  if (H != INVALID_HANDLE_VALUE)
  {
   // try{
      Result = true;
      while(1)
      {
          if (FindData.dwFileAttributes && (FILE_ATTRIBUTE_DIRECTORY == 0))
          {
        //  { Found a file }
          Result = false;
          break;
        }
        if ((wcscmp(FindData.cFileName, L".") != 0) && (wcscmp(FindData.cFileName, L"..") != 0))
        {
        //  { Found a subdirectory }
          Result = false;
          break;
        }
        if (FindNextFile(H, &FindData)==false)
        {
          if (GetLastError() != ERROR_NO_MORE_FILES)
          {
           // { Exited the loop early due to some unexpected error. The directory
         //     might not be empty, so return false }
            Result = false;
          }
          break;
        }
      }
   // }
   // __finally
   // {
     FindClose(H);
   // }
  }
  else
  {
   // { The directory may not exist, or it may lack list permission }
    Result = false;
  }

  return Result;
}


void RemoveDirectory1::RestartDeleteDir(const bool DisableFsRedir, wstring Dir)
{
 Dir = path.PathExpand(Dir);
 if (DisableFsRedir==false)
    {
//{ Work around WOW64 bug present in the IA64 and x64 editions of Windows
//XP (3790) and Server 2003 prior to SP1 RC2: MoveFileEx writes filenames
//to the registry verbatim without mapping system32->syswow64. }
     Dir = directory.ReplaceSystemDirWithSysWow64(Dir.c_str());
    }

 if (redirection.MoveFileExRedir(DisableFsRedir, Dir, L"", MOVEFILE_DELAY_UNTIL_REBOOT)==false)
 {
  char buffer[100];
  sprintf_s(buffer, "MoveFileEx failed (%lu).",GetLastError());

  Log::instance().out(buffer);
 }

}


bool RemoveDirectory1::ListContainsPathOrSubdir(std::list<wstring> *List,  const wstring Path)
{
//{ Returns true if List contains Path or a subdirectory of Path }
  wstring SlashPath;
  unsigned int SlashPathLen;
 // bool Result = false;

  SlashPath = path.AddBackslash(Path);
  SlashPathLen = SlashPath.length();
  if (SlashPathLen > 0)
   {
  //{ ...sanity check }
    for (std::list<wstring>::iterator it=List->begin(); it != List->end(); ++it)
    {
      if (*it == Path)
      {
       return true;
      }
      if (((*it).length() > SlashPathLen) && CompareMem(reinterpret_cast<const void *>((*it).c_str()), reinterpret_cast<const void*>(SlashPath.c_str())))
       {
        return true;
       }
     }
   }
 return false;
}


bool RemoveDirectory1::CompareMem(const void *P1, const void *P2)
{
    __asm
     {
        PUSH    ESI
        PUSH    EDI
        MOV     ESI,P1
        MOV     EDI,P2
        MOV     EDX,ECX
        XOR     EAX,EAX
        AND     EDX,3
        SHR     ECX,1
        SHR     ECX,1
        REPE    CMPSD
        JNE     B
        MOV     ECX,EDX
        REPE    CMPSB
        JNE     B
        INC     EAX
  B:    POP     EDI
        POP     ESI
    }
}
