#include "pch.h"
#include "FlagsView.h"
#include "FlagStore.h"
#include "Helpers.h"
#include "SystemDialogs.h"
#include <ClipboardHelper.h>

using namespace GFlags;

namespace {
	Target TargetOf(ViewType type) {
		return type == ViewType::Kernel ? Target::Kernel : Target::Registry;
	}
}

CFlagsView::CFlagsView(IMainFrame* frame, ViewType type) : CViewBase(frame), m_Target(TargetOf(type)), m_Type(type),
	m_Pane(frame, TargetOf(type)), m_Mask(TargetMask(TargetOf(type))) {
	m_Pane.OnChanged = [this] { UpdateStatus(); };
}

CString CFlagsView::GetTitle() const {
	return m_Type == ViewType::Kernel ? L"Kernel" : L"Registry";
}

UINT CFlagsView::GetIcon() const {
	return m_Type == ViewType::Kernel ? IDI_KERNEL : IDI_REGISTRY;
}

LRESULT CFlagsView::OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_hWndClient = m_Pane.DoCreate(m_hWnd);
	Load();
	return 0;
}

LRESULT CFlagsView::OnSetFocus(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
	m_Pane.SetFocus();
	return 0;
}

void CFlagsView::Load() {
	auto value = m_Type == ViewType::Kernel ? FlagStore::ReadKernelFlags() : FlagStore::ReadRegistryFlags(FlagStore::Locations::System());
	m_Loaded = value.has_value();
	m_Error = m_Loaded ? L"" : Helpers::ErrorText(value.error());
	m_Value = value.value_or(0);
	m_Pane.SetValue(m_Value);
	m_Pane.Enable(m_Loaded);
	UpdateStatus();
}

uint32_t CFlagsView::NewValue() const {
	return (m_Value & ~m_Mask) | m_Pane.GetValue();
}

bool CFlagsView::IsModified() const {
	return m_Loaded && NewValue() != m_Value;
}

bool CFlagsView::Apply() {
	auto value = NewValue();
	auto error = m_Type == ViewType::Kernel ? FlagStore::WriteKernelFlags(value) :
		FlagStore::WriteRegistryFlags(FlagStore::Locations::System(), value);
	if (error != ERROR_SUCCESS) {
		Frame()->ShowError(std::format(L"Setting the {} flags failed.\n\n{}", m_Type == ViewType::Kernel ? L"kernel" : L"registry",
			Helpers::ErrorText(error)));
		return false;
	}
	Load();
	// the kernel takes the flags it knows: those it didn't are shown unchecked now, and said so
	if (m_Type == ViewType::Kernel && m_Value != value) {
		std::wstring names;
		for (auto& flag : All())
			if ((value & flag.Value) && !(m_Value & flag.Value))
				names += std::format(L"\n{}", flag.Name);
		if (!names.empty())
			AtlMessageBox(Frame()->GetHwnd(), (L"The kernel didn't set these flags:\n" + names).c_str(), IDR_MAINFRAME, MB_ICONWARNING);
	}
	return true;
}

bool CFlagsView::CanClose() {
	if (!IsModified())
		return true;
	auto text = std::format(L"The {} flags were changed. Apply the changes?", m_Type == ViewType::Kernel ? L"kernel" : L"registry");
	switch (AtlMessageBox(Frame()->GetHwnd(), text.c_str(), IDR_MAINFRAME, MB_ICONQUESTION | MB_YESNOCANCEL)) {
		case IDYES: return Apply();
		case IDNO: return true;
	}
	return false;
}

std::wstring CFlagsView::GetStatusText() const {
	if (!m_Loaded)
		return std::format(L"Reading the flags failed: {}", m_Error);
	auto text = std::format(L"{}: {}", m_Type == ViewType::Kernel ? L"Running system's flags" : L"Flags for the next boot", FormatValue(m_Value));
	if (IsModified())
		text += std::format(L" (changed to {}: not applied)", FormatValue(NewValue()));
	// bits of flags that aren't this target's (kept on Apply): seen nowhere else
	if (auto other = m_Value & ~m_Mask)
		text += std::format(L"; set with no check box here: {}", FormatValue(other));
	return text;
}

void CFlagsView::UpdateStatus() const {
	Frame()->SetStatusText(this, GetStatusText());
}

void CFlagsView::OnActivate(bool active) {
	if (active)
		m_Pane.SetFocus();
}

void CFlagsView::UpdateUI(CUpdateUIBase& ui) {
	ui.UIEnable(ID_FLAGS_SPECIALPOOL, true);
	ui.UIEnable(ID_FLAGS_OBJECTTRACE, true);
	ui.UIEnable(ID_FLAGS_APPLY, IsModified());
	ui.UIEnable(ID_VIEW_REFRESH, true);
	ui.UIEnable(ID_FLAGS_CLEARALL, m_Loaded && m_Pane.GetValue() != 0);
	ui.UIEnable(ID_FLAGS_TOGGLE, m_Loaded && m_Pane.HasSelection());
	ui.UIEnable(ID_EDIT_COPY, m_Pane.HasSelection());
}

void CFlagsView::OnDpiChanged(UINT /*dpi*/) {
	m_Pane.OnDpiChanged();
}

LRESULT CFlagsView::OnApply(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (IsModified())
		Apply();
	return 0;
}

LRESULT CFlagsView::OnReload(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (IsModified() && AtlMessageBox(Frame()->GetHwnd(), L"Discard the changes and read the flags again?", IDR_MAINFRAME,
		MB_ICONQUESTION | MB_OKCANCEL) != IDOK)
		return 0;
	Load();
	return 0;
}

LRESULT CFlagsView::OnClearAll(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	m_Pane.ClearAll();
	return 0;
}

LRESULT CFlagsView::OnToggle(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	m_Pane.ToggleSelected();
	return 0;
}

LRESULT CFlagsView::OnCopy(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	if (auto text = m_Pane.GetSelectedText(); !text.empty())
		ClipboardHelper::CopyText(m_hWnd, text.c_str());
	return 0;
}

LRESULT CFlagsView::OnSpecialPool(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool kernel = m_Type == ViewType::Kernel;
	auto settings = kernel ? FlagStore::ReadKernelSpecialPool() : FlagStore::ReadSpecialPool(FlagStore::Locations::System());
	if (!settings) {
		Frame()->ShowError(L"Reading the special pool settings failed.\n\n" + Helpers::ErrorText(settings.error()));
		return 0;
	}
	CSpecialPoolDlg dlg(kernel, *settings);
	if (dlg.DoModal(Frame()->GetHwnd()) != IDOK || dlg.GetSettings() == *settings)
		return 0;
	auto error = kernel ? FlagStore::WriteKernelSpecialPool(dlg.GetSettings()) :
		FlagStore::WriteSpecialPool(FlagStore::Locations::System(), dlg.GetSettings());
	if (error != ERROR_SUCCESS)
		Frame()->ShowError(L"Writing the special pool settings failed.\n\n" + Helpers::ErrorText(error));
	return 0;
}

LRESULT CFlagsView::OnObjectTrace(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
	bool kernel = m_Type == ViewType::Kernel;
	auto settings = kernel ? FlagStore::ReadKernelObjectTrace() : FlagStore::ReadObjectTrace(FlagStore::Locations::System());
	if (!settings) {
		Frame()->ShowError(L"Reading the object reference tracing settings failed.\n\n" + Helpers::ErrorText(settings.error()));
		return 0;
	}
	CObjectTraceDlg dlg(kernel, *settings);
	if (dlg.DoModal(Frame()->GetHwnd()) != IDOK || dlg.GetSettings() == *settings)
		return 0;
	auto error = kernel ? FlagStore::WriteKernelObjectTrace(dlg.GetSettings()) :
		FlagStore::WriteObjectTrace(FlagStore::Locations::System(), dlg.GetSettings());
	if (error != ERROR_SUCCESS)
		Frame()->ShowError(L"Writing the object reference tracing settings failed.\n\n" + Helpers::ErrorText(error));
	return 0;
}
