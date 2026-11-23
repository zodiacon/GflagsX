// GflagsX.cpp : main source file for GflagsX.exe
//

#include "pch.h"
#include "MainFrm.h"
#include "AppSettings.h"
#include <WTLHelper.h>

CAppModule _Module;
AppSettings _Settings;

static int Run(int nCmdShow) {
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	CMainFrame wndMain;
	if (wndMain.CreateEx() == nullptr) {
		ATLTRACE(_T("Main window creation failed!\n"));
		return 0;
	}

	// the last position, but never minimized: as it was before it was minimized
	auto& settings = AppSettings::Get();
	if (auto wp = settings.GetBinary<WINDOWPLACEMENT>(AppSettings::MainWindowPlacement); wp && (wp->showCmd == SW_SHOWMINIMIZED || wp->showCmd == SW_MINIMIZE)) {
		auto placement = *wp;
		placement.showCmd = (placement.flags & WPF_RESTORETOMAXIMIZED) ? SW_SHOWMAXIMIZED : SW_SHOWNORMAL;
		settings.Set(AppSettings::MainWindowPlacement, placement);
	}
	if (!settings.LoadWindowPosition(wndMain, AppSettings::MainWindowPlacement))
		wndMain.ShowWindow(nCmdShow);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int nCmdShow) {
	HRESULT hRes = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES | ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES);	// SysLink: the About box

	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	if (AppSettings::Get().Load(AppSettings::RegistryPath))
		WTLHelper::InitDarkMode(AppSettings::Get().DarkMode() ? DarkModeKind::Dark : DarkModeKind::Light);
	else
		WTLHelper::InitDarkMode();

	int nRet = Run(nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
