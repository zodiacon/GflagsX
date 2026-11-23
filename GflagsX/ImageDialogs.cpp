#include "pch.h"
#include "ImageDialogs.h"
#include "Helpers.h"
#include "PageHeap.h"
#include <WTLHelper.h>

using namespace FlagStore;

LRESULT CNewImageDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());
	CEdit(GetDlgItem(IDC_NAME)).SetLimitText(255);
	Validate();
	return TRUE;
}

void CNewImageDlg::Validate() {
	CString text;
	GetDlgItemText(IDC_NAME, text);
	m_Name = Helpers::Trim((PCWSTR)text);
	PCWSTR error = L"";
	if (!m_Name.empty() && !IsValidImageName(m_Name))
		error = L"A file name, without a folder.";
	SetDlgItemText(IDC_NAME_ERROR, error);
	GetDlgItem(IDOK).EnableWindow(!m_Name.empty() && !*error);
}

LRESULT CNewImageDlg::OnNameChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Validate();
	return 0;
}

LRESULT CNewImageDlg::OnBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	COMDLG_FILTERSPEC filters[] = { { L"Programs", L"*.exe" }, { L"All Files", L"*.*" } };
	CShellFileOpenDialog dlg(nullptr, FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST, nullptr, filters, _countof(filters));
	dlg.GetPtr()->SetTitle(L"Executable");
	SuspendResumeHook sr;
	CString path;
	if (dlg.DoModal(m_hWnd) == IDOK && SUCCEEDED(dlg.GetFilePath(path)))
		// the options are by file name, whatever the folder
		SetDlgItemText(IDC_NAME, Helpers::FileName((PCWSTR)path).c_str());
	return 0;
}

LRESULT CNewImageDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Validate();
	if (!m_Name.empty() && IsValidImageName(m_Name))
		EndDialog(wID);
	return 0;
}

LRESULT CNewImageDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

CSilentProcessExitDlg::CSilentProcessExitDlg(std::wstring image, SilentProcessExitSettings settings) :
	m_Image(std::move(image)), m_Settings(std::move(settings)) {
}

LRESULT CSilentProcessExitDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());
	SetDlgItemText(IDC_IMAGE, (L"Image: " + m_Image).c_str());

	auto& s = m_Settings;
	CheckDlgButton(IDC_IGNORESELF, s.IgnoreSelfExits);
	CheckDlgButton(IDC_LAUNCHMONITOR, (s.ReportingMode & LaunchMonitorProcess) != 0);
	CheckDlgButton(IDC_NOTIFICATION, (s.ReportingMode & Notification) != 0);
	CheckDlgButton(IDC_DUMP, (s.ReportingMode & LocalDump) != 0);
	SetDlgItemText(IDC_MONITOR, s.MonitorProcess.c_str());
	SetDlgItemText(IDC_DUMPFOLDER, s.LocalDumpFolder.c_str());
	SetDlgItemText(IDC_MODULES, Helpers::JoinLines(s.ModuleIgnoreList).c_str());

	CComboBox types(GetDlgItem(IDC_DUMPTYPE));
	// in DumpPreset order
	for (auto name : { L"Micro", L"Mini", L"Heap", L"Custom" })
		types.AddString(name);
	types.SetCurSel((int)PresetOf(s.DumpType));
	SetDlgItemText(IDC_DUMPTYPE_VALUE, std::format(L"0x{:X}", s.DumpType).c_str());
	if (s.MaxNumberOfDumpFiles)
		SetDlgItemInt(IDC_MAXDUMPS, s.MaxNumberOfDumpFiles, FALSE);

	UpdateEnabled();
	return TRUE;
}

void CSilentProcessExitDlg::UpdateEnabled() {
	bool monitor = IsDlgButtonChecked(IDC_LAUNCHMONITOR);
	for (auto id : { IDC_MONITOR_LABEL, IDC_MONITOR })
		GetDlgItem(id).EnableWindow(monitor);
	bool dump = IsDlgButtonChecked(IDC_DUMP);
	for (auto id : { IDC_DUMPFOLDER_LABEL, IDC_DUMPFOLDER, IDC_DUMPFOLDER_BROWSE, IDC_DUMPTYPE_LABEL, IDC_DUMPTYPE,
		IDC_MAXDUMPS_LABEL, IDC_MAXDUMPS, IDC_MAXDUMPS_NOTE, IDC_MODULES_LABEL, IDC_MODULES })
		GetDlgItem(id).EnableWindow(dump);
	bool custom = CComboBox(GetDlgItem(IDC_DUMPTYPE)).GetCurSel() == (int)DumpPreset::Custom;
	GetDlgItem(IDC_DUMPTYPE_VALUE).EnableWindow(dump);
	CEdit(GetDlgItem(IDC_DUMPTYPE_VALUE)).SetReadOnly(!custom);
}

void CSilentProcessExitDlg::ShowDumpType() {
	auto preset = DumpPreset(std::max(0, CComboBox(GetDlgItem(IDC_DUMPTYPE)).GetCurSel()));
	if (auto value = PresetValue(preset))
		SetDlgItemText(IDC_DUMPTYPE_VALUE, std::format(L"0x{:X}", *value).c_str());
}

LRESULT CSilentProcessExitDlg::OnDumpTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ShowDumpType();
	UpdateEnabled();
	return 0;
}

std::wstring CSilentProcessExitDlg::GetText(int id) const {
	CString text;
	GetDlgItemText(id, text);
	return Helpers::Trim((PCWSTR)text);
}

LRESULT CSilentProcessExitDlg::OnCheckChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdateEnabled();
	return 0;
}

LRESULT CSilentProcessExitDlg::OnBrowseFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CShellFileOpenDialog dlg(nullptr, FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST | FOS_PICKFOLDERS);
	dlg.GetPtr()->SetTitle(L"Dump Folder");
	SuspendResumeHook sr;
	CString path;
	if (dlg.DoModal(m_hWnd) == IDOK && SUCCEEDED(dlg.GetFilePath(path)))
		SetDlgItemText(IDC_DUMPFOLDER, path);
	return 0;
}

LRESULT CSilentProcessExitDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SilentProcessExitSettings s = m_Settings;
	s.IgnoreSelfExits = IsDlgButtonChecked(IDC_IGNORESELF);
	// the bits no check box shows are kept
	s.ReportingMode &= ~(LaunchMonitorProcess | LocalDump | Notification);
	if (IsDlgButtonChecked(IDC_LAUNCHMONITOR))
		s.ReportingMode |= LaunchMonitorProcess;
	if (IsDlgButtonChecked(IDC_DUMP))
		s.ReportingMode |= LocalDump;
	if (IsDlgButtonChecked(IDC_NOTIFICATION))
		s.ReportingMode |= Notification;
	s.MonitorProcess = GetText(IDC_MONITOR);
	s.LocalDumpFolder = GetText(IDC_DUMPFOLDER);
	s.ModuleIgnoreList = Helpers::SplitList(GetText(IDC_MODULES));

	auto refuse = [this](PCWSTR text, int id) {
		AtlMessageBox(m_hWnd, text, IDR_MAINFRAME, MB_ICONWARNING);
		GotoDlgCtrl(GetDlgItem(id));
	};
	if ((s.ReportingMode & LaunchMonitorProcess) && s.MonitorProcess.empty()) {
		refuse(L"Type the monitor process's command line.", IDC_MONITOR);
		return 0;
	}
	if ((s.ReportingMode & LocalDump) && s.LocalDumpFolder.empty()) {
		refuse(L"Type the folder for the dumps.", IDC_DUMPFOLDER);
		return 0;
	}
	// a preset's value, or Custom's as typed (MINIDUMP_TYPE flags)
	auto preset = DumpPreset(std::max(0, CComboBox(GetDlgItem(IDC_DUMPTYPE)).GetCurSel()));
	auto dumpType = PresetValue(preset);
	if (!dumpType)
		dumpType = Helpers::ParseNumber(GetText(IDC_DUMPTYPE_VALUE));
	if (!dumpType) {
		refuse(L"Type the custom dump type: MINIDUMP_TYPE flags, in decimal or in hex with 0x.", IDC_DUMPTYPE_VALUE);
		return 0;
	}
	s.DumpType = *dumpType;
	auto maxDumps = GetText(IDC_MAXDUMPS);
	auto max = maxDumps.empty() ? std::optional<uint32_t>(0) : Helpers::ParseNumber(maxDumps);
	if (!max) {
		refuse(L"Type the maximum number of dumps, or leave it empty for Windows' default.", IDC_MAXDUMPS);
		return 0;
	}
	s.MaxNumberOfDumpFiles = *max;
	m_Settings = std::move(s);
	EndDialog(wID);
	return 0;
}

LRESULT CSilentProcessExitDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

namespace {
	// the value check boxes, their bits and the controls they turn on and off
	struct ValueOption {
		int Check;
		uint32_t Flag;
		std::initializer_list<int> Controls;
	};
	const ValueOption s_ValueOptions[] = {
		{ IDC_PH_SIZE, PageHeap::SizeRange, { IDC_PH_SIZE_START, IDC_PH_SIZE_LABEL, IDC_PH_SIZE_END } },
		{ IDC_PH_RANDOM, PageHeap::Random, { IDC_PH_RANDOM_VALUE } },
		{ IDC_PH_DLLS, PageHeap::TargetDlls, { IDC_PH_DLLS_VALUE } },
		{ IDC_PH_FAULTS, PageHeap::Faults, { IDC_PH_FAULT_LABEL, IDC_PH_FAULT_PROB, IDC_PH_FAULT_LABEL2, IDC_PH_FAULT_TIMEOUT } },
	};

	// all but faults choose the allocations full page heap gets
	bool NeedsFull(ValueOption const& option) {
		return option.Flag != PageHeap::Faults;
	}
}

CPageHeapDlg::CPageHeapDlg(std::wstring image, PageHeapSettings settings) :
	m_Image(std::move(image)), m_Settings(std::move(settings)) {
}

LRESULT CPageHeapDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());
	SetDlgItemText(IDC_IMAGE, (L"Image: " + m_Image).c_str());

	auto& s = m_Settings;
	// never set: GFlags' defaults
	auto flags = s.Flags == 0 && s.FlagsType == REG_NONE ? PageHeap::Defaults : s.Flags;
	CheckDlgButton(IDC_PH_FULL, (flags & PageHeap::Full) != 0);
	auto options = PageHeap::Options();
	for (int i = 0; i < (int)options.size(); i++) {
		SetDlgItemText(IDC_PH_OPTION_FIRST + i, options[i].Name);
		CheckDlgButton(IDC_PH_OPTION_FIRST + i, (flags & options[i].Value) != 0);
	}
	for (auto& option : s_ValueOptions)
		CheckDlgButton(option.Check, (flags & option.Flag) != 0);

	auto number = [this](int id, DWORD value) {
		if (value)
			SetDlgItemInt(id, value, FALSE);
	};
	number(IDC_PH_SIZE_START, s.SizeRangeStart);
	number(IDC_PH_SIZE_END, s.SizeRangeEnd);
	number(IDC_PH_RANDOM_VALUE, s.RandomProbability);
	number(IDC_PH_FAULT_PROB, s.FaultProbability);
	number(IDC_PH_FAULT_TIMEOUT, s.FaultTimeOut);
	SetDlgItemText(IDC_PH_DLLS_VALUE, Helpers::JoinWords(s.TargetDlls).c_str());
	// the bits no control shows start from the image's
	m_Settings.Flags = flags;

	UpdateControls();
	return TRUE;
}

uint32_t CPageHeapDlg::CheckedFlags() const {
	auto flags = m_Settings.Flags;
	auto set = [&](uint32_t flag, bool on) {
		flags = on ? flags | flag : flags & ~flag;
	};
	set(PageHeap::Full, IsDlgButtonChecked(IDC_PH_FULL));
	auto options = PageHeap::Options();
	for (int i = 0; i < (int)options.size(); i++)
		set(options[i].Value, IsDlgButtonChecked(IDC_PH_OPTION_FIRST + i));
	for (auto& option : s_ValueOptions)
		set(option.Flag, IsDlgButtonChecked(option.Check));
	return flags;
}

void CPageHeapDlg::UpdateControls() {
	bool full = IsDlgButtonChecked(IDC_PH_FULL);
	auto options = PageHeap::Options();
	for (int i = 0; i < (int)options.size(); i++)
		if (options[i].NeedsFull)
			GetDlgItem(IDC_PH_OPTION_FIRST + i).EnableWindow(full);
	for (auto& option : s_ValueOptions) {
		bool available = full || !NeedsFull(option);
		GetDlgItem(option.Check).EnableWindow(available);
		bool on = available && IsDlgButtonChecked(option.Check);
		for (auto id : option.Controls)
			GetDlgItem(id).EnableWindow(on);
	}
	SetDlgItemText(IDC_PH_FLAGS, std::format(L"PageHeapFlags: 0x{:X}", CheckedFlags()).c_str());
}

std::wstring CPageHeapDlg::GetText(int id) const {
	CString text;
	GetDlgItemText(id, text);
	return Helpers::Trim((PCWSTR)text);
}

LRESULT CPageHeapDlg::OnCheckChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	// without Full, its options are off
	if (wID == IDC_PH_FULL && !IsDlgButtonChecked(IDC_PH_FULL)) {
		auto options = PageHeap::Options();
		for (int i = 0; i < (int)options.size(); i++)
			if (options[i].NeedsFull)
				CheckDlgButton(IDC_PH_OPTION_FIRST + i, BST_UNCHECKED);
		for (auto& option : s_ValueOptions)
			if (NeedsFull(option))
				CheckDlgButton(option.Check, BST_UNCHECKED);
	}
	UpdateControls();
	return 0;
}

LRESULT CPageHeapDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto refuse = [this](PCWSTR text, int id) {
		AtlMessageBox(m_hWnd, text, IDR_MAINFRAME, MB_ICONWARNING);
		GotoDlgCtrl(GetDlgItem(id));
	};
	// a number typed (empty: 0)
	auto number = [this](int id) {
		auto text = GetText(id);
		return text.empty() ? std::optional<uint32_t>(0) : Helpers::ParseNumber(text);
	};

	PageHeapSettings s = m_Settings;
	s.Flags = CheckedFlags();
	// the values of the options that are off go
	s.SizeRangeStart = s.SizeRangeEnd = s.RandomProbability = s.FaultProbability = s.FaultTimeOut = 0;
	s.TargetDlls.clear();
	if (s.Flags & PageHeap::SizeRange) {
		auto start = number(IDC_PH_SIZE_START), end = number(IDC_PH_SIZE_END);
		if (!start || !end || *end == 0 || *start > *end) {
			refuse(L"Type the sizes' range: from a number of bytes to a larger one.", IDC_PH_SIZE_START);
			return 0;
		}
		s.SizeRangeStart = *start;
		s.SizeRangeEnd = *end;
	}
	if (s.Flags & PageHeap::Random) {
		auto percent = number(IDC_PH_RANDOM_VALUE);
		if (!percent || *percent < 1 || *percent > 100) {
			refuse(L"Type the percent of the allocations: 1 to 100.", IDC_PH_RANDOM_VALUE);
			return 0;
		}
		s.RandomProbability = *percent;
	}
	if (s.Flags & PageHeap::TargetDlls) {
		s.TargetDlls = Helpers::SplitWords(GetText(IDC_PH_DLLS_VALUE));
		if (s.TargetDlls.empty()) {
			refuse(L"Type the DLLs, separated by spaces.", IDC_PH_DLLS_VALUE);
			return 0;
		}
	}
	if (s.Flags & PageHeap::Faults) {
		auto probability = number(IDC_PH_FAULT_PROB), timeout = number(IDC_PH_FAULT_TIMEOUT);
		if (!probability || *probability < 1 || *probability > 10000) {
			refuse(L"Type the faults' probability: 1 to 10000 (in 10000s).", IDC_PH_FAULT_PROB);
			return 0;
		}
		if (!timeout) {
			refuse(L"Type the seconds before faults start (or leave it empty).", IDC_PH_FAULT_TIMEOUT);
			return 0;
		}
		s.FaultProbability = *probability;
		s.FaultTimeOut = *timeout;
	}
	m_Settings = std::move(s);
	EndDialog(wID);
	return 0;
}

LRESULT CPageHeapDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

CVerifierDlg::CVerifierDlg(std::wstring image, VerifierSettings settings, std::vector<std::wstring> providers) :
	m_Image(std::move(image)), m_Settings(std::move(settings)), m_Providers(std::move(providers)) {
}

LRESULT CVerifierDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());
	SetDlgItemText(IDC_IMAGE, (L"Image: " + m_Image).c_str());

	auto same = [](std::wstring const& a, std::wstring const& b) { return _wcsicmp(a.c_str(), b.c_str()) == 0; };
	// the installed providers, then the image's others
	for (auto& dll : m_Settings.Dlls)
		if (std::ranges::none_of(m_Providers, [&](auto& p) { return same(p, dll); }))
			m_Providers.push_back(dll);

	m_List.Attach(GetDlgItem(IDC_VRF_PROVIDERS));
	m_List.SetExtendedListViewStyle(LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
	CRect rc;
	m_List.GetClientRect(&rc);
	m_List.InsertColumn(0, L"Provider", LVCFMT_LEFT, rc.Width() - ::GetSystemMetrics(SM_CXVSCROLL));
	for (int i = 0; i < (int)m_Providers.size(); i++) {
		m_List.InsertItem(i, m_Providers[i].c_str());
		m_List.SetCheckState(i, std::ranges::any_of(m_Settings.Dlls, [&](auto& d) { return same(d, m_Providers[i]); }));
	}
	if (m_Settings.Flags)
		SetDlgItemText(IDC_VRF_FLAGS, std::format(L"{:X}", m_Settings.Flags).c_str());
	return TRUE;
}

LRESULT CVerifierDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CString text;
	GetDlgItemText(IDC_VRF_FLAGS, text);
	auto flagsText = Helpers::Trim((PCWSTR)text);
	if (flagsText.starts_with(L"0x") || flagsText.starts_with(L"0X"))
		flagsText.erase(0, 2);
	auto flags = flagsText.empty() ? std::optional<uint32_t>(0) : Helpers::ParseNumber(L"0x" + flagsText);
	if (!flags) {
		AtlMessageBox(m_hWnd, L"Type the flags in hex (or leave them empty).", IDR_MAINFRAME, MB_ICONWARNING);
		GotoDlgCtrl(GetDlgItem(IDC_VRF_FLAGS));
		return 0;
	}
	VerifierSettings s = m_Settings;
	s.Flags = *flags;
	s.Dlls.clear();
	for (int i = 0; i < m_List.GetItemCount(); i++)
		if (m_List.GetCheckState(i))
			s.Dlls.push_back(m_Providers[i]);
	m_Settings = std::move(s);
	EndDialog(wID);
	return 0;
}

LRESULT CVerifierDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}
