#pragma once

#include <FrameView.h>
#include <WTLHelper.h>
#include "Interfaces.h"

//
// WTLHelper's window creation hook (WH_CALLWNDPROCRET) routes every SendMessage on the thread through the kernel.
// Painting a large list sends thousands of messages (custom draw, display info) and creates no windows, so the hook
// is suspended while the given window paints.
//
inline LRESULT CALLBACK SuspendHookPaintSubclass(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam, UINT_PTR id, DWORD_PTR) {
	switch (msg) {
		case WM_PAINT:
		{
			SuspendResumeHook noHook;
			return ::DefSubclassProc(hWnd, msg, wParam, lParam);
		}
		case WM_NCDESTROY:
			::RemoveWindowSubclass(hWnd, SuspendHookPaintSubclass, id);
			break;
	}
	return ::DefSubclassProc(hWnd, msg, wParam, lParam);
}

inline void SuspendHookWhilePainting(HWND hWnd) {
	::SetWindowSubclass(hWnd, SuspendHookPaintSubclass, 0, 0);
}

//
// Base for the tab views. Views are heap-allocated, created with DoCreate and delete themselves when destroyed (CFrameView).
// Commands are routed from the frame to the active view's ALT_MSG_MAP(1); derived views must end that section
// with CHAIN_MSG_MAP_ALT(CViewBase<T>, 1).
//
template<typename T, typename TBase = CWindow>
class CViewBase :
	public CFrameView<T, IMainFrame, TBase>,
	public IView,
	public CAutoUpdateUI<T>,
	public CIdleHandler {
public:
	using FrameView = CFrameView<T, IMainFrame, TBase>;

	explicit CViewBase(IMainFrame* frame) : FrameView(frame) {}

	HWND DoCreate(HWND hParent) {
		if (!this->Create(hParent, this->rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN))
			return nullptr;

		_Module.GetMessageLoop()->AddIdleHandler(this);
		return this->m_hWnd;
	}

	HWND GetHwnd() const override {
		return this->m_hWnd;
	}

	BOOL OnIdle() override {
		static_cast<T*>(this)->UIUpdateToolBar();
		return FALSE;
	}

	BEGIN_MSG_MAP(CViewBase)
		MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
		CHAIN_MSG_MAP(CAutoUpdateUI<T>)
		CHAIN_MSG_MAP(typename FrameView::BaseFrame)
	ALT_MSG_MAP(1)
	END_MSG_MAP()

private:
	LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
		_Module.GetMessageLoop()->RemoveIdleHandler(this);
		bHandled = FALSE;
		return 0;
	}
};
