#include "registry.h"
#include "logger.h"

using namespace std;

#define MAX_KEY_LENGTH 255

bool Registry::InternalRegQueryStringValue(HKEY H, const wchar_t *Name, wstring &ResultStr, DWORD Type1, DWORD Type2)
{
  DWORD Typ, Size;
  unsigned int Len;
  long ErrorCode;
  bool Result = false;

  l:

  Size = 0;

  if ((RegQueryValueEx(H, Name, nullptr, &Typ, nullptr, &Size) == ERROR_SUCCESS) && ((Typ == Type1) || (Typ == Type2)))
  {
    if (Size == 0)
    {
      // It's an empty string with no null terminator.
      //  (Must handle those here since we can't pass a 0 lpData pointer on
      //  the second RegQueryValueEx call.)
      ResultStr = L"";
      Result = true;
    }
    else
    {
     // { Note: If Size isn't a multiple of sizeof(TCHAR), we have to round up
    // here so that RegQueryValueEx doesn't overflow the buffer }
      Len = (Size + (sizeof(TCHAR) - 1)) / sizeof(TCHAR);

      vector<TCHAR> S(Len);
      ErrorCode = RegQueryValueEx(H, Name, nullptr, &Typ, reinterpret_cast<LPBYTE>(&S[0]), &Size);
      if (ErrorCode == ERROR_MORE_DATA)
      {
      //  { The data must've increased in size since the first RegQueryValueEx
      //    call. Start over. }
        goto l;
      }
      if ((ErrorCode == ERROR_SUCCESS) && ((Typ == Type1) || (Typ == Type2)))
      {
      //  { If Size isn't a multiple of sizeof(S[1]), we disregard the partial
       //   character, like RegGetValue }
        Len = Size / sizeof(TCHAR);
      //  { Remove any null terminators from the end and trim the string to the
      //    returned length.
      //    Note: We *should* find 1 null terminator, but it's possible for
      //    there to be more or none if the value was written that way. }
        while ((Len !=0 ) && (S[Len-1] != 0))
        {
          Len--;
        }
      //In a REG_MULTI_SZ value, each individual string is null-terminated,
      //so add 1 null (back) to the end, unless there are no strings (Len=0)
        if ((Typ == REG_MULTI_SZ) && (Len != 0))
        {
         Len++;
        }


        S.reserve(Len);
        if ((Typ == REG_MULTI_SZ) && (Len != 0))
        {
         S[Len-1] = 0;
        }


        ResultStr = &S[0];
        Result = true;
      }
    }
  }

  return Result;
}

bool Registry::RegQueryStringValue(HKEY H, const wchar_t *SubKeyName, wstring ValueName, wstring &ResultStr)
// Queries the specified REG_SZ or REG_EXPAND_SZ registry key/value, and returns
//  the value in ResultStr. Returns true if successful. When false is returned,
//  ResultStr is unmodified.
{
    TRegView RegView;
    HKEY K, RootKey;
    wstring S, N, V;
    string DataS;
  //  DWORD Typ, ExistingTyp, Data, Size;
    //Arr: TPSVariantIFC;

    bool ret;

    CrackCodeRootKey(H, RegView, RootKey);
    S = SubKeyName;

    if (RegOpenKeyExView(RegView, RootKey, S.c_str(), 0, KEY_QUERY_VALUE, K) == ERROR_SUCCESS)
    {
       N = ValueName;
       V = ResultStr;
       ret = RegQueryStringValue1(K, N.c_str(), S);

       ResultStr = S;
       RegCloseKey(K);
     }
    else
    {
     ret = false;
    }


  return ret;

}

bool Registry::RegQueryStringValue1(HKEY H, const wchar_t *Name, wstring &ResultStr)
{
// Queries the specified REG_SZ or REG_EXPAND_SZ registry key/value, and returns
//  the value in ResultStr. Returns true if successful. When false is returned,
//  ResultStr is unmodified.
 return InternalRegQueryStringValue(H, Name, ResultStr, REG_SZ, REG_EXPAND_SZ);
}


LSTATUS Registry::RegCreateKeyExView(const TRegView &RegView, HKEY &hKey, LPCWSTR lpSubKey,
  DWORD Reserved, LPWSTR lpClass,DWORD dwOptions, REGSAM samDesired,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes, PHKEY phkResult,
  LPDWORD lpdwDisposition)
{
    if (RegView == rv64Bit) {
        samDesired = samDesired | KEY_WOW64_64KEY;
    }
    else if (RegView == rv32Bit) {
        samDesired = samDesired | KEY_WOW64_32KEY;
    }
    
    return RegCreateKeyEx(hKey, lpSubKey, Reserved, lpClass, dwOptions, samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
}


LSTATUS Registry::RegOpenKeyExView(const TRegView RegView, const HKEY hKey, LPCWSTR lpSubKey, DWORD ulOptions, REGSAM samDesired, HKEY &phkResult)
{
    if (RegView == rv64Bit) {
        samDesired = samDesired | KEY_WOW64_64KEY;
    }
    else if (RegView == rv32Bit) {
        samDesired = samDesired | KEY_WOW64_32KEY;
    }

    return RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, &phkResult);
}

void Registry::CrackCodeRootKey(HKEY CodeRootKey, TRegView &RegView, HKEY &RootKey)
{
 int CodeRootKeyFlagMask  = 0x7F000000;
 int CodeRootKeyFlag32Bit = 0x01000000;
 int CodeRootKeyFlag64Bit = 0x02000000;
 int CodeRootKeyValidFlags = CodeRootKeyFlag32Bit | CodeRootKeyFlag64Bit;
 TRegView InstallDefaultRegView  = rvDefault;
//  { Allow only predefined key handles (8xxxxxxx). Can't accept handles to
//    open keys because they might have our special flag bits set.
//    Also reject unknown flags which may have a meaning in the future. }
 int CodeRootKey1 = reinterpret_cast<int>(CodeRootKey);


 if(((CodeRootKey1 >> 31) != 1) || (((CodeRootKey1 & CodeRootKeyFlagMask)!=0) && ((~CodeRootKeyValidFlags) != 0)))
 {
    // I commented this out because it is logging HKEY_LOCAL_MACHINE and HKEY_CURRENT_USER as invalid.
     //Log::instance().out("(registry) Invalid RootKey value (%d)", CodeRootKey1);
 }

 if ((CodeRootKey!=nullptr) && (CodeRootKeyFlag32Bit != 0))
 {
  RegView = rv32Bit;
 }
 else
 {
  if ((CodeRootKey!=nullptr) && (CodeRootKeyFlag64Bit != 0))
  {
   RegView = rv64Bit;
  }
  else
  {
   RegView = InstallDefaultRegView;
  }
 }

 RootKey = reinterpret_cast<HKEY>((CodeRootKey1 & (~CodeRootKeyFlagMask)));
}

bool Registry::RegWriteStringValue(HKEY H, wstring SubKeyName, wstring ValueName, wstring ValueData)
{
  bool ret;

//PStart: Cardinal;
//ExistingFilename: String;
TRegView RegView;
HKEY K, RootKey;
wstring S, N, V;
string DataS;
DWORD Typ, ExistingTyp;//, Data, Size;
//Arr: TPSVariantIFC;

CrackCodeRootKey(H, RegView, RootKey);
S = SubKeyName;

if(RegCreateKeyExView(RegView, RootKey, S.c_str(), 0, nullptr, REG_OPTION_NON_VOLATILE, KEY_QUERY_VALUE | KEY_SET_VALUE, nullptr, &K, nullptr) == ERROR_SUCCESS)
{
  N = ValueName;
  V = ValueData;
  if ((RegQueryValueEx(K, N.c_str(), nullptr, &ExistingTyp, nullptr, nullptr) == ERROR_SUCCESS) && (ExistingTyp == REG_EXPAND_SZ))
    Typ = REG_EXPAND_SZ;
  else
    Typ = REG_SZ;
  if (RegSetValueEx(K, N.c_str(), 0, Typ, reinterpret_cast<const BYTE *>(V.c_str()), (V.length()+1)*sizeof(V[0])) ==ERROR_SUCCESS)
    ret = true;
  else
    ret = false;

  RegCloseKey(K);
}
else
{
 ret = false;
}

 return ret;
}


bool Registry::RegDeleteValue1(HKEY H,const wchar_t *SubKeyName,wstring ValueName)
{
  bool ret=true;

  TRegView RegView;
  HKEY K, RootKey;
  wstring S, N, V;
  string DataS;
  //DWORD Typ, ExistingTyp, Data, Size;

  CrackCodeRootKey(H, RegView, RootKey);

  S = SubKeyName;
  if(RegOpenKeyExView(RegView, RootKey, S.c_str(), 0, KEY_SET_VALUE, K) == ERROR_SUCCESS)
    {
      N = ValueName;

      if(RegDeleteValue(K, N.c_str()) == ERROR_SUCCESS)
      {
       ret = false;
      }

      RegCloseKey(K);
    }
    else
    {
     ret = false;
    }
  return ret;
}


LSTATUS Registry::RegDeleteKeyView(const TRegView RegView, const HKEY Key,  const wchar_t *Name)
{
 LSTATUS Result;

 if (RegView != rv64Bit)
 {
  Result = RegDeleteKey(Key, Name);
 }
 else
 {
  Result = RegDeleteKeyEx(Key, Name, KEY_WOW64_64KEY, 0);
 }

 return Result;
}

LSTATUS Registry::RegDeleteKeyIncludingSubkeys1(TRegView RegView, const HKEY Key, const wchar_t *Name)
//{ Deletes the specified key and all subkeys.
 // Returns ERROR_SUCCESS if the key was successful deleted. }
{
  HKEY H;
  wchar_t *KeyName;
  DWORD I, KeyNameCount;
  int ErrorCode;

  unsigned int len = 0;
  LSTATUS Result;

  if ((Name == nullptr) || (Name[0] == 0))
  {
   return ERROR_INVALID_PARAMETER;
  }

    if (RegOpenKeyExView(RegView, Key, Name, 0, KEY_ENUMERATE_SUB_KEYS, H) == ERROR_SUCCESS)
    {
     // try
     //   {
        len = 256;

        KeyName = new wchar_t[len];//
        I = 0;
        while (1)
        {
          KeyNameCount = len;
          ErrorCode = RegEnumKeyEx(H, I, KeyName, &KeyNameCount, nullptr, nullptr, nullptr, nullptr);
          if (ErrorCode == ERROR_MORE_DATA)
          {
           // { Double the size of the buffer and try again }
           if (len >= 65536)
          {
             // { Sanity check: If we tried a 64 KB buffer and it's still saying
            //    there's more data, something must be seriously wrong. Bail. }
              break;
            }

           len = len * 2;

            delete [] KeyName;
            KeyName = new wchar_t[len];

            continue;
          }
          if (ErrorCode != ERROR_SUCCESS)
          {
            break;
          }
          if (RegDeleteKeyIncludingSubkeys1(RegView, H, KeyName) != ERROR_SUCCESS)
          {
            I++;
          }
          }
    //}
        //// __finally
             {
           RegCloseKey(H);
           delete [] KeyName;
         //}
    }

   }

  Result = RegDeleteKeyView(RegView, Key, Name);

  return Result;
}

bool Registry::RegDeleteKeyIncludingSubkeys(HKEY RootKey, const wstring SubkeyName)
{
 TRegView RegView;
 CrackCodeRootKey(RootKey, RegView, RootKey);

 return (RegDeleteKeyIncludingSubkeys1(RegView, RootKey, SubkeyName.c_str()) == ERROR_SUCCESS);
}

wstring Registry::GetRegRootKeyName(const HKEY RootKey)
{
    wstring Result;

    switch(reinterpret_cast<unsigned long>(RootKey))
    {
   //case (LONG)HKEY_AUTO: //InternalError('GetRegRootKeyName called for HKEY_AUTO');
    case 0x80000000: Result = L"HKEY_CLASSES_ROOT"; break;
    case 0x80000001: Result = L"HKEY_CURRENT_USER"; break;
    case 0x80000002: Result = L"HKEY_LOCAL_MACHINE"; break;
    case 0x80000003: Result = L"HKEY_USERS"; break;
    case 0x80000004: Result = L"HKEY_PERFORMANCE_DATA"; break;
    case 0x80000005: Result = L"HKEY_CURRENT_CONFIG"; break;
    case 0x80000006: Result = L"HKEY_DYN_DATA"; break;
    default:
    // { unknown - shouldn't get here }
    wchar_t buffer [50];
    swprintf_s(buffer,L"%x",reinterpret_cast<LONG>(RootKey));

    Result = buffer;
    }

  return Result;
}

std::vector<std::wstring> Registry::regGetSubkeyChildNames(HKEY h, const wchar_t * subkeyName)
{
    std::vector<std::wstring> registryInterfaces;
    HKEY hKey;

    if (RegOpenKeyEx(h, subkeyName, 0, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
        DWORD    cbName;                   // size of name string
        DWORD    cSubKeys = 0;               // number of subkeys

        if (RegQueryInfoKey(hKey, NULL, NULL, NULL, &cSubKeys, NULL, NULL, NULL, NULL, NULL, NULL, NULL) == ERROR_SUCCESS)
        {
            for (DWORD i = 0; i < cSubKeys; i++)
            {
                cbName = MAX_KEY_LENGTH; // magic -- dont try to initialize with this value
                LONG error = RegEnumKeyEx(hKey, i, achKey, &cbName, NULL, NULL, NULL, NULL);
                
                if (error == ERROR_SUCCESS)
                {
                    registryInterfaces.push_back(std::wstring(achKey));
                }
            }
        }
        
        RegCloseKey(hKey);
    }

    return registryInterfaces;
}

bool Registry::regDeleteProperty(HKEY h, const wchar_t * subkeyName, std::wstring valueName)
{
    bool ret = false;
    HKEY K;

    if (RegOpenKeyEx(h, subkeyName, 0, KEY_SET_VALUE, &K) == ERROR_SUCCESS)
    {
        ret = RegDeleteValue(K, valueName.c_str()) == ERROR_SUCCESS;
        RegCloseKey(K);
    }

    return ret;
}

bool Registry::regHasSubkeyProperty(HKEY h, const wchar_t * subkeyName, std::wstring valueName)
{
    bool result = false;
    HKEY hKey;

    if (RegOpenKeyEx(h, subkeyName, NULL, KEY_READ, &hKey) == ERROR_SUCCESS)
    {
        result = RegQueryValueEx(hKey, valueName.c_str(), NULL, REG_NONE, NULL, NULL) == ERROR_SUCCESS; // contents not required
        RegCloseKey(hKey);
    }

    return result;
}
