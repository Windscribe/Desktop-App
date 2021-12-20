//---------------------------------------------------------------------------

#include "version_os.h"
//---------------------------------------------------------------------------


bool VersionOS::isWin64()
{
	bool is_64_bit = true;

	if(FALSE == GetSystemWow64DirectoryW(nullptr, 0u))
	{
		const DWORD last_error = GetLastError();
		if(ERROR_CALL_NOT_IMPLEMENTED == last_error)
		{
			is_64_bit = false;
		}
	}

	return is_64_bit;
}

