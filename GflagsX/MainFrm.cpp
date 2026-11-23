#include "pch.h"
#include "MainFrm.h"
#include "AboutDlg.h"
#include "AppSettings.h"
#include "FlagsView.h"
#include "ImageView.h"
#include <WTLHelper.h>
#include <ToolbarHelper.h>
#include <IconHelper.h>
#include <DpiHelper.h>

namespace {
	const ToolBarButtonInfo s_ToolBarButtons[] = {
		{ ID_FLAGS_APPLY, IDI_APPLY, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, L"Apply" },
		{ ID_VIEW_REFRESH, IDI_REFRESH, BTNS_BUTTON | BTNS_AUTOSIZE | BTNS_SHOWTEXT, L"Reload" },
		{ 0 },
		{ ID_IMAGE_NEW, IDI_NEWIMAGE },
		{ ID_IMAGE_DELETE, IDI_DELETE },
		{ ID_IMAGE_SILENTEXIT, IDI_CONFIGURE },
		{ 0 },
		{ ID_EDIT_COPY, IDI_COPY },
	};

	// the tabs' icons, in ViewType order
	const UINT s_Icons[] = { IDI_REGISTRY, IDI_KERNEL, IDI_IMAGE };

	int s_Panes[] = { ID_DEFAULT_PANE, ID_PANE_VIEW };

	bool IsRunningElevated() {
		wil::unique_handle hToken;
		if (!::OpenProcessToken(::GetCurrentProcess(), TOKEN_QUERY, hToken.addressof()))
			return false;
		TOKEN_ELEVATION elevation{};
		DWORD len;
		return ::GetTokenInformation(hToken.get(), TokenElevation, &elevation, sizeof(elevation), &len) && elevation.TokenIsElevated;
	}
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg) {
	if (CFrameWindowImpl<CMainFrame>::PreTranslateMessage(pMsg))
		return TRUE;
	return m_ActiveView && m_ActiveView->PreTranslateMessage(pMsg);
}

BOOL CMainFrame::OnIdle() {
	// the active view enables its own
	for (auto id : { ID_FLAGS_APPLY, ID_VIEW_REFRESH, ID_FLAGS_CLEARALL, ID_FLAGS_TOGGLE, ID_EDIT_COPY, ID_IMAGE_NEW, ID_IMAGE_DELETE,
		ID_IMAGE_SILENTEXIT, ID_IMAGE_PAGEHEAP, ID_IMAGE_VERIFIER }) {
		UIEnable(id, false);
	}
	if (m_ActiveView)
		m_ActiveView->UpdateUI(*this);
	for (int i = 0; i < (int)m_Views.size(); i++)
		UISetCheck(ID_VIEW_REGISTRY + i, m_Views[i] == m_ActiveView);
	UIUpdateToolBar();
	return FALSE;
}

void CMainFrame::SetStatusText(IView const* view, std::wstring const& text) {
	if (view == m_ActiveView)
		m_StatusBar.SetPaneText(ID_PANE_VIEW, text.c_str());
}

BOOL CMainFrame::ShowContextMenu(HMENU hMenu, POINT const& pt, DWORD flags) {
	// commands land in the frame and are routed to the active view
	return ::TrackPopupMenuEx(hMenu, flags, pt.x, pt.y, m_hWnd, nullptr);
}

void CMainFrame::ShowError(std::wstring const& message) const {
	AtlMessageBox(m_hWnd, message.c_str(), IDR_MAINFRAME, MB_ICONERROR);
}

LRESULT CMainFrame::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	InitImageList();
	InitToolBar();
	InitStatusBar();

	m_hWndClient = m_Tabs.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN, WS_EX_CLIENTEDGE);
	m_Tabs.SetImageList(m_Images);

	GetWindowText(m_Caption);
	if (IsRunningElevated())
		m_Caption += L" (Administrator)";

	UIAddMenu(GetMenu());

	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->AddMessageFilter(this);
	pLoop->AddIdleHandler(this);

	auto& settings = AppSettings::Get();
	ShowToolBar(settings.ToolBar());
	ShowStatusBar(settings.StatusBar());
	UISetCheck(ID_OPTIONS_DARKMODE, settings.DarkMode());

	// in ViewType order
	for (auto type : { ViewType::Registry, ViewType::Kernel }) {
		auto view = new CFlagsView(this, type);
		AddView(view, view->DoCreate(m_Tabs));
	}
	auto images = new CImageView(this);
	AddView(images, images->DoCreate(m_Tabs));
	m_Tabs.SetActivePage(std::clamp(settings.ActiveTab(), 0, (int)m_Views.size() - 1));

	SetDarkMode(settings.DarkMode());
	UpdateCaption();
	return 0;
}

void CMainFrame::AddView(IView* view, HWND hWnd) {
	// a view whose window failed to be made is a bug (it would delete itself as its window goes)
	ATLASSERT(hWnd);
	m_Views.push_back(view);
	// the page data is what COMMAND_TABVIEW_HANDLER routes the commands to
	m_Tabs.AddPage(hWnd, view->GetTitle(), int(m_Views.size() - 1), dynamic_cast<CMessageMap*>(view));
}

LRESULT CMainFrame::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	SaveSettings();

	// the views delete themselves as their windows are destroyed, and the tab view activates pages while removing
	// them (TBVN_PAGEACTIVATED), so stop talking to the views now
	m_Views.clear();
	m_ActiveView = nullptr;

	auto pLoop = _Module.GetMessageLoop();
	ATLASSERT(pLoop);
	pLoop->RemoveMessageFilter(this);
	pLoop->RemoveIdleHandler(this);

	bHandled = FALSE;
	return 1;
}

LRESULT CMainFrame::OnEndSession(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	// at logoff or shutdown no window is destroyed
	if (wParam)
		SaveSettings();
	return 0;
}

LRESULT CMainFrame::OnDpiChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/) {
	auto rc = reinterpret_cast<RECT const*>(lParam);
	SetWindowPos(nullptr, rc, SWP_NOZORDER | SWP_NOACTIVATE);

	InitImageList();
	m_Tabs.SetImageList(m_Images);
	auto dpi = DpiHelper::GetDpi(m_hWnd);
	for (auto view : m_Views)
		view->OnDpiChanged(dpi);

	ToolbarHelper::UpdateToolBarImages(m_ToolBar, s_ToolBarButtons, _countof(s_ToolBarButtons), ToolBarIconSize);
	m_ToolBar.AutoSize();
	InitMenu();
	m_StatusBar.SetPaneWidth(ID_PANE_VIEW, DpiHelper::Scale(500, m_hWnd));
	UpdateLayout();
	return 0;
}

LRESULT CMainFrame::OnPageActivated(int /*idCtrl*/, LPNMHDR hdr, BOOL& bHandled) {
	if (hdr->hwndFrom != m_Tabs) {
		bHandled = FALSE;
		return 0;
	}
	auto page = (int)hdr->idFrom;
	auto view = page >= 0 && page < (int)m_Views.size() ? m_Views[page] : nullptr;
	if (view != m_ActiveView) {
		if (m_ActiveView)
			m_ActiveView->OnActivate(false);
		m_ActiveView = view;
		if (view)
			view->OnActivate(true);
	}
	m_StatusBar.SetPaneText(ID_PANE_VIEW, view ? view->GetStatusText().c_str() : L"");
	UpdateCaption();
	return 0;
}

LRESULT CMainFrame::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
	for (int page = 0; page < (int)m_Views.size(); page++) {
		auto view = m_Views[page];
		if (view->IsModified()) {
			// the one asked about is shown
			m_Tabs.SetActivePage(page);
			if (!view->CanClose())
				return 0;
		}
	}
	bHandled = FALSE;
	return 0;
}

LRESULT CMainFrame::OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	PostMessage(WM_CLOSE);
	return 0;
}

LRESULT CMainFrame::OnViewTab(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	m_Tabs.SetActivePage(wID - ID_VIEW_REGISTRY);
	return 0;
}

LRESULT CMainFrame::OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool show = !::IsWindowVisible(m_hWndToolBar);
	ShowToolBar(show);
	auto& settings = AppSettings::Get();
	settings.ToolBar(show);
	settings.Save();
	return 0;
}

LRESULT CMainFrame::OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool show = !::IsWindowVisible(m_hWndStatusBar);
	ShowStatusBar(show);
	auto& settings = AppSettings::Get();
	settings.StatusBar(show);
	settings.Save();
	return 0;
}

LRESULT CMainFrame::OnDarkMode(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	auto& settings = AppSettings::Get();
	bool dark = !settings.DarkMode();
	settings.DarkMode(dark);
	settings.Save();
	UISetCheck(ID_OPTIONS_DARKMODE, dark);
	SetDarkMode(dark);
	return 0;
}

LRESULT CMainFrame::OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	CAboutDlg dlg;
	dlg.DoModal();
	return 0;
}

void CMainFrame::InitToolBar() {
	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	// the button images' size is at 96 DPI, scaled to the frame's
	m_ToolBar = ToolbarHelper::CreateAndInitToolBar(m_hWnd, s_ToolBarButtons, _countof(s_ToolBarButtons), ToolBarIconSize);
	m_ToolBar.SetExtendedStyle(m_ToolBar.GetExtendedStyle() | TBSTYLE_EX_MIXEDBUTTONS);
	AddSimpleReBarBand(m_ToolBar);
	UIAddToolBar(m_ToolBar);
}

void CMainFrame::InitMenu() {
	static const MenuItemData commands[] = {
		{ ID_FLAGS_APPLY, IDI_APPLY },
		{ ID_VIEW_REFRESH, IDI_REFRESH },
		{ ID_IMAGE_NEW, IDI_NEWIMAGE },
		{ ID_IMAGE_DELETE, IDI_DELETE },
		{ ID_IMAGE_SILENTEXIT, IDI_CONFIGURE },
		{ ID_IMAGE_PAGEHEAP, IDI_CONFIGURE },
		{ ID_IMAGE_VERIFIER, IDI_CONFIGURE },
		{ ID_EDIT_COPY, IDI_COPY },
		{ ID_VIEW_REGISTRY, IDI_REGISTRY },
		{ ID_VIEW_KERNEL, IDI_KERNEL },
		{ ID_VIEW_IMAGE, IDI_IMAGE },
	};
	WTLHelper::InitMenu(GetMenu(), commands, _countof(commands), DpiHelper::SmallIconSize(m_hWnd));
}

void CMainFrame::InitStatusBar() {
	CreateSimpleStatusBar();
	m_StatusBar.SubclassWindow(m_hWndStatusBar);
	m_StatusBar.SetPanes(s_Panes, _countof(s_Panes), false);
	m_StatusBar.SetPaneWidth(ID_PANE_VIEW, DpiHelper::Scale(500, m_hWnd));
}

void CMainFrame::InitImageList() {
	auto size = DpiHelper::SmallIconSize(m_hWnd);
	if (m_Images)
		m_Images.Destroy();
	m_Images.Create(size, size, ILC_COLOR32 | ILC_MASK, _countof(s_Icons), 4);
	for (auto id : s_Icons)
		m_Images.AddIcon(IconHelper::LoadCached(id, size));
}

void CMainFrame::ShowToolBar(bool show) {
	// m_hWndToolBar is the rebar hosting the toolbar
	::ShowWindow(m_hWndToolBar, show ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_TOOLBAR, show);
	UpdateLayout();
}

void CMainFrame::ShowStatusBar(bool show) {
	::ShowWindow(m_hWndStatusBar, show ? SW_SHOWNOACTIVATE : SW_HIDE);
	UISetCheck(ID_VIEW_STATUS_BAR, show);
	UpdateLayout();
}

void CMainFrame::SetDarkMode(bool dark) {
	WTLHelper::SwitchToMode(dark ? DarkModeKind::Dark : DarkModeKind::Light, m_hWnd);

	// menu icon bitmaps are drawn on the current menu background, so rebuild them for the new mode
	InitMenu();

	m_StatusBar.SetBkColor(dark ? DarkMode::getBackgroundColor() : CLR_DEFAULT);

	for (auto view : m_Views)
		view->OnThemeChanged(dark);

	::EnumThreadWindows(::GetCurrentThreadId(), [](auto h, auto) {
		::RedrawWindow(h, nullptr, nullptr, RDW_ERASENOW | RDW_INTERNALPAINT | RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN);
		return TRUE;
		}, 0);

	DrawMenuBar();
}

void CMainFrame::SaveSettings() {
	auto& settings = AppSettings::Get();
	settings.SaveWindowPosition(m_hWnd, AppSettings::MainWindowPlacement);
	if (auto page = m_Tabs.GetActivePage(); page >= 0)
		settings.ActiveTab(page);
	settings.Save();
}

void CMainFrame::UpdateCaption() {
	SetWindowText(m_ActiveView ? m_ActiveView->GetTitle() + L" - " + m_Caption : m_Caption);
}
