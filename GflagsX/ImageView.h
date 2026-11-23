#pragma once

#include "resource.h"
#include "ViewBase.h"
#include "FlagsPane.h"
#include "ImageSettingsForm.h"
#include "FlagStore.h"
#include <VirtualListView.h>
#include <CustomSplitterWindow.h>
#include <atlscrl.h>

//
// The image list with a filter box above it. The list's notifications (and the box's) go on to the parent: the
// splitter, which passes them to the view.
//
class CFilteredListHost : public CWindowImpl<CFilteredListHost> {
public:
	DECLARE_WND_CLASS(L"GflagsXFilteredList")

	CEdit Filter;
	CWindow List;

	BEGIN_MSG_MAP(CFilteredListHost)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		FORWARD_NOTIFICATIONS()
	END_MSG_MAP()

private:
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
};

//
// The executables with Image File Execution Options keys (those whose names have the filter's text), left to right: the images (with their flags, debugger and
// mitigations), the selected image's flags, and its other settings (CImageSettingsForm, in a scroll container: it can
// be taller than the pane). Apply writes the flags and the settings together; changes that weren't applied are asked
// about before another image is shown (the selection goes back if the user keeps them), before Reload, and at exit.
//
class CImageView :
	public CViewBase<CImageView>,
	public CVirtualListView<CImageView> {
public:
	explicit CImageView(IMainFrame* frame);

	// IView
	ViewType GetType() const override {
		return ViewType::Image;
	}
	CString GetTitle() const override {
		return L"Image";
	}
	UINT GetIcon() const override {
		return IDI_IMAGE;
	}
	std::wstring GetStatusText() const override;
	bool IsModified() const override;
	bool CanClose() override;
	void OnActivate(bool active) override;
	void UpdateUI(CUpdateUIBase& ui) override;
	void OnDpiChanged(UINT dpi) override;

	// the settings form's keyboard navigation (Tab, Enter), from the frame
	BOOL PreTranslateMessage(MSG* pMsg) override;

	// CVirtualListView
	CString GetColumnText(HWND, int row, int col) const;
	void DoSort(SortInfo const* si);
	void OnStateChanged(HWND, int from, int to, UINT oldState, UINT newState);
	bool OnRightClickList(HWND, int row, int col, POINT const& pt);

	BEGIN_MSG_MAP(CImageView)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		MESSAGE_HANDLER(WM_TIMER, OnTimer)
		NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnListKeyDown)
		COMMAND_HANDLER(IDC_FILTER, EN_CHANGE, OnFilterChanged)
		CHAIN_MSG_MAP(CVirtualListView<CImageView>)
		CHAIN_MSG_MAP(CViewBase<CImageView>)
	ALT_MSG_MAP(1)
		COMMAND_ID_HANDLER(ID_FLAGS_APPLY, OnApply)
		COMMAND_ID_HANDLER(ID_VIEW_REFRESH, OnReload)
		COMMAND_ID_HANDLER(ID_FLAGS_CLEARALL, OnClearAll)
		COMMAND_ID_HANDLER(ID_FLAGS_TOGGLE, OnToggle)
		COMMAND_ID_HANDLER(ID_EDIT_COPY, OnCopy)
		COMMAND_ID_HANDLER(ID_EDIT_FIND, OnFind)
		COMMAND_ID_HANDLER(ID_IMAGE_NEW, OnNewImage)
		COMMAND_ID_HANDLER(ID_IMAGE_DELETE, OnDeleteImage)
		COMMAND_ID_HANDLER(ID_IMAGE_SILENTEXIT, OnSilentProcessExit)
		COMMAND_ID_HANDLER(ID_IMAGE_PAGEHEAP, OnPageHeap)
		COMMAND_ID_HANDLER(ID_IMAGE_VERIFIER, OnVerifier)
		CHAIN_MSG_MAP_ALT(CViewBase<CImageView>, 1)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	static constexpr UINT_PTR SelectTimer = 1;
	static constexpr UINT SelectDelay = 100;	// ms: the selection settles before the image is shown

	enum class Column { Name, GlobalFlag, Debugger, Mitigations };

	struct Image {
		std::wstring Name;
		FlagStore::ImageSettings Settings;
		bool Readable{ true };
	};

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled);
	LRESULT OnListKeyDown(int /*idCtrl*/, LPNMHDR hdr, BOOL& bHandled);
	LRESULT OnFilterChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnFind(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnNewImage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnDeleteImage(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnSilentProcessExit(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	// the dialogs' settings are the form's, written on Apply; OK checks the flag that turns them on
	LRESULT OnPageHeap(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnVerifier(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);

	// the images from the registry; the one shown (if it's still there) stays selected
	void LoadImages();
	// the list's images: those whose names have the filter's text, sorted
	void ShowFiltered();
	// the selected image in the flags and the form (none: they're empty); false if the user kept the shown one's changes
	bool ShowSelected();
	// shows the image (read again), or none (empty name)
	void ShowImage(std::wstring const& name);
	// the shown image's settings as the panes have them
	FlagStore::ImageSettings EditedSettings() const;
	bool Apply();
	// unapplied changes: apply (Yes), discard (No) or keep them (Cancel: false)
	bool AskAboutChanges();
	int FindImage(std::wstring const& name) const;
	void SelectImage(std::wstring const& name);
	std::wstring SilentProcessExitText() const;
	void UpdateStatus() const;

	CCustomSplitterWindow m_Splitter;		// the images | m_Right
	CCustomSplitterWindow m_Right;			// the flags | the settings
	CFilteredListHost m_ListHost;			// the filter box and m_List
	CListViewCtrl m_List;
	CFlagsPane m_Flags;
	CScrollContainer m_FormContainer;
	CImageSettingsForm m_Form;
	std::vector<Image> m_AllImages;			// as read
	std::vector<Image> m_Images;			// the list's: the filter's
	std::wstring m_Filter;
	std::wstring m_Shown;					// the image in the panes
	FlagStore::ImageSettings m_ShownSettings;	// as read
	bool m_Restoring{ false };				// putting the selection back on the shown image
	std::wstring m_Error;					// reading the images failed
};
