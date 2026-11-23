#pragma once

#include "resource.h"
#include "FlagStore.h"

//
// The system's special pool and object reference tracing, of the registry (from the next boot) or of the running
// system (until it restarts): the Registry and Kernel tabs' dialogs. The settings are the dialog's on OK.
//
class CSpecialPoolDlg : public CDialogImpl<CSpecialPoolDlg> {
public:
	enum { IDD = IDD_SPECIALPOOL };

	CSpecialPoolDlg(bool kernel, FlagStore::SpecialPoolSettings settings);

	FlagStore::SpecialPoolSettings const& GetSettings() const {
		return m_Settings;
	}

	BEGIN_MSG_MAP(CSpecialPoolDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_SPP_ENABLE, OnEnable)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void UpdateEnabled();

	bool m_Kernel;
	FlagStore::SpecialPoolSettings m_Settings;
};

class CObjectTraceDlg : public CDialogImpl<CObjectTraceDlg> {
public:
	enum { IDD = IDD_OBJECTTRACE };

	CObjectTraceDlg(bool kernel, FlagStore::ObjectTraceSettings settings);

	FlagStore::ObjectTraceSettings const& GetSettings() const {
		return m_Settings;
	}

	BEGIN_MSG_MAP(CObjectTraceDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_OBT_ENABLE, OnEnable)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnEnable(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	void UpdateEnabled();

	bool m_Kernel;
	FlagStore::ObjectTraceSettings m_Settings;
};
