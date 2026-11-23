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
		COMMAND_HANDLER(IDC_DUMPTYPE, CBN_SELCHANGE, OnDumpTypeChanged)
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
	LRESULT OnDumpTypeChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowseFolder(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// the monitor's and the dumps' controls follow their check boxes; the dump type's value is typed for Custom only
	void UpdateEnabled();
	// the DumpType value of a preset in its box (Custom: the last value typed or read stays)
	void ShowDumpType();
	std::wstring GetText(int id) const;

	std::wstring m_Image;
	FlagStore::SilentProcessExitSettings m_Settings;
};

//
// An image's page heap options (PageHeap.h): Full, the options as check boxes, and those with values. Full's options
// (and the size range, random and DLLs, which choose the allocations full page heap gets) set Full, as gflags.exe does,
// and are off without it. PageHeapFlags' bits no control shows are kept.
//
class CPageHeapDlg : public CDialogImpl<CPageHeapDlg> {
public:
	enum { IDD = IDD_PAGEHEAP };

	CPageHeapDlg(std::wstring image, FlagStore::PageHeapSettings settings);

	FlagStore::PageHeapSettings const& GetSettings() const {
		return m_Settings;
	}

	BEGIN_MSG_MAP(CPageHeapDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_RANGE_CODE_HANDLER(IDC_PH_FULL, IDC_PH_FAULTS, BN_CLICKED, OnCheckChanged)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCheckChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// the flags as the check boxes have them (with the bits no check box shows)
	uint32_t CheckedFlags() const;
	// the controls Full and the value check boxes turn on and off, and the flags' text
	void UpdateControls();
	std::wstring GetText(int id) const;

	std::wstring m_Image;
	FlagStore::PageHeapSettings m_Settings;
};

//
// An image's Application Verifier settings that Windows reads: the providers to load (the installed ones, and the
// image's own others, as check boxes) and the flags in hex.
//
class CVerifierDlg : public CDialogImpl<CVerifierDlg> {
public:
	enum { IDD = IDD_VERIFIER };

	CVerifierDlg(std::wstring image, FlagStore::VerifierSettings settings, std::vector<std::wstring> providers);

	FlagStore::VerifierSettings const& GetSettings() const {
		return m_Settings;
	}

	BEGIN_MSG_MAP(CVerifierDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDOK, OnOK)
		COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnOK(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	std::wstring m_Image;
	FlagStore::VerifierSettings m_Settings;
	std::vector<std::wstring> m_Providers;
	CListViewCtrl m_List;
};
