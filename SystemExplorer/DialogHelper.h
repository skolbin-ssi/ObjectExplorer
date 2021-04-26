#pragma once

class DialogHelper final abstract {
public:
	static void AdjustOKCancelButtons(CWindow* dlg);
	static bool AddIconToButton(CWindow* dlg, WORD id, WORD icon, int size = 16);
	static void SetDialogIcon(CWindow* dlg, UINT icon);
	static void SetDialogIcon(CWindow* dlg, HICON icon);
};

