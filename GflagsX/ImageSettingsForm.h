#pragma once

#include "resource.h"
#include "FlagStore.h"

//
// The Image tab's settings of the selected image besides its flags: the debugger, the Silent Process Exit summary with
// its Configure button, and the mitigation options: the value in hex, DEP's bits as check boxes and each mitigation
// field (Mitigations::Fields) as a combo box, made at run time below them. The hex value and the controls follow each
// other; the value's bits no control shows are kept. OnChanged tells the host of the user's changes.
//
class CImageSettingsForm : public CDialogImpl<CImageSettingsForm> {
public:
	enum { IDD = IDD_IMAGESETTINGS };

	// shows the image's settings: not a change
	void SetSettings(FlagStore::ImageSettings const& settings);
	// the debugger and the mitigation options as shown (the rest stays)
	void GetSettings(FlagStore::ImageSettings& settings) const;
	void SetSilentProcessExitText(std::wstring const& text);
	// no image selected: empty and disabled
	void Enable(bool enable);
	// the size the controls need (the host's pane can be smaller)
	CSize GetIdealSize() const;

	std::function<void()> OnChanged;
	std::function<void()> OnSilentProcessExit;

	BEGIN_MSG_MAP(CImageSettingsForm)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_HANDLER(IDC_DEBUGGER, EN_CHANGE, OnDebuggerChanged)
		COMMAND_ID_HANDLER(IDC_DEBUGGER_BROWSE, OnBrowseDebugger)
		COMMAND_HANDLER(IDC_MITIGATIONS, EN_KILLFOCUS, OnValueKillFocus)
		COMMAND_ID_HANDLER(IDOK, OnEnter)
		COMMAND_ID_HANDLER(IDCANCEL, OnEscape)
		COMMAND_RANGE_CODE_HANDLER(IDC_DEP, IDC_SEHOP, BN_CLICKED, OnBitClicked)
		COMMAND_RANGE_CODE_HANDLER(IDC_FIELD_FIRST, IDC_FIELD_FIRST + 31, CBN_SELCHANGE, OnFieldChanged)
		COMMAND_ID_HANDLER(IDC_SILENTEXIT, OnSilentExit)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDebuggerChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBrowseDebugger(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnValueKillFocus(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	// Enter in the hex value takes it (the frame's dialog navigation sends IDOK), Escape puts the value back
	LRESULT OnEnter(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnEscape(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnBitClicked(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFieldChanged(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSilentExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// the controls from m_Mitigations
	void ShowMitigations();
	// the hex text as the value, if it's one; otherwise it goes back to the value
	void TakeValueText();
	void Changed();

	uint64_t m_Mitigations{ 0 };
	CSize m_IdealSize;
	bool m_Updating{ false };	// setting the controls: no changes to tell
};
