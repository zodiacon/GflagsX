#include "pch.h"
#include "FlagsPane.h"
#include "ViewBase.h"
#include "resource.h"
#include <SortHelper.h>
#include <DpiHelper.h>
#include <ListViewhelper.h>

using namespace GFlags;

CFlagsPane::CFlagsPane(IMainFrame* frame, Target target) : m_Frame(frame) {
	for (auto flag : ForTarget(target))
		m_Rows.push_back({ flag });
}

HWND CFlagsPane::DoCreate(HWND hParent) {
	return Create(hParent, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS);
}

LRESULT CFlagsPane::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_List.Create(m_hWnd, rcDefault, nullptr,
		WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_TABSTOP | LVS_REPORT | LVS_OWNERDATA | LVS_SHOWSELALWAYS);
	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES | LVS_EX_INFOTIP);
	SuspendHookWhilePainting(m_List);

	auto scale = [this](int width) { return DpiHelper::Scale(width, m_hWnd); };
	m_List.InsertColumn(0, L"Flag", LVCFMT_LEFT, scale(300));
	m_List.InsertColumn(1, L"Value", LVCFMT_RIGHT, scale(90));
	m_List.InsertColumn(2, L"Affects", LVCFMT_LEFT, scale(140));
	ListViewHelper::EnableColumnDpiScaling(m_List);
	m_List.SetItemCount((int)m_Rows.size());
	return 0;
}

LRESULT CFlagsPane::OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	if (m_List) {
		CRect rc;
		GetClientRect(&rc);
		m_List.MoveWindow(&rc);
	}
	return 0;
}

LRESULT CFlagsPane::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_List.SetFocus();
	return 0;
}

void CFlagsPane::OnDpiChanged() {
	// the columns follow by themselves (EnableColumnDpiScaling); the check boxes' images are the system's
	m_List.Invalidate();
}

void CFlagsPane::SetValue(uint32_t value) {
	for (auto& row : m_Rows)
		row.Checked = (value & row.Flag->Value) == row.Flag->Value;
	m_List.Invalidate();
}

uint32_t CFlagsPane::GetValue() const {
	uint32_t value = 0;
	for (auto& row : m_Rows)
		if (row.Checked)
			value |= row.Flag->Value;
	return value;
}

void CFlagsPane::Check(uint32_t flag, bool check) {
	for (int i = 0; i < (int)m_Rows.size(); i++) {
		if (m_Rows[i].Flag->Value == flag && m_Rows[i].Checked != check) {
			m_Rows[i].Checked = check;
			m_List.RedrawItems(i, i);
			Changed();
		}
	}
}

void CFlagsPane::Toggle(int row) {
	if (row < 0 || row >= (int)m_Rows.size() || !m_List.IsWindowEnabled())
		return;
	m_Rows[row].Checked = !m_Rows[row].Checked;
	m_List.RedrawItems(row, row);
	Changed();
}

void CFlagsPane::ToggleSelected() {
	if (!m_List.IsWindowEnabled())
		return;
	// all as the first one becomes, as Explorer's check boxes do
	int first = m_List.GetNextItem(-1, LVNI_SELECTED);
	if (first < 0)
		return;
	bool check = !m_Rows[first].Checked;
	bool changed = false;
	for (int row = first; row >= 0; row = m_List.GetNextItem(row, LVNI_SELECTED)) {
		if (m_Rows[row].Checked != check) {
			m_Rows[row].Checked = check;
			changed = true;
		}
	}
	m_List.Invalidate();
	if (changed)
		Changed();
}

void CFlagsPane::ClearAll() {
	if (GetValue() == 0)
		return;
	SetValue(0);
	Changed();
}

bool CFlagsPane::HasSelection() const {
	return m_List.GetSelectedCount() > 0;
}

std::wstring CFlagsPane::GetSelectedText() const {
	std::wstring text;
	for (int row = m_List.GetNextItem(-1, LVNI_SELECTED); row >= 0; row = m_List.GetNextItem(row, LVNI_SELECTED)) {
		auto flag = m_Rows[row].Flag;
		text += std::format(L"{}\t{}\t{}\r\n", flag->Name, FormatValue(flag->Value), CategoryName(flag->Category));
	}
	return text;
}

bool CFlagsPane::HasFocus() const {
	return ::GetFocus() == m_List;
}

void CFlagsPane::Enable(bool enable) {
	if (!enable)
		SetValue(0);
	m_List.EnableWindow(enable);
}

void CFlagsPane::Changed() {
	if (OnChanged)
		OnChanged();
}

CString CFlagsPane::GetColumnText(HWND, int row, int col) const {
	auto flag = m_Rows[row].Flag;
	switch (static_cast<Column>(col)) {
		case Column::Name: return flag->Name;
		case Column::Value: return FormatValue(flag->Value).c_str();
		case Column::Category: return CategoryName(flag->Category);
	}
	return L"";
}

ListViewRowCheck CFlagsPane::IsRowChecked(HWND, int row) const {
	return m_Rows[row].Checked ? ListViewRowCheck::Checked : ListViewRowCheck::Unchecked;
}

void CFlagsPane::DoSort(SortInfo const* si) {
	auto asc = si->SortAscending;
	std::ranges::stable_sort(m_Rows, [&](auto& r1, auto& r2) {
		switch (static_cast<Column>(si->SortColumn)) {
			case Column::Name: return SortHelper::Sort(r1.Flag->Name, r2.Flag->Name, asc);
			case Column::Value: return SortHelper::Sort(r1.Flag->Value, r2.Flag->Value, asc);
			case Column::Category: return SortHelper::Sort(CategoryName(r1.Flag->Category), CategoryName(r2.Flag->Category), asc);
		}
		return false;
		});
}

void CFlagsPane::OnListViewClick(HWND hList, int /*row*/, int /*col*/, POINT const& pt) {
	if (hList != m_List)
		return;
	LVHITTESTINFO info{};
	info.pt = pt;
	if (m_List.HitTest(&info) >= 0 && (info.flags & LVHT_ONITEMSTATEICON))
		Toggle(info.iItem);
}

bool CFlagsPane::OnDoubleClickList(HWND hList, int row, int /*col*/, POINT const& pt) {
	if (hList != m_List || row < 0)
		return false;
	// a double click on the check box is two clicks, each of which toggled it
	CPoint client(pt);
	m_List.ScreenToClient(&client);
	LVHITTESTINFO info{};
	info.pt = client;
	if (m_List.HitTest(&info) >= 0 && !(info.flags & LVHT_ONITEMSTATEICON))
		Toggle(row);
	return true;
}

LRESULT CFlagsPane::OnKeyDown(int /*idCtrl*/, LPNMHDR hdr, BOOL& bHandled) {
	auto kd = reinterpret_cast<NMLVKEYDOWN*>(hdr);
	if (hdr->hwndFrom != m_List || kd->wVKey != VK_SPACE) {
		bHandled = FALSE;
		return 0;
	}
	ToggleSelected();
	return 0;
}

LRESULT CFlagsPane::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
	if ((HWND)wParam != m_List) {
		bHandled = FALSE;
		return 0;
	}
	CPoint pt(GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam));
	if (pt.x == -1 && pt.y == -1) {
		// from the keyboard: under the focused row (or the list's corner)
		CRect rc;
		int focused = m_List.GetNextItem(-1, LVNI_FOCUSED);
		if (focused < 0 || !m_List.GetItemRect(focused, &rc, LVIR_LABEL))
			rc.SetRect(0, 0, 0, 0);
		pt.SetPoint(rc.left, rc.bottom);
		m_List.ClientToScreen(&pt);
	}
	CMenu menu;
	menu.LoadMenu(IDR_CONTEXT);
	m_Frame->ShowContextMenu(menu.GetSubMenu(0), pt);
	return 0;
}
