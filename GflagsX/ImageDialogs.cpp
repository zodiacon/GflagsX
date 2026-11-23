#include "pch.h"
#include "ImageDialogs.h"
#include "Helpers.h"
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
