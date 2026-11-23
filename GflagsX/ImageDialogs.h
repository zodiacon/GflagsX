#pragma once

#include "resource.h"
#include "FlagStore.h"

// an image's name for a new key: typed, or an executable's file name (Browse)
class CNewImageDlg : public CDialogImpl<CNewImageDlg> {
public:
	enum { IDD = IDD_NEWIMAGE };

	std::wstring const& GetName() const {
		return m_Name;
	}

	BEGIN_MSG_MAP(CNewImageDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_NAME, EN_CHANGE, OnNameChanged)
		COMMAND_ID_HANDLER(IDC_BROWSE, OnBrowse)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnNameChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowse(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// the typed name, trimmed; what's wrong with it (empty: nothing) shown under it, OK enabled if nothing
	void Validate();

	std::wstring m_Name;
};

//
// An image's Silent Process Exit settings (Windows' monitoring of processes that end other than by an exception):
// ignoring self exits, a monitor process, a notification, and dumps (folder, type, modules whose loading skips them).
//
class CSilentProcessExitDlg : public CDialogImpl<CSilentProcessExitDlg> {
public:
	enum { IDD = IDD_SILENTEXIT };

	CSilentProcessExitDlg(std::wstring image, FlagStore::SilentProcessExitSettings settings);

	FlagStore::SilentProcessExitSettings const& GetSettings() const {
		return m_Settings;
	}

	BEGIN_MSG_MAP(CSilentProcessExitDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDC_LAUNCHMONITOR, OnCheckChanged)
		COMMAND_ID_HANDLER(IDC_DUMP, OnCheckChanged)
		COMMAND_ID_HANDLER(IDC_DUMPFOLDER_BROWSE, OnBrowseFolder)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCheckChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowseFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// the monitor's and the dumps' controls follow their check boxes
	void UpdateEnabled();
	std::wstring GetText(int id) const;

	std::wstring m_Image;
	FlagStore::SilentProcessExitSettings m_Settings;
};
