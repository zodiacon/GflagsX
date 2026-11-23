#include "pch.h"
#include "SystemDialogs.h"
#include "Helpers.h"

using namespace FlagStore;

namespace {
	PCWSTR TargetText(bool kernel) {
		return kernel ? L"The running system's, until it restarts." :
			L"In the registry: the system has them from its next boot.";
	}

	std::wstring TextOf(CWindow dlg, int id) {
		CString text;
		dlg.GetDlgItemText(id, text);
		return Helpers::Trim((PCWSTR)text);
	}

	void Refuse(CWindow dlg, PCWSTR text, int id) {
		AtlMessageBox(dlg, text, IDR_MAINFRAME, MB_ICONWARNING);
		::SetFocus(dlg.GetDlgItem(id));
	}
}

CSpecialPoolDlg::CSpecialPoolDlg(bool kernel, SpecialPoolSettings settings) : m_Kernel(kernel), m_Settings(settings) {
}

LRESULT CSpecialPoolDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());
	SetDlgItemText(IDC_TARGET_TEXT, TargetText(m_Kernel));
	CheckDlgButton(IDC_SPP_ENABLE, m_Settings.Tag != 0);
	if (m_Settings.Tag)
		SetDlgItemText(IDC_SPP_TAG, FormatPoolTag(m_Settings.Tag).c_str());
	CEdit(GetDlgItem(IDC_SPP_TAG)).SetLimitText(10);
	CheckRadioButton(IDC_SPP_END, IDC_SPP_START, m_Settings.VerifyEnd ? IDC_SPP_END : IDC_SPP_START);
	UpdateEnabled();
	return TRUE;
}

void CSpecialPoolDlg::UpdateEnabled() {
	bool enabled = IsDlgButtonChecked(IDC_SPP_ENABLE);
	for (auto id : { IDC_SPP_LABEL, IDC_SPP_TAG, IDC_SPP_HINT, IDC_SPP_END, IDC_SPP_START })
		GetDlgItem(id).EnableWindow(enabled);
}

LRESULT CSpecialPoolDlg::OnEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdateEnabled();
	return 0;
}

LRESULT CSpecialPoolDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	SpecialPoolSettings settings;
	if (IsDlgButtonChecked(IDC_SPP_ENABLE)) {
		// the text as typed: a tag's trailing spaces count, the others don't
		CString text;
		GetDlgItemText(IDC_SPP_TAG, text);
		std::wstring typed = (PCWSTR)text;
		while (!typed.empty() && iswspace(typed.front()))
			typed.erase(0, 1);
		auto tag = ParsePoolTag(typed);
		if (!tag) {
			Refuse(m_hWnd, L"Type a pool tag (4 characters at most) or a size in hex (0x...).", IDC_SPP_TAG);
			return 0;
		}
		settings.Tag = *tag;
		settings.VerifyEnd = IsDlgButtonChecked(IDC_SPP_END);
	}
	else {
		// off: as it was set, for the next time
		settings.VerifyEnd = m_Settings.VerifyEnd;
	}
	m_Settings = settings;
	EndDialog(wID);
	return 0;
}

LRESULT CSpecialPoolDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

CObjectTraceDlg::CObjectTraceDlg(bool kernel, ObjectTraceSettings settings) : m_Kernel(kernel), m_Settings(std::move(settings)) {
}

LRESULT CObjectTraceDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	CenterWindow(GetParent());
	SetDlgItemText(IDC_TARGET_TEXT, TargetText(m_Kernel));
	CheckDlgButton(IDC_OBT_ENABLE, m_Settings.Enabled);
	std::wstring tags;
	for (auto& tag : m_Settings.PoolTags)
		tags += (tags.empty() ? L"" : L";") + tag;
	SetDlgItemText(IDC_OBT_TAGS, tags.c_str());
	SetDlgItemText(IDC_OBT_PROCESS, m_Settings.ProcessName.c_str());
	CheckDlgButton(IDC_OBT_PERMANENT, m_Settings.Permanent);
	UpdateEnabled();
	return TRUE;
}

void CObjectTraceDlg::UpdateEnabled() {
	bool enabled = IsDlgButtonChecked(IDC_OBT_ENABLE);
	for (auto id : { IDC_OBT_TAGS_LABEL, IDC_OBT_TAGS, IDC_OBT_PROCESS_LABEL, IDC_OBT_PROCESS, IDC_OBT_PERMANENT })
		GetDlgItem(id).EnableWindow(enabled);
}

LRESULT CObjectTraceDlg::OnEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	UpdateEnabled();
	return 0;
}

LRESULT CObjectTraceDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	ObjectTraceSettings settings;
	settings.Enabled = IsDlgButtonChecked(IDC_OBT_ENABLE);
	if (settings.Enabled) {
		auto tags = TextOf(m_hWnd, IDC_OBT_TAGS);
		size_t start = 0;
		while (start <= tags.size()) {
			auto end = std::min(tags.find(L';', start), tags.size());
			auto tag = Helpers::Trim(std::wstring_view(tags).substr(start, end - start));
			if (!tag.empty()) {
				if (tag.size() > 4) {
					Refuse(m_hWnd, L"Pool tags have 4 characters at most.", IDC_OBT_TAGS);
					return 0;
				}
				settings.PoolTags.push_back(tag);
			}
			start = end + 1;
		}
		settings.ProcessName = TextOf(m_hWnd, IDC_OBT_PROCESS);
		if (settings.PoolTags.empty() && settings.ProcessName.empty()) {
			Refuse(m_hWnd, L"Type the pool tags to trace, or the image name of the processes to trace.", IDC_OBT_TAGS);
			return 0;
		}
		settings.Permanent = IsDlgButtonChecked(IDC_OBT_PERMANENT);
	}
	m_Settings = std::move(settings);
	EndDialog(wID);
	return 0;
}

LRESULT CObjectTraceDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}
