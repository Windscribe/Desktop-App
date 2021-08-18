//---------------------------------------------------------------------------

#ifndef version_osH
#define version_osH
//---------------------------------------------------------------------------
#include "Windows.h"
#include <stdio.h>
#include "logger.h"
#include "directory.h"
#include "registry.h"
#include <VersionHelpers.h>

class VersionOS
{
	public:
		static bool isWin64();
};

#endif
