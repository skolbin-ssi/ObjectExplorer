#include "stdafx.h"
#include "WindowsView.h"
#include "WindowHelper.h"
#include "resource.h"

static PCWSTR properties[] = {
	L"Handle", L"Owner Thread", L"Owner Process", L"Style", L"Extended Style", L"Class Name", L"Rectangle"
};

CWindowsView::CWindowsView(IMainFrame* frame) : m_pFrame(frame), m_SearchCombo(this, 1), m_SearchEdit(this, 2) {
}

void CWindowsView::SetDesktopOptions(bool defaultDesktopOnly) {
	m_DefaultDesktopOnly = defaultDesktopOnly;
}

LRESULT CWindowsView::OnCreate(UINT, WPARAM, LPARAM, BOOL&) {
	auto hWndToolBar = m_Toolbar.Create(m_hWnd, nullptr, nullptr, ATL_SIMPLE_TOOLBAR_PANE_STYLE | TBSTYLE_LIST, 0, ATL_IDW_TOOLBAR);
	m_Toolbar.SetExtendedStyle(TBSTYLE_EX_MIXEDBUTTONS);
	InitTreeToolbar(m_Toolbar);

	CreateSimpleReBar(ATL_SIMPLE_REBAR_NOBORDER_STYLE);
	AddSimpleReBarBand(m_Toolbar);

	m_hWndClient = m_Splitter.Create(m_hWnd, rcDefault, nullptr, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS, WS_EX_CLIENTEDGE);

	m_List.Create(m_Splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE |
		WS_CLIPCHILDREN | WS_CLIPSIBLINGS | LVS_REPORT | LVS_OWNERDATA | LVS_SINGLESEL | LVS_NOSORTHEADER);
	m_Tree.Create(m_Splitter, rcDefault, nullptr, WS_CHILD | WS_VISIBLE |
		WS_CLIPCHILDREN | WS_CLIPSIBLINGS | TVS_LINESATROOT | TVS_HASBUTTONS | TVS_HASLINES | TVS_SHOWSELALWAYS);

	CImageList images;
	images.Create(16, 16, ILC_COLOR | ILC_COLOR32, 2, 0);
	UINT icons[] = { IDI_DESKTOP, IDI_WINDOW, IDI_NOACCESS, IDI_WINDOW_BLACK };
	for (auto icon : icons)
		images.AddIcon(AtlLoadIcon(icon));

	m_Tree.SetImageList(images, TVSIL_NORMAL);

	m_List.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
	m_List.InsertColumn(0, L"Property", LVCFMT_LEFT, 160);
	m_List.InsertColumn(1, L"Value", LVCFMT_LEFT, 250);
	m_List.InsertColumn(2, L"Details", LVCFMT_LEFT, 550);

	m_Splitter.SetSplitterExtendedStyle(SPLIT_FLATBAR | SPLIT_PROPORTIONAL);
	m_Splitter.SetSplitterPanes(m_Tree, m_List);
	m_Splitter.SetSplitterPosPct(30);

	InitTree();

	return 0;
}

LRESULT CWindowsView::OnTreeSelectionChanged(int, LPNMHDR, BOOL&) {
	UpdateList(false);

	return 0;
}

LRESULT CWindowsView::OnListGetDispInfo(int, LPNMHDR hdr, BOOL&) {
	auto lv = (NMLVDISPINFO*)hdr;
	auto& item = lv->item;
	auto index = item.iItem;

	if (item.mask & LVIF_TEXT) {
		switch (item.iSubItem) {
			case 0:
				item.pszText = (PWSTR)properties[index];
				break;

			case 1:
				if (::IsWindow(m_SelectedHwnd))
					::StringCchCopy(item.pszText, item.cchTextMax, GetPropertyValue(m_SelectedHwnd, index));
				break;

			case 2:
				if (::IsWindow(m_SelectedHwnd))
					::StringCchCopy(item.pszText, item.cchTextMax, GetPropertyDetails(m_SelectedHwnd, index));
				break;
		}
	}

	return 0;
}

LRESULT CWindowsView::OnRefresh(WORD, WORD, HWND, BOOL&) {
	UpdateList(false);
	return 0;
}

LRESULT CWindowsView::OnRefreshTree(WORD, WORD, HWND, BOOL&) {
	InitTree();
	return 0;
}

LRESULT CWindowsView::OnRefreshTreeVisible(WORD, WORD, HWND, BOOL&) {
	if (m_Toolbar.IsButtonChecked(IdOnlyVisible))
		m_TreeViewOptions |= TreeViewOptions::VisibleOnly;
	else
		m_TreeViewOptions &= ~TreeViewOptions::VisibleOnly;

	InitTree();
	return 0;
}

LRESULT CWindowsView::OnRefreshTreeWithTitle(WORD, WORD, HWND, BOOL&) {
	if (m_Toolbar.IsButtonChecked(IdOnlyWithTitle))
		m_TreeViewOptions |= TreeViewOptions::WithTitleOnly;
	else
		m_TreeViewOptions &= ~TreeViewOptions::WithTitleOnly;

	InitTree();
	return 0;
}

LRESULT CWindowsView::OnFind(WORD, WORD, HWND, BOOL&) {
	m_SearchCombo.SetFocus();
	return 0;
}

LRESULT CWindowsView::OnFindNext(WORD, WORD, HWND, BOOL&) {
	CString text;
	m_SearchEdit.GetWindowText(text);
	if (text.IsEmpty())
		return 0;

	text.MakeLower();
	auto item = m_Tree.GetSelectedItem();
	if (item == nullptr)
		item = m_Tree.GetRootItem();

	CString itemText;
	while (item) {
		auto next = item.GetChild();
		if (next == nullptr)
			next = item.GetNext(TVGN_NEXT);
		if (next == nullptr)
			break;
		next.GetText(itemText);
		ATLTRACE(L"Item text: %s\n", itemText);
		if (itemText.MakeLower().Find(text) >= 0) {
			next.Select(TVGN_CARET);
			next.EnsureVisible();
			m_Tree.SetFocus();
			break;
		}
		item = next;
	}
	return 0;
}

LRESULT CWindowsView::OnComboKeyDown(UINT, WPARAM wParam, LPARAM, BOOL& handled) {
	if (wParam == VK_RETURN) {
		PostMessage(WM_COMMAND, ID_EDIT_FIND_NEXT);
		CString text;
		m_SearchEdit.GetWindowText(text);
		m_SearchCombo.InsertString(0, text);
		if (m_SearchCombo.GetCount() > 10)
			m_SearchCombo.DeleteString(10);
		return 0;
	}
	handled = FALSE;
	return 0;
}

LRESULT CWindowsView::OnActivate(UINT, WPARAM activate, LPARAM, BOOL&) {
	if (activate) {
		auto ui = m_pFrame->GetUpdateUI();
		ui->UIEnable(ID_EDIT_FIND, TRUE);
		ui->UIEnable(ID_EDIT_FIND_NEXT, TRUE);
	}
	return 0;
}

void CWindowsView::InitTree() {
	m_Tree.LockWindowUpdate();
	m_Tree.DeleteAllItems();
	m_ProcMgr.EnumProcesses();

	if (m_DefaultDesktopOnly) {
		auto root = m_Tree.InsertItem(L"Default", 0, 0, TVI_ROOT, TVI_LAST);
		for (auto h : ObjectManager::EnumDsktopWindows(nullptr)) {
			InsertWindow(h, root);
		}
		root.Expand(TVE_EXPAND);
	}
	else {
		m_ObjMgr.EnumHandlesAndObjects(L"Desktop");
		for (auto& obj : m_ObjMgr.GetObjects()) {
			auto root = m_Tree.InsertItem(obj->Name.IsEmpty() ? L"(Unnamed)" : obj->Name, 0, 0, TVI_ROOT, TVI_LAST);
			if (!obj->Handles.empty()) {
				auto hDesktop = ObjectManager::DupHandle(obj.get(), DESKTOP_ENUMERATE);
				if (hDesktop) {
					CString text;
					for (auto h : ObjectManager::EnumDsktopWindows(hDesktop)) {
						InsertWindow(h, root);
					}
					::CloseHandle(hDesktop);
				}
				else {
					CString text;
					root.GetText(text);
					root.SetText(text + L" <access denied>");
					root.SetImage(2, 2);
				}
			}
		}
	}
	m_Tree.LockWindowUpdate(FALSE);
}

void CWindowsView::UpdateList(bool newNode) {
	auto item = m_Tree.GetSelectedItem();
	if (item == nullptr || item.GetData() == 0) {
		m_List.SetItemCount(0);
		return;
	}
	m_SelectedHwnd = (HWND)item.GetData();
	int image = ::IsWindowVisible(m_SelectedHwnd) ? 1 : 3;
	item.SetImage(image, image);

	if (newNode) {
		m_List.SetItemCount(_countof(properties));
	}
	else {
		m_List.SetItemCountEx(_countof(properties), LVSICF_NOINVALIDATEALL | LVSICF_NOSCROLL);
		m_List.RedrawItems(m_List.GetTopIndex(), m_List.GetTopIndex() + m_List.GetCountPerPage());
	}
}

#define ID_VIEW_SEARCH 0x510

void CWindowsView::InitTreeToolbar(CToolBarCtrl& tb) {
	CImageList tbImages;
	tbImages.Create(24, 24, ILC_COLOR32, 8, 4);
	tb.SetImageList(tbImages);

	struct {
		UINT id;
		int image;
		int style = BTNS_BUTTON;
		int state = TBSTATE_ENABLED;
		PCWSTR text = nullptr;
	} buttons[] = {
		{ IdRefreshTree, IDI_REFRESH },
		{ 0 },
		{ IdOnlyVisible, IDI_WINDOW, BTNS_CHECK, TBSTATE_CHECKED | TBSTATE_ENABLED },
		{ IdOnlyWithTitle, IDI_WINDOW_TITLE, BTNS_CHECK, TBSTATE_ENABLED, L"With Title" },
		{ 0 },
		{ ID_VIEW_SEARCH, IDI_FIND, BTNS_SHOWTEXT, TBSTATE_ENABLED, L"Find: " },
	};
	for (auto& b : buttons) {
		if (b.id == 0)
			tb.AddSeparator(0);
		else {
			int image = b.image == 0 ? I_IMAGENONE : tbImages.AddIcon(AtlLoadIconImage(b.image, 0, 24, 24));
			tb.AddButton(b.id, b.style, b.state, image, b.text, 0);
		}
	}

	auto hCombo = CreateToolbarComboBox(tb, ID_VIEW_SEARCH, 24, 16, CBS_DROPDOWN);
	m_SearchCombo.SubclassWindow(hCombo);

	COMBOBOXINFO cbi = { sizeof(cbi) };
	m_SearchCombo.GetComboBoxInfo(&cbi);
	m_SearchEdit.SubclassWindow(cbi.hwndItem);
}

CTreeItem CWindowsView::InsertWindow(HWND hWnd, HTREEITEM hParent) {
	auto image = ::IsWindowVisible(hWnd) ? 1 : 3;
	if (image == 3 && (m_TreeViewOptions & TreeViewOptions::VisibleOnly) == TreeViewOptions::VisibleOnly)
		return nullptr;

	WCHAR title[64];
	auto len = ::GetWindowTextLength(hWnd);
	if (len > 0)
		::GetWindowText(hWnd, title, _countof(title));
	else if ((m_TreeViewOptions & TreeViewOptions::WithTitleOnly) == TreeViewOptions::WithTitleOnly)
		return nullptr;

	CString text;
	text.Format(L"0x%X", HandleToULong(hWnd));
	if (len > 0)
		text += L" [" + CString(title) + (len > _countof(title) ? L"..." : L"") + L"]";
	auto item = m_Tree.InsertItem(text, image, image, hParent, TVI_LAST);

	item.SetData((DWORD_PTR)hWnd);
	return item;
}

CString CWindowsView::GetPropertyValue(HWND hWnd, int index) const {
	CString text;
	switch (index) {
		case 0:	// handle
			text.Format(L"0x%08X", hWnd);
			break;
		case 1:	// thread
		case 2:	// process
		{
			DWORD pid;
			auto tid = ::GetWindowThreadProcessId(hWnd, &pid);
			text.Format(L"%u (0x%X)", index == 1 ? tid : pid);
			break;
		}

		case 3:	// style
			text.Format(L"0x%08X", CWindow(hWnd).GetStyle());
			break;

		case 4:	// extended style
			text.Format(L"0x%08X", CWindow(hWnd).GetExStyle());
			break;

		case 5:	// class name
			::GetClassName(hWnd, text.GetBufferSetLength(128), 128);
			break;

		case 6:	// rectangle
			text = WindowHelper::WindowRectToString(hWnd);
			break;
	}

	return text;
}

CString CWindowsView::GetPropertyDetails(HWND hWnd, int index) const {
	CString text;
	switch (index) {
		case 2:	// process
		{
			DWORD pid;
			::GetWindowThreadProcessId(hWnd, &pid);
			text = m_ProcMgr.GetProcessNameById(pid).c_str();
			break;
		}

		case 3:
			text = WindowHelper::WindowStyleToString(hWnd);
			break;

		case 4:
			text = WindowHelper::WindowExtendedStyleToString(hWnd);
			break;

		case 5:	// class name
			::RealGetWindowClass(hWnd, text.GetBufferSetLength(128), 128);
			break;

	}

	return text;
}
