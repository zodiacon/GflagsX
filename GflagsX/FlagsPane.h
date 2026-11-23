#pragma once

#include "GlobalFlags.h"
#include "Interfaces.h"
#include <VirtualListView.h>

//
// A target's flags as a list with check boxes: the flag, its value and what it affects, sortable by any of them. A click
// on a check box, Space or a double click checks or unchecks; OnChanged tells the host (the view, which keeps the value
// read and knows what changed). Not a view: a child window the views host.
//
class CFlagsPane :
	public CWindowImpl<CFlagsPane>,
	public CVirtualListView<CFlagsPane> {
public:
	DECLARE_WND_CLASS_EX(L"GflagsX_FlagsPane", 0, COLOR_WINDOW)

	CFlagsPane(IMainFrame* frame, GFlags::Target target);

	HWND DoCreate(HWND hParent);

	// checks the flags in the value (the others are left out): not a change
	void SetValue(uint32_t value);
	// the checked flags
	uint32_t GetValue() const;
	void Check(uint32_t flag, bool check);
	void ToggleSelected();
	void ClearAll();
	bool HasSelection() const;
	// the selected flags as lines of text: name, value, what it affects
	std::wstring GetSelectedText() const;
	bool HasFocus() const;
	// no image to show the flags of: unchecked and disabled
	void Enable(bool enable);
	void OnDpiChanged();

	// a flag was checked or unchecked by the user (not by SetValue)
	std::function<void()> OnChanged;

	// CVirtualListView
	CString GetColumnText(HWND, int row, int col) const;
	ListViewRowCheck IsRowChecked(HWND, int row) const;
	void DoSort(SortInfo const* si);
	void OnListViewClick(HWND, int row, int col, POINT const& pt);
	bool OnDoubleClickList(HWND, int row, int col, POINT const& pt);

	BEGIN_MSG_MAP(CFlagsPane)
		MESSAGE_HANDLER(WM_CREATE, OnCreate)
		MESSAGE_HANDLER(WM_SIZE, OnSize)
		MESSAGE_HANDLER(WM_SETFOCUS, OnSetFocus)
		MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
		NOTIFY_CODE_HANDLER(LVN_KEYDOWN, OnKeyDown)
		CHAIN_MSG_MAP(CVirtualListView<CFlagsPane>)
	END_MSG_MAP()

// Handler prototypes (uncomment arguments if needed):
//	LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
//	LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
//	LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

private:
	enum class Column { Name, Value, Category };

	struct Row {
		GFlags::Flag const* Flag;
		bool Checked{ false };
	};

	LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
	LRESULT OnKeyDown(int /*idCtrl*/, LPNMHDR hdr, BOOL& bHandled);

	void Toggle(int row);
	void Changed();

	IMainFrame* m_Frame;
	CListViewCtrl m_List;
	std::vector<Row> m_Rows;
};
