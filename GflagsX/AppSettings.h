#pragma once

#include <Settings.h>

// persisted under HKCU\Software\ScorpioSoftware\GflagsX; the first instance is the singleton (AppSettings::Get())
struct AppSettings : Settings {
	BEGIN_SETTINGS(AppSettings)
		SETTING(DarkMode, 0, SettingType::Bool);
		SETTING(ToolBar, 1, SettingType::Bool);		// View > Toolbar
		SETTING(StatusBar, 1, SettingType::Bool);	// View > Status Bar
		SETTING(ActiveTab, 0, SettingType::Int32);	// ViewType: the tab shown last
		// the Image tab's: the images' share of the width, and the flags' share of the rest, in percent
		SETTING(ImagesSplitterPos, 30, SettingType::Int32);
		SETTING(FlagsSplitterPos, 55, SettingType::Int32);
	END_SETTINGS

	DEF_SETTING(DarkMode, int)
	DEF_SETTING(ToolBar, int)
	DEF_SETTING(StatusBar, int)
	DEF_SETTING(ActiveTab, int)
	DEF_SETTING(ImagesSplitterPos, int)
	DEF_SETTING(FlagsSplitterPos, int)

	static constexpr PCWSTR MainWindowPlacement = L"MainWindowPlacement";
	static constexpr PCWSTR RegistryPath = L"Software\\ScorpioSoftware\\GflagsX";
};
