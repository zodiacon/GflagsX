#include "pch.h"
#include "ImageSettingsForm.h"
#include "Mitigations.h"
#include "PageHeap.h"
#include "Helpers.h"
#include <WTLHelper.h>

using namespace Mitigations;

namespace {
	// the check boxes' bits (Mitigations::Bits), in IDC_DEP.. order
	const uint64_t s_BitValues[] = { 0x1, 0x2 };

	// the fields' rows, in dialog units: below the check boxes, inside the group box
	constexpr int FirstRowY = 130, RowHeight = 17, LabelX = 14, LabelWidth = 128, ComboX = 146, ComboWidth = 90;
}

LRESULT CImageSettingsForm::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	auto font = GetFont();
	auto fields = Fields();
	auto toPixels = [this](int x, int y, int cx, int cy) {
		CRect rc(x, y, x + cx, y + cy);
		MapDialogRect(&rc);
		return rc;
	};
	bool inferred = false;
	for (int i = 0; i < (int)fields.size(); i++) {
		int y = FirstRowY + i * RowHeight;
		CStatic label;
		auto rcLabel = toPixels(LabelX, y + 2, LabelWidth, 8);
		// a field whose place in the value is inferred is marked, and the note below says so
		auto name = std::wstring(fields[i].Name) + (fields[i].Inferred ? L" *:" : L":");
		inferred |= fields[i].Inferred;
		label.Create(m_hWnd, &rcLabel, name.c_str(), WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 0, IDC_FIELD_LABEL_FIRST + i);
		label.SetFont(font);
		CComboBox combo;
		auto rcCombo = toPixels(ComboX, y, ComboWidth, 120);
		combo.Create(m_hWnd, &rcCombo, nullptr, WS_CHILD | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWNLIST,
			0, IDC_FIELD_FIRST + i);
		combo.SetFont(font);
	}
	int bottom = FirstRowY + (int)fields.size() * RowHeight;
	if (inferred) {
		CStatic note;
		auto rcNote = toPixels(LabelX, bottom + 2, ComboX + ComboWidth - LabelX, 16);
		note.Create(m_hWnd, &rcNote, L"* Its place in the value is inferred from Windows' list of mitigations, not seen written by Windows.",
			WS_CHILD | WS_VISIBLE | SS_NOPREFIX, 0, IDC_FIELD_NOTE);
		note.SetFont(font);
		bottom += 20;
	}
	// the group box around the rows, the form as tall as it all
	bottom += 4;
	CRect group;
	auto hGroup = GetDlgItem(IDC_MITIGATIONS_GROUP);
	hGroup.GetWindowRect(&group);
	ScreenToClient(&group);
	group.bottom = toPixels(0, bottom, 0, 0).top;
	hGroup.MoveWindow(&group);

	CRect rc;
	GetClientRect(&rc);
	m_IdealSize = CSize(rc.Width(), toPixels(0, bottom + 7, 0, 0).top);
	SetWindowPos(nullptr, 0, 0, m_IdealSize.cx, m_IdealSize.cy, SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);

	CEdit(GetDlgItem(IDC_MITIGATIONS)).SetLimitText(2 + Mitigations::Words * 16);	// 0x and the digits
	CEdit(GetDlgItem(IDC_DEBUGGER)).SetLimitText(MAX_PATH * 2);
	Enable(false);
	return TRUE;
}

CSize CImageSettingsForm::GetIdealSize() const {
	return m_IdealSize;
}

void CImageSettingsForm::SetSettings(FlagStore::ImageSettings const& settings) {
	m_Updating = true;
	SetDlgItemText(IDC_DEBUGGER, settings.Debugger.c_str());
	m_Mitigations = settings.Mitigations;
	ShowMitigations();
	SetPageHeap(settings.PageHeap);
	SetVerifier(settings.Verifier);
	m_Updating = false;
}

FlagStore::PageHeapSettings const& CImageSettingsForm::GetPageHeap() const {
	return m_PageHeap;
}

void CImageSettingsForm::SetPageHeap(FlagStore::PageHeapSettings const& settings) {
	m_PageHeap = settings;
	bool none = settings.Flags == 0 && settings.FlagsType == REG_NONE;
	SetDlgItemText(IDC_PAGEHEAP_TEXT, none ? L"Not configured" : PageHeap::Summary(settings.Flags).c_str());
}

FlagStore::VerifierSettings const& CImageSettingsForm::GetVerifier() const {
	return m_Verifier;
}

void CImageSettingsForm::SetVerifier(FlagStore::VerifierSettings const& settings) {
	m_Verifier = settings;
	auto text = Helpers::JoinWords(settings.Dlls);
	if (settings.Flags)
		text += std::format(L"{}flags 0x{:X}", text.empty() ? L"" : L"; ", settings.Flags);
	SetDlgItemText(IDC_VERIFIER_TEXT, text.empty() ? L"Not configured" : text.c_str());
}

void CImageSettingsForm::GetSettings(FlagStore::ImageSettings& settings) const {
	CString debugger;
	GetDlgItemText(IDC_DEBUGGER, debugger);
	settings.Debugger = Helpers::Trim((PCWSTR)debugger);
	settings.Mitigations = m_Mitigations;
	settings.PageHeap = m_PageHeap;
	settings.Verifier = m_Verifier;
}

void CImageSettingsForm::SetSilentProcessExitText(std::wstring const& text) {
	SetDlgItemText(IDC_SILENTEXIT_TEXT, text.c_str());
}

void CImageSettingsForm::Enable(bool enable) {
	if (!enable) {
		SetSettings({});
		for (auto id : { IDC_SILENTEXIT_TEXT, IDC_PAGEHEAP_TEXT, IDC_VERIFIER_TEXT })
			SetDlgItemText(id, L"");
	}
	for (auto child = GetWindow(GW_CHILD); child; child = child.GetWindow(GW_HWNDNEXT))
		child.EnableWindow(enable);
}

void CImageSettingsForm::ShowMitigations() {
	auto updating = m_Updating;
	m_Updating = true;
	SetDlgItemText(IDC_MITIGATIONS, Format(m_Mitigations).c_str());
	for (int i = 0; i < _countof(s_BitValues); i++)
		CheckDlgButton(IDC_DEP + i, (m_Mitigations.Word[0] & s_BitValues[i]) ? BST_CHECKED : BST_UNCHECKED);

	auto fields = Fields();
	for (int i = 0; i < (int)fields.size(); i++) {
		auto& field = fields[i];
		auto state = GetState(m_Mitigations, field);
		CComboBox combo(GetDlgItem(IDC_FIELD_FIRST + i));
		combo.ResetContent();
		// the third choice where the field has one, or where the value has it anyway (reserved)
		int count = field.OtherName || state == State::Other ? 4 : 3;
		for (int s = 0; s < count; s++)
			combo.AddString(StateName(field, State(s)).c_str());
		combo.SetCurSel((int)state);
	}
	m_Updating = updating;
}

void CImageSettingsForm::Changed() {
	if (!m_Updating && OnChanged)
		OnChanged();
}

LRESULT CImageSettingsForm::OnDebuggerChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	Changed();
	return 0;
}

LRESULT CImageSettingsForm::OnBrowseDebugger(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	COMDLG_FILTERSPEC filters[] = { { L"Programs", L"*.exe" }, { L"All Files", L"*.*" } };
	CShellFileOpenDialog dlg(nullptr, FOS_FORCEFILESYSTEM | FOS_FILEMUSTEXIST | FOS_PATHMUSTEXIST, nullptr, filters, _countof(filters));
	dlg.GetPtr()->SetTitle(L"Debugger");
	SuspendResumeHook sr;
	if (dlg.DoModal(m_hWnd) != IDOK)
		return 0;
	CString path;
	if (FAILED(dlg.GetFilePath(path)))
		return 0;
	// Windows runs the command line with the image's after it: a path with spaces is quoted
	if (path.Find(L' ') >= 0)
		path = L"\"" + path + L"\"";
	SetDlgItemText(IDC_DEBUGGER, path);
	return 0;
}

void CImageSettingsForm::TakeValueText() {
	CString text;
	GetDlgItemText(IDC_MITIGATIONS, text);
	auto value = Parse((PCWSTR)text);
	if (value && *value != m_Mitigations) {
		m_Mitigations = *value;
		ShowMitigations();
		Changed();
	}
	else {
		// not a value: back to the one there is
		ShowMitigations();
	}
}

LRESULT CImageSettingsForm::OnValueKillFocus(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!m_Updating)
		TakeValueText();
	return 0;
}

LRESULT CImageSettingsForm::OnEnter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (::GetFocus() == GetDlgItem(IDC_MITIGATIONS)) {
		TakeValueText();
		CEdit(GetDlgItem(IDC_MITIGATIONS)).SetSelAll();
	}
	return 0;
}

LRESULT CImageSettingsForm::OnEscape(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (::GetFocus() == GetDlgItem(IDC_MITIGATIONS))
		ShowMitigations();
	return 0;
}

LRESULT CImageSettingsForm::OnBitClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto bit = s_BitValues[wID - IDC_DEP];
	if (IsDlgButtonChecked(wID))
		m_Mitigations.Word[0] |= bit;
	else
		m_Mitigations.Word[0] &= ~bit;
	ShowMitigations();
	Changed();
	return 0;
}

LRESULT CImageSettingsForm::OnFieldChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int index = wID - IDC_FIELD_FIRST;
	auto fields = Fields();
	if (index >= (int)fields.size())
		return 0;
	int selected = CComboBox(GetDlgItem(wID)).GetCurSel();
	if (selected < 0)
		return 0;
	m_Mitigations = SetState(m_Mitigations, fields[index], State(selected));
	ShowMitigations();
	Changed();
	return 0;
}

LRESULT CImageSettingsForm::OnSilentExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (OnSilentProcessExit)
		OnSilentProcessExit();
	return 0;
}

LRESULT CImageSettingsForm::OnPageHeapButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (OnPageHeap)
		OnPageHeap();
	return 0;
}

LRESULT CImageSettingsForm::OnVerifierButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (OnVerifier)
		OnVerifier();
	return 0;
}
