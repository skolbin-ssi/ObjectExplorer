#pragma once

#include "resource.h"
#include "VirtualListView.h"
#include <ProcessManager.h>

struct ObjectInfo;
struct HandleInfo;

class CObjectHandlesDlg : 
	public CDialogImpl<CObjectHandlesDlg>,
	public CDialogResize<CObjectHandlesDlg>,
	public CVirtualListView<CObjectHandlesDlg> {
public:
	CObjectHandlesDlg(ObjectInfo* info, WinSys::ProcessManager& pm);
	enum { IDD = IDD_HANDLES };

	void DoSort(const SortInfo* si);
	bool CompareItems(const HandleInfo* h1, const HandleInfo* h2, int col, bool asc);

	BEGIN_DLGRESIZE_MAP(CObjectHandlesDlg)
		BEGIN_DLGRESIZE_GROUP()
			DLGRESIZE_CONTROL(IDC_HANDLES, DLSZ_SIZE_X | DLSZ_SIZE_Y)
		END_DLGRESIZE_GROUP()
	END_DLGRESIZE_MAP()

	BEGIN_MSG_MAP(CObjectHandlesDlg)
		MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
		COMMAND_ID_HANDLER(IDCANCEL, OnCloseCmd)
		NOTIFY_CODE_HANDLER(LVN_GETDISPINFO, OnGetDispInfo)
		CHAIN_MSG_MAP(CVirtualListView<CObjectHandlesDlg>)
		CHAIN_MSG_MAP(CDialogResize<CObjectHandlesDlg>)
	END_MSG_MAP()

private:
	LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
	LRESULT OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
	LRESULT OnGetDispInfo(int, LPNMHDR hdr, BOOL&);

private:
	CListViewCtrl m_List;
	ObjectInfo* m_pObject;
	std::vector<std::shared_ptr<HandleInfo>> m_Handles;
	WinSys::ProcessManager& m_ProcMgr;
};

