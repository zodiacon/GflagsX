#include "pch.h"
#include "AboutDlg.h"
#include <VersionResourceHelper.h>

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	SetIcon(AtlLoadIconImage(IDR_MAINFRAME, LR_SHARED, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON)), TRUE);
	SetIcon(AtlLoadIconImage(IDR_MAINFRAME, LR_SHARED, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON)), FALSE);

	CenterWindow(GetParent());
	// the exe's own version resource
	VersionResourceHelper vh;
	SetDlgItemText(IDC_VERSION, vh.GetValue(L"ProductName") + L" v" + vh.GetValue(L"ProductVersion"));
	SetDlgItemText(IDC_COPYRIGHT, vh.GetValue(L"LegalCopyright"));
	return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	EndDialog(wID);
	return 0;
}

LRESULT CAboutDlg::OnClickLink(int /*idCtrl*/, LPNMHDR hdr, BOOL& /*bHandled*/) {
	if (hdr->idFrom == IDC_LINK)
		::ShellExecute(nullptr, L"open", L"https://github.com/zodiacon/GflagsX", nullptr, nullptr, SW_SHOWDEFAULT);
	return 0;
}
