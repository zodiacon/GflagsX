#pragma once

#include "resource.h"
#include "Interfaces.h"
#include <NativeCustomTabView.h>
#include <TabViewHelper.h>

// the frame's client: a tab per view (Registry, Kernel, Image), always there: no close buttons, no dragging
class CMainTabView : public CNativeCustomTabViewImpl<CMainTabView> {
public:
	DECLARE_WND_CLASS_EX(_T("GflagsX_TabView"), 0, COLOR_APPWORKSPACE)

	CMainTabView() {
		m_bDestroyImageList = false;
		m_bTabCloseButton = false;
		m_bNoTabDrag = true;
	}
};

//
// The main window. The views are made with it, one per tab in ViewType order (m_Views[page] is the page's), and the
// frame's commands go to the active one (COMMAND_TABVIEW_HANDLER). Closing asks every view about its changes first.
//
class CMainFrame :
	public CFrameWindowImpl<CMainFrame>,
	public CAutoUpdateUI<CMainFrame>,
	public IMainFrame,
	public CMessageFilter,
	public CIdleHandler {
public:
	DECLARE_FRAME_WND_CLASS(L"GflagsXFrame", IDR_MAINFRAME)

	BOOL PreTranslateMessage(MSG* pMsg) override;
	BOOL OnIdle() override;

	// IMainFrame
	HWND GetHwnd() const override {
		return m_hWnd;
	}
	CUpdateUIBase& GetUI() override {
		return *this;
	}
	void SetStatusText(IView const* view, std::wstring const& text) override;
	BOOL ShowContextMenu(HMENU hMenu, POINT const& pt, DWORD flags = 0) override;
	void ShowError(std::wstring const& message) const override;

	BEGIN_MSG_MAP(CMainFrame)
		COMMAND_TABVIEW_HANDLER(m_Tabs, 1)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_CLOSE, OnClose)
		MESSAGE_HANDLER(WM_ENDSESSION, OnEndSession)
		MESSAGE_HANDLER(WM_DPICHANGED, OnDpiChanged)
		NOTIFY_CODE_HANDLER(TBVN_PAGEACTIVATED, OnPageActivated)
		COMMAND_ID_HANDLER(ID_APP_EXIT, OnFileExit)
		COMMAND_RANGE_HANDLER(ID_VIEW_REGISTRY, ID_VIEW_IMAGE, OnViewTab)
		COMMAND_ID_HANDLER(ID_VIEW_TOOLBAR, OnViewToolBar)
		COMMAND_ID_HANDLER(ID_VIEW_STATUS_BAR, OnViewStatusBar)
		COMMAND_ID_HANDLER(ID_OPTIONS_DARKMODE, OnDarkMode)
		COMMAND_ID_HANDLER(ID_APP_ABOUT, OnAppAbout)
		CHAIN_MSG_MAP(CAutoUpdateUI<CMainFrame>)
		CHAIN_MSG_MAP(CFrameWindowImpl<CMainFrame>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnEndSession(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDpiChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM lParam, BOOL& /*bHandled*/);
	LRESULT OnPageActivated(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);
	LRESULT OnFileExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewTab(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewToolBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnViewStatusBar(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDarkMode(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	// the views with changes that weren't applied may keep the app open
	LRESULT OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnAppAbout(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void InitToolBar();
	void InitMenu();
	void InitStatusBar();
	void InitImageList();
	void AddView(IView* view, HWND hWnd);
	void ShowToolBar(bool show);
	void ShowStatusBar(bool show);
	void SetDarkMode(bool dark);
	void SaveSettings();
	// "<active tab> - GFlagsX (Administrator)"
	void UpdateCaption();

	static constexpr int ToolBarIconSize = 24;	// at 96 DPI

	CMainTabView m_Tabs;
	CToolBarCtrl m_ToolBar;
	CMultiPaneStatusBarCtrl m_StatusBar;
	CImageList m_Images;
	std::vector<IView*> m_Views;	// by page (ViewType order)
	IView* m_ActiveView{ nullptr };
	CString m_Caption;	// the app's own, without a tab's title
};
