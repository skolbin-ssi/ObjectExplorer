#include "pch.h"
#include "ColumnManager.h"
#include <atlctrls.h>

class ListCtrlHelper {
public:
	template<typename T = int>
	static T FindColumn(CHeaderCtrl header, int id) {
		auto count = header.GetItemCount();
		HDITEM hdi;
		hdi.mask = HDI_LPARAM;
		for (int i = 0; i < count; i++) {
			header.GetItem(i, &hdi);
			if (hdi.lParam == id)
				return static_cast<T>(i);
		}
		return static_cast<T>(-1);
	}

	template<typename T = int>
	static T FindColumn(CListViewCtrl list, int id) {
		return FindColumn(list.GetHeader(), id);
	}

	template<typename T = int>
	static T GetRealColumn(CListViewCtrl& list, int index) {
		return static_cast<T>(GetRealColumn(list.GetHeader(), index));
	}

	template<typename T = int>
	static T GetRealColumn(CHeaderCtrl header, int index) {
		HDITEM hdi;
		hdi.mask = HDI_LPARAM;
		header.GetItem(index, &hdi);
		return static_cast<T>(hdi.lParam);
	}
};

ColumnManager::~ColumnManager() = default;

bool ColumnManager::CopyTo(ColumnManager & other) const {
	if (other.GetCount() != GetCount())
		return false;

	int i = 0;
	for (const auto& column : m_Columns) {
		other.SetColumn(i, column);
		i++;
	}
	return true;
}

void ColumnManager::AddFromControl(HWND hWnd) {
	CHeaderCtrl header(hWnd == nullptr ? m_ListView.GetHeader() : CListViewCtrl(hWnd).GetHeader());
	ATLASSERT(header);
	auto count = header.GetItemCount();
	HDITEM item;
	WCHAR text[64];
	item.pszText = text;
	item.cchTextMax = _countof(text);
	item.mask = HDI_FORMAT | HDI_WIDTH | HDI_TEXT;
	for (int i = 0; i < count; i++) {
		ATLVERIFY(header.GetItem(i, &item));
		ColumnInfo info;
		info.DefaultWidth = item.cxy;
		info.Flags = (info.DefaultWidth <= 1 && ((item.fmt & HDF_FIXEDWIDTH) > 0) ? ColumnFlags::Visible : ColumnFlags::None);
		info.Name = item.pszText;
		m_Columns.push_back(info);
	}
}

void ColumnManager::SetVisible(int column, bool visible) {
	m_Columns[column].SetVisible(visible);
}

bool ColumnManager::IsVisible(int column) const {
	ATLASSERT(column >= 0 && column < GetCount());
	return (m_Columns[column].Flags & ColumnFlags::Visible) == ColumnFlags::Visible;
}

bool ColumnManager::IsModified(int column) const {
	return (GetColumn(column).Flags & ColumnFlags::Modified) == ColumnFlags::Modified;
}

void ColumnManager::SetModified(int column, bool modified) {
	auto& col = m_Columns[column];
	if (modified)
		col.Flags |= ColumnFlags::Modified;
	else
		col.Flags &= ~ColumnFlags::Modified;
}

bool ColumnManager::IsConst(int column) const {
	return (m_Columns[column].Flags & ColumnFlags::Const) == ColumnFlags::Const;
}

int ColumnManager::AddColumn(PCWSTR name, int format, int width, ColumnFlags flags, int tag) {
	auto category = ::wcschr(name, L'\\');
	CString categoryName;
	if (category) {
		categoryName = CString(name, static_cast<int>(category - name));
		name = category + 1;
	}
	else {
		categoryName = L"General";
	}
	ColumnInfo info;
	info.Format = format;
	info.DefaultWidth = width;
	info.Flags = flags;
	info.Name = name;
	info.Category = categoryName;
	info.Tag = tag;

	if (m_ListView && ((flags & ColumnFlags::Visible) == ColumnFlags::Visible)) {
		auto header = m_ListView.GetHeader();
		int i = m_ListView.InsertColumn(header.GetItemCount(), name, format, width);
		HDITEM hdi;
		hdi.mask = HDI_LPARAM;
		hdi.lParam = m_Columns.size();
		header.SetItem(i, &hdi);
	}

	m_Columns.push_back(info);
	if (!categoryName.IsEmpty()) {
		if (std::find(m_Categories.begin(), m_Categories.end(), categoryName) == m_Categories.end())
			m_Categories.push_back(categoryName);
		m_ColumnsByCategory[categoryName].push_back(static_cast<int>(m_Columns.size() - 1));
	}

	return static_cast<int>(m_Columns.size());
}

int ColumnManager::GetColumnIndex(int tag) const {
	auto count = (int)m_Columns.size();
	for (int i = 0; i < count; i++)
		if (m_Columns[i].Tag == tag)
			return i;
	return -1;
}

void ColumnManager::Clear() {
	m_Columns.clear();
}

void ColumnManager::UpdateColumns() {
	auto header = m_ListView.GetHeader();
	HDITEM hdi;
	hdi.mask = HDI_LPARAM;
	for (int i = 0; i < GetCount(); i++) {
		if (IsModified(i)) {
			if (IsVisible(i)) {
				auto& info = GetColumn(i);
				// make visible - add column
				int c = m_ListView.InsertColumn(header.GetItemCount(), info.Name, info.Format, info.DefaultWidth);
				hdi.lParam = i;
				header.SetItem(c, &hdi);
			}
			else {
				int c = ListCtrlHelper::FindColumn(header, i);
				ATLASSERT(c >= 0);
				m_ListView.DeleteColumn(c);
			}
			SetModified(i, false);
		}
	}
}

const ColumnManager::ColumnInfo& ColumnManager::GetColumn(int index) const {
	return m_Columns[index];
}

const std::vector<int>& ColumnManager::GetColumnsByCategory(PCWSTR category) const {
	return m_ColumnsByCategory.at(category);
}

const std::vector<CString>& ColumnManager::GetCategories() const {
	return m_Categories;
}

void ColumnManager::SetColumn(int i, const ColumnInfo & info) {
	ATLASSERT(i >= 0 && i < GetCount());
	if ((info.Flags & ColumnFlags::Visible) != (m_Columns[i].Flags & ColumnFlags::Visible)) {
		SetVisible(i, (info.Flags & ColumnFlags::Visible) == ColumnFlags::Visible);
	}
}

int ColumnManager::GetRealColumn(int index) const {
	HDITEM hdi;
	hdi.mask = HDI_LPARAM;
	m_ListView.GetHeader().GetItem(index, &hdi);
	return static_cast<int>(hdi.lParam);
}
