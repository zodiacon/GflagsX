#pragma once

// the tabs, in their order
enum class ViewType {
	Registry,	// the system's flags in the registry, for the next boot
	Kernel,		// the running system's flags
	Image,		// the executables' settings (Image File Execution Options)
};

// what the views see of the frame
struct IMainFrame abstract {
	virtual HWND GetHwnd() const = 0;
	virtual CUpdateUIBase& GetUI() = 0;
	// the status bar's second pane: the active view's summary (a view that isn't active is ignored)
	virtual void SetStatusText(struct IView const* view, std::wstring const& text) = 0;
	virtual BOOL ShowContextMenu(HMENU hMenu, POINT const& pt, DWORD flags = 0) = 0;
	virtual void ShowError(std::wstring const& message) const = 0;
};

struct IView abstract {
	virtual ViewType GetType() const = 0;
	virtual CString GetTitle() const = 0;
	virtual UINT GetIcon() const = 0;
	virtual HWND GetHwnd() const = 0;
	// the status bar text while the view is active
	virtual std::wstring GetStatusText() const = 0;

	// changes that weren't applied
	virtual bool IsModified() const = 0;
	// before the app exits: false keeps it open (the user canceled applying or discarding the changes)
	virtual bool CanClose() = 0;
	virtual void OnActivate(bool /*active*/) {}
	// on idle while active: the frame's commands that depend on the view (Apply...)
	virtual void UpdateUI(CUpdateUIBase& /*ui*/) {}
	virtual void OnThemeChanged(bool /*dark*/) {}
	virtual void OnDpiChanged(UINT /*dpi*/) {}
	// a key message before it's dispatched (after the frame's accelerators): a form's dialog navigation
	virtual BOOL PreTranslateMessage(MSG* /*pMsg*/) {
		return FALSE;
	}
};
