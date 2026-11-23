#include "pch.h"
#include "ImageView.h"
#include "ImageDialogs.h"
#include "AppSettings.h"
#include "GlobalFlags.h"
#include "Mitigations.h"
#include "Helpers.h"
#include <SortHelper.h>
#include <DpiHelper.h>
#include <ClipboardHelper.h>
#include <ListViewhelper.h>

using namespace FlagStore;

namespace {
	Locations const& Where() {
		return Locations::System();
	}
}

CImageView::CImageView(IMainFrame* frame) : CViewBase(frame), m_Flags(frame, GFlags::Target::Image) {
	m_Flags.OnChanged = [this] { UpdateStatus(); };
}

LRESULT CImageView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_hWndClient = m_Splitter.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_List.Create(m_Splitter, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS | LVS_SINGLESEL);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_INFOTIP);
	SuspendHookWhilePainting(m_List);

	auto scale = [this](int width) { return DpiHelper::Scale(width, m_hWnd); };
	m_List.InsertColumn(0, L"Image", LVCFMT_LEFT, scale(160));
	m_List.InsertColumn(1, L"Global Flags", LVCFMT_RIGHT, scale(85));
	m_List.InsertColumn(2, L"Debugger", LVCFMT_LEFT, scale(110));
	m_List.InsertColumn(3, L"Mitigations", LVCFMT_RIGHT, scale(125));
	ListViewHelper::EnableColumnDpiScaling(m_List);

	m_Right.Create(m_Splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_Flags.DoCreate(m_Right);
	m_FormContainer.Create(m_Right, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
	m_Form.Create(m_FormContainer);
	m_Form.ShowWindow(SW_SHOWNOACTIVATE);
	m_FormContainer.SetClient(m_Form);
	m_Form.OnChanged = [this] { UpdateStatus(); };
	m_Form.OnSilentProcessExit = [this] {
		BOOL handled;
		OnSilentProcessExit(0, ID_IMAGE_SILENTEXIT, nullptr, handled);
	};

	m_Right.SetSplitterPanes(m_Flags, m_FormContainer);
	m_Splitter.SetSplitterPanes(m_List, m_Right);
	auto& settings = AppSettings::Get();
	m_Splitter.SetSplitterPosPct(std::clamp(settings.ImagesSplitterPos(), 10, 80), false);
	m_Right.SetSplitterPosPct(std::clamp(settings.FlagsSplitterPos(), 20, 80), false);

	LoadImages();
	return 0;
}

LRESULT CImageView::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	auto& settings = AppSettings::Get();
	settings.ImagesSplitterPos(m_Splitter.GetSplitterPosPct());
	settings.FlagsSplitterPos(m_Right.GetSplitterPosPct());
	bHandled = FALSE;
	return 0;
}

BOOL CImageView::PreTranslateMessage(MSG* pMsg) {
	if (pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST || !m_Form.IsChild(pMsg->hwnd))
		return FALSE;
	return m_Form.IsDialogMessage(pMsg);
}

void CImageView::LoadImages() {
	auto names = GetImages(Where());
	m_Error = names ? L"" : Helpers::ErrorText(names.error());
	m_Images.clear();
	for (auto& name : names.value_or(std::vector<std::wstring>{})) {
		auto settings = ReadImage(Where(), name);
		m_Images.push_back({ name, settings.value_or(ImageSettings{}), settings.has_value() });
	}
	Sort(m_List);
	m_List.SetItemCountEx((int)m_Images.size(), LVSICF_NOSCROLL);
	m_List.Invalidate();

	// the first time, the first image
	auto shown = m_Shown.empty() && !m_Images.empty() && !m_List.GetSelectedCount() ? m_Images[0].Name : m_Shown;
	m_Restoring = true;
	SelectImage(shown);
	m_Restoring = false;
	ShowImage(FindImage(shown) >= 0 ? shown : L"");
}

int CImageView::FindImage(std::wstring const& name) const {
	auto it = std::ranges::find_if(m_Images, [&](auto& image) { return _wcsicmp(image.Name.c_str(), name.c_str()) == 0; });
	return it == m_Images.end() ? -1 : int(it - m_Images.begin());
}

void CImageView::SelectImage(std::wstring const& name) {
	m_List.SetItemState(-1, 0, LVIS_SELECTED | LVIS_FOCUSED);
	if (int row = FindImage(name); row >= 0) {
		m_List.SetItemState(row, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
		m_List.EnsureVisible(row, FALSE);
	}
}

void CImageView::ShowImage(std::wstring const& name) {
	KillTimer(SelectTimer);
	m_Shown = name;
	ImageSettings settings;
	bool readable = false;
	if (!name.empty()) {
		auto read = ReadImage(Where(), name);
		readable = read.has_value();
		if (readable) {
			settings = *read;
			// the row follows the key as it is now
			if (int row = FindImage(name); row >= 0) {
				m_Images[row].Settings = settings;
				m_Images[row].Readable = true;
				m_List.RedrawItems(row, row);
			}
		}
		else {
			Frame()->ShowError(std::format(L"{}\n\n{}", name, Helpers::ErrorText(read.error())));
		}
	}
	if (!readable)
		m_Shown.clear();
	m_ShownSettings = settings;
	m_Flags.Enable(readable);
	m_Form.Enable(readable);
	if (readable) {
		m_Flags.SetValue(settings.GlobalFlag);
		m_Form.SetSettings(settings);
		m_Form.SetSilentProcessExitText(SilentProcessExitText());
	}
	UpdateStatus();
}

std::wstring CImageView::SilentProcessExitText() const {
	auto settings = ReadSilentProcessExit(Where(), m_Shown);
	if (!settings || !settings->Exists)
		return L"Not configured";
	std::wstring text;
	if (settings->ReportingMode & LocalDump)
		text = L"Dumps";
	if (settings->ReportingMode & LaunchMonitorProcess)
		text += text.empty() ? L"Monitor" : L", Monitor";
	if (settings->ReportingMode & Notification)
		text += text.empty() ? L"Notification" : L", Notification";
	return text.empty() ? L"Configured" : text;
}

bool CImageView::ShowSelected() {
	std::wstring name;
	if (int row = m_List.GetNextItem(-1, LVNI_SELECTED); row >= 0)
		name = m_Images[row].Name;
	if (_wcsicmp(name.c_str(), m_Shown.c_str()) == 0)
		return true;
	if (!AskAboutChanges()) {
		// the user kept the shown image's changes: it gets the selection back
		m_Restoring = true;
		SelectImage(m_Shown);
		m_Restoring = false;
		return false;
	}
	ShowImage(name);
	return true;
}

ImageSettings CImageView::EditedSettings() const {
	auto settings = m_ShownSettings;
	// the image's flags no check box shows are kept
	settings.GlobalFlag = (settings.GlobalFlag & ~GFlags::TargetMask(GFlags::Target::Image)) | m_Flags.GetValue();
	m_Form.GetSettings(settings);
	return settings;
}

bool CImageView::IsModified() const {
	return !m_Shown.empty() && EditedSettings() != m_ShownSettings;
}

bool CImageView::Apply() {
	auto settings = EditedSettings();
	auto error = WriteImage(Where(), m_Shown, settings);
	if (error != ERROR_SUCCESS) {
		Frame()->ShowError(std::format(L"Writing the settings of {} failed.\n\n{}", m_Shown, Helpers::ErrorText(error)));
		return false;
	}
	// without the flag, the Silent Process Exit settings do nothing: they go (as they did in the WPF version)
	if (!(settings.GlobalFlag & GFlags::MonitorSilentProcessExit))
		DeleteSilentProcessExit(Where(), m_Shown);
	ShowImage(m_Shown);
	return true;
}

bool CImageView::AskAboutChanges() {
	if (!IsModified())
		return true;
	auto text = std::format(L"The settings of {} were changed. Apply the changes?", m_Shown);
	switch (AtlMessageBox(Frame()->GetHwnd(), text.c_str(), IDR_MAINFRAME, MB_ICONQUESTION | MB_YESNOCANCEL)) {
		case IDYES: return Apply();
		case IDNO: return true;
	}
	return false;
}

bool CImageView::CanClose() {
	return AskAboutChanges();
}

std::wstring CImageView::GetStatusText() const {
	if (!m_Error.empty())
		return std::format(L"Reading the images failed: {}", m_Error);
	auto text = std::format(L"{} image{}", m_Images.size(), m_Images.size() == 1 ? L"" : L"s");
	if (!m_Shown.empty()) {
		text += std::format(L" | {}: flags {}", m_Shown, GFlags::FormatValue(m_ShownSettings.GlobalFlag));
		if (IsModified())
			text += L" (changed: not applied)";
		// bits of flags that aren't an image's (kept on Apply): seen nowhere else
		if (auto other = m_ShownSettings.GlobalFlag & ~GFlags::TargetMask(GFlags::Target::Image))
			text += std::format(L"; set with no check box here: {}", GFlags::FormatValue(other));
	}
	return text;
}

void CImageView::UpdateStatus() const {
	Frame()->SetStatusText(this, GetStatusText());
}

void CImageView::OnActivate(bool active) {
	if (!active)
		return;
	m_List.SetFocus();
	// the list had no size when it was filled
	if (int row = m_List.GetNextItem(-1, LVNI_SELECTED); row >= 0)
		m_List.EnsureVisible(row, FALSE);
}

void CImageView::UpdateUI(CUpdateUIBase& ui) {
	bool shown = !m_Shown.empty();
	bool selected = m_List.GetSelectedCount() > 0;
	ui.UIEnable(ID_FLAGS_APPLY, IsModified());
	ui.UIEnable(ID_VIEW_REFRESH, true);
	ui.UIEnable(ID_FLAGS_CLEARALL, shown && m_Flags.GetValue() != 0);
	ui.UIEnable(ID_FLAGS_TOGGLE, shown && m_Flags.HasSelection());
	ui.UIEnable(ID_IMAGE_NEW, true);
	ui.UIEnable(ID_IMAGE_DELETE, selected);
	ui.UIEnable(ID_IMAGE_SILENTEXIT, shown);
	auto focus = ::GetFocus();
	ui.UIEnable(ID_EDIT_COPY, focus == m_List ? selected : m_Flags.HasFocus() ? m_Flags.HasSelection() : m_Form.IsChild(focus));
}

void CImageView::OnDpiChanged(UINT /*dpi*/) {
	m_Flags.OnDpiChanged();
}

LRESULT CImageView::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
	if (wParam != SelectTimer) {
		bHandled = FALSE;
		return 0;
	}
	KillTimer(SelectTimer);
	ShowSelected();
	return 0;
}

void CImageView::OnStateChanged(HWND hList, int /*from*/, int /*to*/, UINT oldState, UINT newState) {
	if (hList != m_List || m_Restoring || !((oldState ^ newState) & LVIS_SELECTED))
		return;
	// once the selection settles
	SetTimer(SelectTimer, SelectDelay);
}

LRESULT CImageView::OnListKeyDown(int /*idCtrl*/, LPNMHDR hdr, BOOL& bHandled) {
	auto kd = reinterpret_cast<NMLVKEYDOWN*>(hdr);
	if (hdr->hwndFrom != m_List || kd->wVKey != VK_DELETE) {
		bHandled = FALSE;
		return 0;
	}
	PostMessage(WM_COMMAND, ID_IMAGE_DELETE);
	return 0;
}

bool CImageView::OnRightClickList(HWND hList, int row, int /*col*/, POINT const& pt) {
	if (hList != m_List)
		return false;
	// the image under the mouse is shown first (its changes are asked about), so the commands are its
	if (row >= 0) {
		KillTimer(SelectTimer);
		if (!ShowSelected())
			return true;
	}
	CMenu menu;
	menu.LoadMenu(IDR_CONTEXT);
	Frame()->ShowContextMenu(menu.GetSubMenu(1), pt);
	return true;
}

CString CImageView::GetColumnText(HWND, int row, int col) const {
	auto& image = m_Images[row];
	switch (static_cast<Column>(col)) {
		case Column::Name: return image.Name.c_str();
		case Column::GlobalFlag:
			if (!image.Readable)
				return L"(not readable)";
			return image.Settings.GlobalFlagType == REG_NONE ? L"" : GFlags::FormatValue(image.Settings.GlobalFlag).c_str();
		case Column::Debugger: return image.Settings.Debugger.c_str();
		case Column::Mitigations:
			return image.Settings.MitigationsType == REG_NONE ? L"" : Mitigations::Format(image.Settings.Mitigations).c_str();
	}
	return L"";
}

void CImageView::DoSort(SortInfo const* si) {
	auto asc = si->SortAscending;
	std::ranges::stable_sort(m_Images, [&](auto& i1, auto& i2) {
		switch (static_cast<Column>(si->SortColumn)) {
			case Column::Name: return SortHelper::Sort(i1.Name, i2.Name, asc);
			case Column::GlobalFlag: return SortHelper::Sort(i1.Settings.GlobalFlag, i2.Settings.GlobalFlag, asc);
			case Column::Debugger: return SortHelper::Sort(i1.Settings.Debugger, i2.Settings.Debugger, asc);
			case Column::Mitigations: return SortHelper::Sort(i1.Settings.Mitigations, i2.Settings.Mitigations, asc);
		}
		return false;
		});
	// the selection stays on the shown image
	if (m_List) {
		m_Restoring = true;
		SelectImage(m_Shown);
		m_Restoring = false;
	}
}

LRESULT CImageView::OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (IsModified())
		Apply();
	return 0;
}

LRESULT CImageView::OnReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (IsModified() && AtlMessageBox(Frame()->GetHwnd(), L"Discard the changes and read the images again?", IDR_MAINFRAME,
		MB_ICONQUESTION | MB_OKCANCEL) != IDOK)
		return 0;
	LoadImages();
	return 0;
}

LRESULT CImageView::OnClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	m_Flags.ClearAll();
	return 0;
}

LRESULT CImageView::OnToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	m_Flags.ToggleSelected();
	return 0;
}

LRESULT CImageView::OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto focus = ::GetFocus();
	// Ctrl+C is an accelerator: in the form's edit boxes it's theirs
	if (m_Form.IsChild(focus)) {
		::SendMessage(focus, WM_COPY, 0, 0);
		return 0;
	}
	std::wstring text;
	if (m_Flags.HasFocus())
		text = m_Flags.GetSelectedText();
	else if (int row = m_List.GetNextItem(-1, LVNI_SELECTED); row >= 0)
		text = m_Images[row].Name;
	if (!text.empty())
		ClipboardHelper::CopyText(m_hWnd, text.c_str());
	return 0;
}

LRESULT CImageView::OnNewImage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (!AskAboutChanges())
		return 0;
	CNewImageDlg dlg;
	if (dlg.DoModal(Frame()->GetHwnd()) != IDOK)
		return 0;
	auto name = dlg.GetName();
	auto error = CreateImage(Where(), name);
	if (error != ERROR_SUCCESS && error != ERROR_ALREADY_EXISTS) {
		Frame()->ShowError(std::format(L"Adding {} failed.\n\n{}", name, Helpers::ErrorText(error)));
		return 0;
	}
	// there already (perhaps added elsewhere since the list was read): it's selected
	m_Shown = name;
	LoadImages();
	m_List.SetFocus();
	return 0;
}

LRESULT CImageView::OnDeleteImage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	int row = m_List.GetNextItem(-1, LVNI_SELECTED);
	if (row < 0)
		return 0;
	auto name = m_Images[row].Name;
	auto question = std::format(L"Delete all the settings of {} (its Image File Execution Options key, and its Silent Process Exit settings)?", name);
	if (AtlMessageBox(Frame()->GetHwnd(), question.c_str(), IDR_MAINFRAME, MB_ICONWARNING | MB_OKCANCEL | MB_DEFBUTTON2) != IDOK)
		return 0;
	auto error = DeleteImage(Where(), name);
	if (error != ERROR_SUCCESS) {
		Frame()->ShowError(std::format(L"Deleting {} failed.\n\n{}", name, Helpers::ErrorText(error)));
		return 0;
	}
	// the next one is shown (its changes are gone with the key)
	m_Shown = row + 1 < (int)m_Images.size() ? m_Images[row + 1].Name : row > 0 ? m_Images[row - 1].Name : L"";
	m_ShownSettings = {};
	LoadImages();
	return 0;
}

LRESULT CImageView::OnSilentProcessExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (m_Shown.empty())
		return 0;
	auto settings = ReadSilentProcessExit(Where(), m_Shown);
	if (!settings) {
		Frame()->ShowError(std::format(L"Reading the Silent Process Exit settings of {} failed.\n\n{}", m_Shown, Helpers::ErrorText(settings.error())));
		return 0;
	}
	CSilentProcessExitDlg dlg(m_Shown, *settings);
	if (dlg.DoModal(Frame()->GetHwnd()) != IDOK)
		return 0;
	auto error = WriteSilentProcessExit(Where(), m_Shown, dlg.GetSettings());
	if (error != ERROR_SUCCESS) {
		Frame()->ShowError(std::format(L"Writing the Silent Process Exit settings of {} failed.\n\n{}", m_Shown, Helpers::ErrorText(error)));
		return 0;
	}
	m_Form.SetSilentProcessExitText(SilentProcessExitText());
	// the settings do nothing without the flag: it's checked, for Apply
	m_Flags.Check(GFlags::MonitorSilentProcessExit, true);
	return 0;
}
