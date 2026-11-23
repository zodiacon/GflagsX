#pragma once

#include "resource.h"
#include "ViewBase.h"
#include "FlagsPane.h"

//
// The system's flags: in the registry (ViewType::Registry, read at boot) or the running kernel's (ViewType::Kernel).
// The flags are checked as they were read; Apply writes them (the bits no flag of the tab covers are written as they
// were read), Reload reads them again. Changes that weren't applied are asked about before they're lost.
//
class CFlagsView : public CViewBase<CFlagsView> {
public:
	CFlagsView(IMainFrame* frame, ViewType type);

	// IView
	ViewType GetType() const override {
		return m_Type;
	}
	CString GetTitle() const override;
	UINT GetIcon() const override;
	std::wstring GetStatusText() const override;
	bool IsModified() const override;
	bool CanClose() override;
	void OnActivate(bool active) override;
	void UpdateUI(CUpdateUIBase& ui) override;
	void OnDpiChanged(UINT dpi) override;

	BEGIN_MSG_MAP(CFlagsView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		CHAIN_MSG_MAP(CViewBase<CFlagsView>)
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_FLAGS_APPLY, OnApply)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnReload)
		COMMAND_ID_HANDLER(ID_FLAGS_CLEARALL, OnClearAll)
		COMMAND_ID_HANDLER(ID_FLAGS_TOGGLE, OnToggle)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnCopy)
		CHAIN_MSG_MAP_ALT(CViewBase<CFlagsView>, 1)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// reads the flags (an error leaves the tab with nothing checked, and says so)
	void Load();
	bool Apply();
	// the value Apply writes: the checked flags, the other bits as read
	uint32_t NewValue() const;
	void UpdateStatus() const;

	GFlags::Target m_Target;
	ViewType m_Type;
	CFlagsPane m_Pane;
	uint32_t m_Value{ 0 };		// as read
	uint32_t m_Mask;			// the bits the tab's flags cover
	bool m_Loaded{ false };
	std::wstring m_Error;		// reading failed
};
