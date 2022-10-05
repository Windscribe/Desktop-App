#pragma once
class ShellExecuteAsUser
{
public:
	static void shellExecuteFromExplorer(PCWSTR pszFile, PCWSTR pszParameters = nullptr, PCWSTR pszDirectory = nullptr,
			PCWSTR pszOperation = nullptr, int nShowCmd = SW_SHOWNORMAL);

private:
	static void getDesktopAutomationObject(REFIID riid, void **ppv);
	static void findDesktopFolderView(REFIID riid, void **ppv);
};

