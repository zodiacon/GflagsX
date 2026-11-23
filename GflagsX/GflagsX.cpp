// GflagsX.cpp : main source file for GflagsX.exe
//

#include "pch.h"
#include "MainFrm.h"
#include "AppSettings.h"
#include "CommandLine.h"
#include "FlagStore.h"
#include "Helpers.h"
#include <WTLHelper.h>
#include <shellapi.h>

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

namespace {
	// the command line's output: to stdout where it's redirected (a file, a pipe), to the console GflagsX was started
	// from, or (started from neither: Explorer, Run, an elevation's new console) in a message box
	void Output(std::wstring const& text, bool error) {
		auto line = text + L"\r\n";
		auto out = ::GetStdHandle(error ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
		if (out && out != INVALID_HANDLE_VALUE && (::GetFileType(out) == FILE_TYPE_DISK || ::GetFileType(out) == FILE_TYPE_PIPE)) {
			auto size = ::WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), nullptr, 0, nullptr, nullptr);
			std::string utf8(size, 0);
			::WideCharToMultiByte(CP_UTF8, 0, line.c_str(), (int)line.size(), utf8.data(), size, nullptr, nullptr);
			DWORD written;
			::WriteFile(out, utf8.data(), (DWORD)utf8.size(), &written, nullptr);
			return;
		}
		static bool attached = ::AttachConsole(ATTACH_PARENT_PROCESS);
		if (attached) {
			wil::unique_hfile console(::CreateFile(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, nullptr, OPEN_EXISTING, 0, nullptr));
			DWORD written;
			// on a line of its own: the shell's prompt is there already (it doesn't wait for a GUI program)
			auto shown = L"\r\n" + line;
			if (console && ::WriteConsole(console.get(), shown.c_str(), (DWORD)shown.size(), &written, nullptr))
				return;
		}
		::MessageBox(nullptr, text.c_str(), L"GFlagsX", MB_OK | (error ? MB_ICONERROR : MB_ICONINFORMATION));
	}

	std::wstring TargetTitle(CommandLine::Command const& command) {
		switch (command.Target) {
			case GFlags::Target::Registry: return L"Registry (the next boot)";
			case GFlags::Target::Kernel: return L"Kernel (the running system)";
		}
		return L"Image " + command.Image;
	}

	// runs a command line's command: its exit code is 0, or the Win32 error (1 for the command line's own)
	int RunCommand(CommandLine::Command const& command) {
		using namespace CommandLine;
		using namespace FlagStore;
		if (command.What == Action::Usage) {
			Output(Usage(), false);
			return 0;
		}
		auto& where = Locations::System();
		auto fail = [&](PCWSTR what, DWORD error) {
			Output(std::format(L"{} failed: {}", what, Helpers::ErrorText(error)), true);
			return (int)error;
		};

		// the value now
		uint32_t value = 0;
		ImageSettings image;
		bool imageExists = true;
		if (command.Target == GFlags::Target::Image) {
			auto read = ReadImage(where, command.Image);
			if (!read && read.error() != ERROR_FILE_NOT_FOUND)
				return fail(L"Reading the image's settings", read.error());
			imageExists = read.has_value();
			image = read.value_or(ImageSettings{});
			value = image.GlobalFlag;
		}
		else {
			auto read = command.Target == GFlags::Target::Kernel ? ReadKernelFlags() : ReadRegistryFlags(where);
			if (!read)
				return fail(L"Reading the flags", read.error());
			value = *read;
		}

		if (command.What == Action::Show) {
			if (!imageExists)
				Output(std::format(L"{}: no settings", TargetTitle(command)), false);
			else
				Output(std::format(L"{}: {}", TargetTitle(command), Describe(value, command.Target)), false);
			return 0;
		}

		auto newValue = NewValue(command, value);
		DWORD error;
		if (command.Target == GFlags::Target::Image) {
			// a new image's key is made (as gflags.exe does)
			if (!imageExists && (error = CreateImage(where, command.Image)) != ERROR_SUCCESS && error != ERROR_ALREADY_EXISTS)
				return fail(L"Making the image's key", error);
			image.GlobalFlag = newValue;
			error = WriteImage(where, command.Image, image);
		}
		else {
			error = command.Target == GFlags::Target::Kernel ? WriteKernelFlags(newValue) : WriteRegistryFlags(where, newValue);
		}
		if (error != ERROR_SUCCESS)
			return fail(L"Writing the flags", error);

		auto text = std::format(L"{}: {}", TargetTitle(command), Describe(newValue, command.Target));
		// the kernel may not take some
		if (command.Target == GFlags::Target::Kernel) {
			if (auto now = ReadKernelFlags(); now && *now != newValue)
				text += std::format(L"\r\nThe kernel has {} (it didn't take {})", GFlags::FormatValue(*now), GFlags::FormatValue(newValue & ~*now));
		}
		Output(text, false);
		return 0;
	}
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR /*lpstrCmdLine*/, int nCmdShow) {
	HRESULT hRes = ::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES | ICC_LINK_CLASS | ICC_LISTVIEW_CLASSES);	// SysLink: the About box

	hRes = _Module.Init(nullptr, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	// arguments: the command line's work, without the UI
	int argc = 0;
	auto argv = ::CommandLineToArgvW(::GetCommandLine(), &argc);
	std::vector<std::wstring> args(argv + std::min(argc, 1), argv + argc);
	::LocalFree(argv);
	if (!args.empty()) {
		int code;
		if (auto command = CommandLine::Parse(args); command)
			code = RunCommand(*command);
		else {
			Output(command.error(), true);
			code = 1;
		}
		_Module.Term();
		::CoUninitialize();
		return code;
	}

	if (AppSettings::Get().Load(AppSettings::RegistryPath))
		WTLHelper::InitDarkMode(AppSettings::Get().DarkMode() ? DarkModeKind::Dark : DarkModeKind::Light);
	else
		WTLHelper::InitDarkMode();

	int nRet = Run(nCmdShow);

	_Module.Term();
	::CoUninitialize();

	return nRet;
}
