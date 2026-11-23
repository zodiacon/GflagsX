#pragma once

#define WINVER			0x0A00
#define _WIN32_WINNT	0x0A00
#define _WIN32_IE		0x0A00
#define _RICHEDIT_VER	0x0500
// CDialogResize keeps its layout at 96 DPI and redoes it for a new DPI (the app is per-monitor DPI aware)
#define _WTL_DPI_SUPPORT

#define NOMINMAX
#include <windows.h>

#include <atlbase.h>
#include <atlapp.h>

extern CAppModule _Module;

#include <atlwin.h>
#include <atlstr.h>
#include <atlframe.h>
#include <atlctrls.h>
#include <atldlgs.h>
#include <atlctrlx.h>
#include <atltheme.h>
#include <atlsplit.h>
#include <atlctrlw.h>
#include <atltypes.h>
#include <atlcrack.h>

#include <ShellScalingApi.h>
#include <shellapi.h>
#include <ShlObj.h>

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <algorithm>
#include <format>
#include <functional>
#include <expected>
#include <span>
#include <cstdint>

#include <wil\resource.h>

#if defined _M_IX86
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='x86' publicKeyToken='6595b64144ccf1df' language='*'\"")
#elif defined _M_X64
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='amd64' publicKeyToken='6595b64144ccf1df' language='*'\"")
#else
  #pragma comment(linker, "/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")
#endif
