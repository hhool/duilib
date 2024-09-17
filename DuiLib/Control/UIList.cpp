#include "StdAfx.h"

namespace DuiLib {
/////////////////////////////////////////////////////////////////////////////////////
//
#define VIR_ITEM_HEIGHT 30//虚表项的默认高度

class CListBodyUI : public CVerticalLayoutUI
{
public:
    CListBodyUI(CListUI* pOwner);

    void SetScrollPos(SIZE szPos);
    void SetPos(RECT rc, bool bNeedInvalidate = true);
    void DoEvent(TEventUI& event);
    bool DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl);
    bool SortItems(PULVCompareFunc pfnCompare, UINT_PTR dwData, int& iCurSel);
protected:
    static int __cdecl ItemComareFunc(void *pvlocale, const void *item1, const void *item2);
    int __cdecl ItemComareFunc(const void *item1, const void *item2);

protected:
    CListUI* m_pOwner;
    PULVCompareFunc m_pCompareFunc;
	UINT_PTR m_compareData;
};

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListUI::CListUI() : m_pCallback(NULL), m_bScrollSelect(false), m_iCurSel(-1), m_iExpandedItem(-1), m_iSelectControlTag(-1)
{
	m_nDrawStartIndex = 0;
	m_nMaxShowCount = 0;
	m_bEnableMouseWhell = false;
	m_nVirtualItemHeight = VIR_ITEM_HEIGHT;
	m_nVirtualItemCount = 0;
	m_pVirutalItemFormat = NULL;
	m_ePanelPos = E_PANELBOTTOM;//Panel的位置，目前仅仅支持上下
	m_nPanelHeight = 20;//Panel高度
	m_nPanelOffset =  0;//默认偏移10
    m_pList = new CListBodyUI(this);
    m_pHeader = new CListHeaderUI;
	m_pFloatPanel = new CChildLayoutUI;
	m_pFloatPanel->SetVisible(false);
	m_pFloatPanel->SetFloat(true);
	m_pFloatPanel->SetFixedHeight(m_nPanelHeight);
	m_pFloatPanel->SetMouseEnabled(false);
	m_bUseVirtualList = false;
	m_bEnableVirtualO = true;
    Add(m_pHeader);
    CVerticalLayoutUI::Add(m_pList);
	CVerticalLayoutUI::Add(m_pFloatPanel);

    m_ListInfo.nColumns = 0;
    m_ListInfo.uFixedHeight = 0;
    m_ListInfo.nFont = -1;
    m_ListInfo.uTextStyle = DT_VCENTER | DT_SINGLELINE; // m_uTextStyle(DT_VCENTER | DT_END_ELLIPSIS)
    m_ListInfo.dwTextColor = 0xFF000000;
    m_ListInfo.dwBkColor = 0;
    m_ListInfo.bAlternateBk = false;
    m_ListInfo.dwSelectedTextColor = 0xFF000000;
    m_ListInfo.dwSelectedBkColor = 0xFFC1E3FF;
    m_ListInfo.dwHotTextColor = 0xFF000000;
    m_ListInfo.dwHotBkColor = 0xFFE9F5FF;
    m_ListInfo.dwDisabledTextColor = 0xFFCCCCCC;
    m_ListInfo.dwDisabledBkColor = 0xFFFFFFFF;
    m_ListInfo.iHLineSize = 0;
    m_ListInfo.dwHLineColor = 0xFF3C3C3C;
    m_ListInfo.iVLineSize = 0;
    m_ListInfo.dwVLineColor = 0xFF3C3C3C;
    m_ListInfo.bShowHtml = false;
    m_ListInfo.bMultiExpandable = false;
    ::ZeroMemory(&m_ListInfo.rcTextPadding, sizeof(m_ListInfo.rcTextPadding));
    ::ZeroMemory(&m_ListInfo.rcColumn, sizeof(m_ListInfo.rcColumn));
	::ZeroMemory(&m_ListInfo.bUsedHeaderContain, sizeof(m_ListInfo.bUsedHeaderContain));
}

LPCTSTR CListUI::GetClass() const
{
    return DUI_CTR_LIST;
}

UINT CListUI::GetControlFlags() const
{
    return UIFLAG_TABSTOP;
}

LPVOID CListUI::GetInterface(LPCTSTR pstrName)
{
	if( _tcscmp(pstrName, DUI_CTR_LIST) == 0 ) return static_cast<CListUI*>(this);
    if( _tcscmp(pstrName, DUI_CTR_ILIST) == 0 ) return static_cast<IListUI*>(this);
    if( _tcscmp(pstrName, DUI_CTR_ILISTOWNER) == 0 ) return static_cast<IListOwnerUI*>(this);
    return CVerticalLayoutUI::GetInterface(pstrName);
}

CControlUI* CListUI::GetItemAt(int iIndex) const
{
    return m_pList->GetItemAt(iIndex);
}

int CListUI::GetItemIndex(CControlUI* pControl) const
{
    if( pControl->GetInterface(DUI_CTR_LISTHEADER) != NULL ) return CVerticalLayoutUI::GetItemIndex(pControl);
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), DUI_CTR_LISTHEADERITEM) != NULL ) return m_pHeader->GetItemIndex(pControl);

    return m_pList->GetItemIndex(pControl);
}

bool CListUI::SetItemIndex(CControlUI* pControl, int iIndex)
{
    if( pControl->GetInterface(DUI_CTR_LISTHEADER) != NULL ) return CVerticalLayoutUI::SetItemIndex(pControl, iIndex);
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), DUI_CTR_LISTHEADERITEM) != NULL ) return m_pHeader->SetItemIndex(pControl, iIndex);

    int iOrginIndex = m_pList->GetItemIndex(pControl);
    if( iOrginIndex == -1 ) return false;
    if( iOrginIndex == iIndex ) return true;

    IListItemUI* pSelectedListItem = NULL;
    if( m_iCurSel >= 0 ) pSelectedListItem = 
        static_cast<IListItemUI*>(GetItemAt(m_iCurSel)->GetInterface(DUI_CTR_ILISTITEM));
    if( !m_pList->SetItemIndex(pControl, iIndex) ) return false;
    int iMinIndex = min(iOrginIndex, iIndex);
    int iMaxIndex = max(iOrginIndex, iIndex);
    for(int i = iMinIndex; i < iMaxIndex + 1; ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(DUI_CTR_ILISTITEM));
        if( pListItem != NULL ) {
            pListItem->SetIndex(i);
        }
    }
    if( m_iCurSel >= 0 && pSelectedListItem != NULL ) m_iCurSel = pSelectedListItem->GetIndex();
    return true;
}

bool CListUI::SetMultiItemIndex(CControlUI* pStartControl, int iCount, int iNewStartIndex)
{
    if (pStartControl == NULL || iCount < 0 || iNewStartIndex < 0) return false;
    if( pStartControl->GetInterface(DUI_CTR_LISTHEADER) != NULL ) return CVerticalLayoutUI::SetMultiItemIndex(pStartControl, iCount, iNewStartIndex);
    // We also need to recognize header sub-items
    if( _tcsstr(pStartControl->GetClass(), DUI_CTR_LISTHEADERITEM) != NULL ) return m_pHeader->SetMultiItemIndex(pStartControl, iCount, iNewStartIndex);

    int iStartIndex = GetItemIndex(pStartControl);
    if (iStartIndex == iNewStartIndex) return true;
    if (iStartIndex + iCount > GetItemCount()) return false;
	if (iNewStartIndex + iCount > GetItemCount()) return false;

    IListItemUI* pSelectedListItem = NULL;
    if( m_iCurSel >= 0 ) pSelectedListItem = 
        static_cast<IListItemUI*>(GetItemAt(m_iCurSel)->GetInterface(DUI_CTR_ILISTITEM));
    if( !m_pList->SetMultiItemIndex(pStartControl, iCount, iNewStartIndex) ) return false;
    int iMinIndex = min(iStartIndex, iNewStartIndex);
    int iMaxIndex = max(iStartIndex + iCount, iNewStartIndex + iCount);
    for(int i = iMinIndex; i < iMaxIndex + 1; ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(DUI_CTR_ILISTITEM));
        if( pListItem != NULL ) {
            pListItem->SetIndex(i);
        }
    }
    if( m_iCurSel >= 0 && pSelectedListItem != NULL ) m_iCurSel = pSelectedListItem->GetIndex();
    return true;
}

int CListUI::GetCount() const
{
	if (IsUseVirtualList())
		return GetVirtualItemCount();

    return m_pList->GetCount();
}

int CListUI::GetItemCount() const
{
	return m_pList->GetCount();
}

bool CListUI::Add(CControlUI* pControl)
{
	// no support multiple list headers.
	if (pControl && pControl->GetInterface(DUI_CTR_LISTHEADER) != NULL) {
        if( m_pHeader != pControl /*&& m_pHeader->GetCount() == 0 */) {
            CVerticalLayoutUI::Remove(m_pHeader);
            m_pHeader = static_cast<CListHeaderUI*>(pControl);
        }
		//计算复合表头实际表头项的数量
		CDuiPtrArray ptrAry;
		GetInsideControl(ptrAry, m_pHeader, DUI_CTR_LISTHEADERITEM);
        //m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
		m_ListInfo.nColumns = MIN(ptrAry.GetSize(), UILIST_MAX_COLUMNS);
        return CVerticalLayoutUI::AddAt(pControl, 0);
    }
    // We also need to recognize header sub-items
	if (pControl && _tcsstr(pControl->GetClass(), DUI_CTR_LISTHEADERITEM) != NULL) {
        bool ret = m_pHeader->Add(pControl);
		//计算复合表头实际表头项的数量
		CDuiPtrArray ptrAry;
		GetInsideControl(ptrAry, m_pHeader, DUI_CTR_LISTHEADERITEM);
		//m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
		m_ListInfo.nColumns = MIN(ptrAry.GetSize(), UILIST_MAX_COLUMNS);
        return ret;
    }
	//如果是虚拟列表则不允许操作,表头除外
	if (IsUseVirtualList())
	{
		return false;
	}

	if (!m_ItemtemplateXml.IsEmpty())
	{
		CDialogBuilder builder;
		pControl = static_cast<CControlUI*>(builder.Create(m_ItemtemplateXml.GetData(), (UINT)0, NULL, m_pManager, NULL));
	}

	if (!pControl)
	{
		return false;
	}
	// The list items should know about us
	IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
	if (pListItem != NULL) {
		pListItem->SetOwner(this);
		pListItem->SetIndex(GetItemCount());
	}

	bool bret = m_pList->Add(pControl);

	if (m_ItemtemplateXml.IsEmpty())
	{
		return bret;
	}
	else
	{
		if (!bret)
		{
			pControl->Delete();
		}
		return false;
	}
}

bool CListUI::AddAt(CControlUI* pControl, int iIndex)
{
	// no support multiple list headers.
	if (pControl&& pControl->GetInterface(DUI_CTR_LISTHEADER) != NULL) {
        if( m_pHeader != pControl/* && m_pHeader->GetCount() == 0*/ ) {
            CVerticalLayoutUI::Remove(m_pHeader);
            m_pHeader = static_cast<CListHeaderUI*>(pControl);
        }
		//计算复合表头实际表头项的数量
		CDuiPtrArray ptrAry;
		GetInsideControl(ptrAry, m_pHeader, DUI_CTR_LISTHEADERITEM);
		//m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
		m_ListInfo.nColumns = MIN(ptrAry.GetSize(), UILIST_MAX_COLUMNS);
        return CVerticalLayoutUI::AddAt(pControl, 0);
    }
    // We also need to recognize header sub-items
	if (pControl&&_tcsstr(pControl->GetClass(), DUI_CTR_LISTHEADERITEM) != NULL) {
        bool ret = m_pHeader->AddAt(pControl, iIndex);
		//计算复合表头实际表头项的数量
		CDuiPtrArray ptrAry;
		GetInsideControl(ptrAry, m_pHeader, DUI_CTR_LISTHEADERITEM);
		//m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
		m_ListInfo.nColumns = MIN(ptrAry.GetSize(), UILIST_MAX_COLUMNS);
        return ret;
    }

	//如果是虚拟列表则不允许操作,表头除外
	if (IsUseVirtualList())
	{
		return false;
	}

	if (!m_ItemtemplateXml.IsEmpty())
	{
		CDialogBuilder builder;
		pControl = static_cast<CControlUI*>(builder.Create(m_ItemtemplateXml.GetData(), (UINT)0, NULL, m_pManager, NULL));
	}

	if (!pControl)
	{
		return false;
	}

	if (!m_pList->AddAt(pControl, iIndex))
	{
		if (!m_ItemtemplateXml.IsEmpty())
		{
			pControl->Delete();
		}
		return false;
	}

	// The list items should know about us
	IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
	if (pListItem != NULL) {
		pListItem->SetOwner(this);
		pListItem->SetIndex(iIndex);
	}

	for (int i = iIndex + 1; i < m_pList->GetCount(); ++i) {
		CControlUI* p = m_pList->GetItemAt(i);
		pListItem = static_cast<IListItemUI*>(p->GetInterface(DUI_CTR_ILISTITEM));
		if (pListItem != NULL) {
			pListItem->SetIndex(i);
		}
	}
	if (m_iCurSel >= iIndex) m_iCurSel += 1;
   
	return m_ItemtemplateXml.IsEmpty();
}

CControlUI* CListUI::AddTemplate()
{
	CControlUI *pControl = NULL;
	//如果是虚拟列表则不允许操作,表头除外
	if (IsUseVirtualList() || m_ItemtemplateXml.IsEmpty())
	{
		return pControl;
	}

	CDialogBuilder builder;
	pControl = static_cast<CControlUI*>(builder.Create(m_ItemtemplateXml.GetData(), (UINT)0, NULL, m_pManager, NULL));

	if (!pControl)
	{
		return pControl;
	}
	// The list items should know about us
	IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
	if (pListItem != NULL) {
		pListItem->SetOwner(this);
		pListItem->SetIndex(GetItemCount());
	}

	if (!m_pList->Add(pControl))
	{
		pControl->Delete();
	}
	return pControl;
}

CControlUI* CListUI::AddTemplateAt(int iIndex)
{
	CControlUI *pControl = NULL;
	//如果是虚拟列表则不允许操作,表头除外
	if (IsUseVirtualList() || m_ItemtemplateXml.IsEmpty())
	{
		return pControl;
	}

	CDialogBuilder builder;
	pControl = static_cast<CControlUI*>(builder.Create(m_ItemtemplateXml.GetData(), (UINT)0, NULL, m_pManager, NULL));

	if (!pControl)
	{
		return pControl;
	}

	if (!m_pList->AddAt(pControl, iIndex))
	{
		if (!m_ItemtemplateXml.IsEmpty())
		{
			pControl->Delete();
		}
		return pControl;
	}

	// The list items should know about us
	IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
	if (pListItem != NULL) {
		pListItem->SetOwner(this);
		pListItem->SetIndex(iIndex);
	}

	for (int i = iIndex + 1; i < m_pList->GetCount(); ++i) {
		CControlUI* p = m_pList->GetItemAt(i);
		pListItem = static_cast<IListItemUI*>(p->GetInterface(DUI_CTR_ILISTITEM));
		if (pListItem != NULL) {
			pListItem->SetIndex(i);
		}
	}
	if (m_iCurSel >= iIndex) m_iCurSel += 1;

	return pControl;
}

bool CListUI::AddVirtualItem(CControlUI* pControl)
{
	// no support multiple list headers.
	if (pControl->GetInterface(DUI_CTR_LISTHEADER) != NULL) {
		if (m_pHeader != pControl /*&& m_pHeader->GetCount() == 0*/) {
			CVerticalLayoutUI::Remove(m_pHeader);
			m_pHeader = static_cast<CListHeaderUI*>(pControl);
		}
		//计算复合表头实际表头项的数量
		CDuiPtrArray ptrAry;
		GetInsideControl(ptrAry, m_pHeader, DUI_CTR_LISTHEADERITEM);
		//m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
		m_ListInfo.nColumns = MIN(ptrAry.GetSize(), UILIST_MAX_COLUMNS);
		return CVerticalLayoutUI::AddAt(pControl, 0);
	}
	// We also need to recognize header sub-items
	if (_tcsstr(pControl->GetClass(), DUI_CTR_LISTHEADERITEM) != NULL) {
		bool ret = m_pHeader->Add(pControl);
		//计算复合表头实际表头项的数量
		CDuiPtrArray ptrAry;
		GetInsideControl(ptrAry, m_pHeader, DUI_CTR_LISTHEADERITEM);
		//m_ListInfo.nColumns = MIN(m_pHeader->GetCount(), UILIST_MAX_COLUMNS);
		m_ListInfo.nColumns = MIN(ptrAry.GetSize(), UILIST_MAX_COLUMNS);
		return ret;
	}

	// The list items should know about us
	IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
	if (pListItem != NULL) {
		pListItem->SetOwner(this);
		pListItem->SetIndex(GetItemCount());
	}
	return m_pList->Add(pControl);
}
bool CListUI::Remove(CControlUI* pControl, bool bDoNotDestroy)
{
	//如果是虚拟列表则不允许操作
	if (IsUseVirtualList())
	{
		return false;
	}

    if( pControl->GetInterface(DUI_CTR_LISTHEADER) != NULL ) return CVerticalLayoutUI::Remove(pControl, bDoNotDestroy);
    // We also need to recognize header sub-items
    if( _tcsstr(pControl->GetClass(), DUI_CTR_LISTHEADERITEM) != NULL ) return m_pHeader->Remove(pControl, bDoNotDestroy);

    int iIndex = m_pList->GetItemIndex(pControl);
    if (iIndex == -1) return false;

    if (!m_pList->RemoveAt(iIndex, bDoNotDestroy)) return false;

    for(int i = iIndex; i < m_pList->GetCount(); ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(DUI_CTR_ILISTITEM));
        if( pListItem != NULL ) {
            pListItem->SetIndex(i);
        }
    }

    if( iIndex == m_iCurSel && m_iCurSel >= 0 ) {
        int iSel = m_iCurSel;
        m_iCurSel = -1;
        SelectItem(FindSelectable(iSel, false));
    }
    else if( iIndex < m_iCurSel ) m_iCurSel -= 1;
    return true;
}

bool CListUI::RemoveAt(int iIndex, bool bDoNotDestroy)
{
	//如果是虚拟列表则不允许操作
	if (IsUseVirtualList())
	{
		return false;
	}

    if (!m_pList->RemoveAt(iIndex, bDoNotDestroy)) return false;

    for(int i = iIndex; i < m_pList->GetCount(); ++i) {
        CControlUI* p = m_pList->GetItemAt(i);
        IListItemUI* pListItem = static_cast<IListItemUI*>(p->GetInterface(DUI_CTR_ILISTITEM));
        if( pListItem != NULL ) pListItem->SetIndex(i);
    }

    if( iIndex == m_iCurSel && m_iCurSel >= 0 ) {
        int iSel = m_iCurSel;
        m_iCurSel = -1;
        SelectItem(FindSelectable(iSel, false));
    }
    else if( iIndex < m_iCurSel ) m_iCurSel -= 1;
    return true;
}

void CListUI::RemoveAll()
{
	//如果是虚拟列表则不允许操作
	if (IsUseVirtualList())
	{
		return ;
	}

    m_iCurSel = -1;
    m_iExpandedItem = -1;
    m_pList->RemoveAll();
}

void CListUI::ResetSortStatus()
{
	if (m_pHeader && m_pHeader->GetCount())
	{
		CListHeaderItemUI *pHeaderItem = static_cast<CListHeaderItemUI*>(m_pHeader->GetItemAt(0));
		pHeaderItem->SetSort(E_SORTNO);
	}
}

void CListUI::SetPos(RECT rc, bool bNeedInvalidate)
{
	//位置发生改变的时候当前滚动条位置iOffsetNow
	//位置发生改变之后动条重新计算位置iResizeOffset
	int iOffsetNow = 0, iResizeOffset = 0;
	//计算复合表头实际表头项的数量
	CDuiPtrArray ptrAry;
	GetInsideControl(ptrAry, m_pHeader, DUI_CTR_LISTHEADERITEM);
	m_ListInfo.nColumns = MIN(ptrAry.GetSize(), UILIST_MAX_COLUMNS);

	if( m_pHeader != NULL ) 
	{ // 设置header各子元素x坐标,因为有些listitem的setpos需要用到(临时修复)
		int iLeft = rc.left + m_rcInset.left;
		int iRight = rc.right - m_rcInset.right;
		if( !m_pHeader->IsVisible() ) {
			for( int it = m_pHeader->GetCount() - 1; it >= 0; it-- ) {
				static_cast<CControlUI*>(m_pHeader->GetItemAt(it))->SetInternVisible(true);
			}
		}
		m_pHeader->SetPos(CDuiRect(iLeft, 0, iRight, 0), false);
		iOffsetNow = m_pList->GetScrollPos().cx;

		for( int i = 0; i < m_ListInfo.nColumns; i++ )
		{
			CControlUI* pControl = static_cast<CControlUI*>(ptrAry[i]);

			m_ListInfo.bUsedHeaderContain[i] = pControl->IsIncludeClassControl(DUI_CTR_LISTHEADERITEM);

			if (!pControl->IsVisible())
			{
				//#liulei 修复由于隐藏最后一列的时候，由于宽度 = m_ListInfo.rcColumn[lastcol].right - m_ListInfo.rcColumn[i].left
				//所以这里需要修复列宽的计算方法 20160616
				i == 0 ? m_ListInfo.rcColumn[i].left = m_ListInfo.rcColumn[i].right = 0 :
					m_ListInfo.rcColumn[i].right = m_ListInfo.rcColumn[i].left = m_ListInfo.rcColumn[i - 1].right;
				continue;
			}
			if (pControl->IsFloat())
			{
				//#liulei 修复由于隐藏最后一列的时候，由于宽度 = m_ListInfo.rcColumn[lastcol].right - m_ListInfo.rcColumn[i].left
				//所以这里需要修复列宽的计算方法 20160616
				i == 0 ? m_ListInfo.rcColumn[i].left = m_ListInfo.rcColumn[i].right = 0 :
					m_ListInfo.rcColumn[i].right = m_ListInfo.rcColumn[i].left = m_ListInfo.rcColumn[i - 1].right;
				continue;
			}

			RECT rcPos = pControl->GetPos();
			if (iOffsetNow > 0 && pControl->GetParent() == m_pHeader) {
				rcPos.left -= iOffsetNow;
				rcPos.right -= iOffsetNow;
				pControl->SetPos(rcPos, false);
			}
			m_ListInfo.rcColumn[i] = pControl->GetPos();
		}

		if( !m_pHeader->IsVisible() ) 
		{
			for( int it = m_pHeader->GetCount() - 1; it >= 0; it-- ) 
			{
				static_cast<CControlUI*>(m_pHeader->GetItemAt(it))->SetInternVisible(false);
			}
			m_pHeader->SetInternVisible(false);
		}
	}

	CVerticalLayoutUI::SetPos(rc, bNeedInvalidate);
	///> 重新计算虚表缓冲区大小
	ResizeVirtualItemBuffer();
	if( m_pHeader == NULL ) return;

	rc = m_rcItem;
	rc.left += m_rcInset.left;
	rc.top += m_rcInset.top;
	rc.right -= m_rcInset.right;
	rc.bottom -= m_rcInset.bottom;

	if( m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) {
		rc.top -= m_pVerticalScrollBar->GetScrollPos();
		rc.bottom -= m_pVerticalScrollBar->GetScrollPos();
		rc.bottom += m_pVerticalScrollBar->GetScrollRange();
		rc.right -= m_pVerticalScrollBar->GetFixedWidth();
	}
	if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) {
		rc.left -= m_pHorizontalScrollBar->GetScrollPos();
		rc.right -= m_pHorizontalScrollBar->GetScrollPos();
		rc.right += m_pHorizontalScrollBar->GetScrollRange();
		rc.bottom -= m_pHorizontalScrollBar->GetFixedHeight();
	}

	if (m_pHeader && !m_pHeader->IsVisible()) {
		for( int it = m_pHeader->GetCount() - 1; it >= 0; it-- ) {
			static_cast<CControlUI*>(m_pHeader->GetItemAt(it))->SetInternVisible(true);
		}
		m_pHeader->SetPos(CDuiRect(rc.left, 0, rc.right, 0), false);
	}
	iResizeOffset = m_pList->GetScrollPos().cx;

	///> 矫正表头项的位置，因为表头是Contain并且不是float，表头SetPos之后会对Item重现排序，如果此时横向滚动条不在位置为0的地方
	///> 则可能出现表头项由于没有偏移而导致表头显示不对，list的内容不受影响，重新调整listHeader包含项的位置
	for (int i = 0; i < m_pHeader->GetCount(); i++) {
		CControlUI* pControl = static_cast<CControlUI*>(m_pHeader->GetItemAt(i));
		if (!pControl->IsVisible()) continue;
		if (pControl->IsFloat()) continue;

		RECT rcPos = pControl->GetPos();
		if (iResizeOffset > 0) {
			rcPos.left -= iResizeOffset;
			rcPos.right -= iResizeOffset;
			pControl->SetPos(rcPos, false);
		}
	}

	///> 复合容器的原因，在这里调整list表头项的对应的区域位置
	for( int i = 0; i < m_ListInfo.nColumns; i++ ) {
		CControlUI* pControl = static_cast<CControlUI*>(ptrAry[i]);
		if( !pControl->IsVisible() ) continue;
		if( pControl->IsFloat() ) continue;
		m_ListInfo.rcColumn[i] = pControl->GetPos();
	}

	if( !m_pHeader->IsVisible() ) {
		for( int it = m_pHeader->GetCount() - 1; it >= 0; it-- ) {
			static_cast<CControlUI*>(m_pHeader->GetItemAt(it))->SetInternVisible(false);
		}
		m_pHeader->SetInternVisible(false);
	}


	//#liulei 如果位置发生改变之前和之后滚动条前后位置不一样则需要刷新ListBody重新计算位置,
	if (iOffsetNow != iResizeOffset)
		CVerticalLayoutUI::SetPos(rc, bNeedInvalidate);

	///#liulei 20160823
	///> 矫正表头项的位置，因为表头是Contain并且不是float，表头SetPos之后会对Item重现排序，如果此时横向滚动条不在位置为0的地方
	///> 则可能出现表头项由于没有偏移而导致表头显示不对，list的内容不受影响
	for (int i = 0; i < m_ListInfo.nColumns; i++) {
		CControlUI* pControl = static_cast<CControlUI*>(ptrAry[i]);
		if (!pControl->IsVisible()) continue;
		if (pControl->IsFloat()) continue;
		pControl->SetPos(m_ListInfo.rcColumn[i], false);
	}

	///#liulei 20161109
	///#计算浮动Panel的位置
	CalcPanelPos();
}

void CListUI::Move(SIZE szOffset, bool bNeedInvalidate)
{
	CVerticalLayoutUI::Move(szOffset, bNeedInvalidate);
	//if( !m_pHeader->IsVisible() ) m_pHeader->Move(szOffset, false);
}

void CListUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pParent != NULL ) m_pParent->DoEvent(event);
        else CVerticalLayoutUI::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_SETFOCUS ) 
    {
        m_bFocused = true;
        return;
    }
    if( event.Type == UIEVENT_KILLFOCUS ) 
    {
        m_bFocused = false;
        return;
    }
	//#liulei 20161115 控制鼠标是否响应鼠标滚动
	if (event.Type == UIEVENT_BUTTONDOWN ||
		event.Type == UIEVENT_RBUTTONDOWN ||
		event.Type == UIEVENT_DBLCLICK)
	{
		if (!m_bEnableMouseWhell && ::PtInRect(&m_rcItem, event.ptMouse))
			m_bEnableMouseWhell = true;
	}
    if (event.Type == UIEVENT_MOUSEENTER)
    {
        m_bEnableMouseWhell = true;
    }

	if (event.Type == UIEVENT_MOUSELEAVE)
	{
		if (m_bEnableMouseWhell && !::PtInRect(&m_rcItem, event.ptMouse))
			m_bEnableMouseWhell = false;
	}
    if( event.Type == UIEVENT_KEYDOWN )
    {
        if (IsKeyboardEnabled() && IsEnabled()) {
            switch( event.chKey ) {
			case VK_UP:
				{
					if (!IsUseVirtualList())
						SelectItem(FindSelectable(m_iCurSel - 1, false), true);
					else
						LineUp();
				}
				break;
            case VK_DOWN:
				{
					if (!IsUseVirtualList())
						SelectItem(FindSelectable(m_iCurSel + 1, true), true);
					else
						LineDown();
				}
				break;
            case VK_PRIOR:
				PageUp(); break;
            case VK_NEXT:
				PageDown(); break;
            case VK_HOME:
			   {
				   if (!IsUseVirtualList())
					   SelectItem(FindSelectable(0, false), true);
				   else
					   HomeUp();
			   }
			   break;
            case VK_END:
				{
					if (!IsUseVirtualList())
						SelectItem(FindSelectable(GetItemCount() - 1, true), true);
					else
						EndDown();
				}
				break;
            case VK_RETURN:
				if (m_iCurSel != -1) GetItemAt(m_iCurSel)->Activate(); break;
            }
            return;
        }
    }

	if (event.Type == UIEVENT_SCROLLWHEEL && m_bEnableMouseWhell)
    {
        if (IsEnabled()) {
            switch( LOWORD(event.wParam) ) {
            case SB_LINEUP:
                if( m_bScrollSelect && !IsUseVirtualList()) SelectItem(FindSelectable(m_iCurSel - 1, false), true);
                else LineUp();
                return;
            case SB_LINEDOWN:
				if (m_bScrollSelect && !IsUseVirtualList() ) SelectItem(FindSelectable(m_iCurSel + 1, true), true);
                else LineDown();
                return;
            }
        }
    }

	CVerticalLayoutUI::DoEvent(event);
}

CListHeaderUI* CListUI::GetHeader() const
{
    return m_pHeader;
}

CContainerUI* CListUI::GetList() const
{
    return m_pList;
}

void CListUI::SetItemTemplateXml(CDuiString xml)
{
	if (m_ItemtemplateXml != xml)
	{
		m_ItemtemplateXml = xml;
		if (!m_ItemtemplateXml.IsEmpty() && m_pList)
		{
			m_pList->RemoveAll();
		}
	}
}

bool CListUI::GetScrollSelect()
{
    return m_bScrollSelect;
}

void CListUI::SetScrollSelect(bool bScrollSelect)
{
    m_bScrollSelect = bScrollSelect;
}

int CListUI::GetCurSel() const
{
    return m_iCurSel;
}

void CListUI::SetVirtualItemFormat(IListVirtalCallbackUI* vrtualitemfroamt)
{
	if (!m_pList) return;
	m_pVirutalItemFormat = vrtualitemfroamt;
	///> 检测是否该关闭虚表优化（是否含有combo）
	CControlUI *pcontrol = m_pVirutalItemFormat ? vrtualitemfroamt->CreateVirtualItem():CreateTemplateControl();
	if (pcontrol && pcontrol->GetInterface(DUI_CTR_CONTAINER))
	{
		CContainerUI *pContain = static_cast<CContainerUI *>(pcontrol->GetInterface(DUI_CTR_CONTAINER));
		if (pContain && (pContain->IsIncludeClassControl(DUI_CTR_COMBO) || pContain->IsIncludeClassControl(DUI_CTR_COMBOBOX)))
			EnableVirtualOptimize(false);
	}
	///> 释放资源
	if (pcontrol) 
	{
		pcontrol->Delete();
	}
	ResizeVirtualItemBuffer();//调整缓冲区大小
}

void CListUI::SetVirtual(bool bUse)
{
	m_bUseVirtualList = bUse;
}

bool CListUI::IsUseVirtualList() const
{
	return m_bUseVirtualList;
}

int CListUI::GetVirtualItemHeight()
{
	return m_nVirtualItemHeight;
}

int CListUI::GetVirtualItemCount() const
{
	return m_nVirtualItemCount;
}

int CListUI::GetShowMaxItemCount() const
{
	return m_nMaxShowCount;
}

int CListUI::GetDrawStartIndex() const
{
	return m_nDrawStartIndex;
}

int CListUI::GetDrawLastIndex() const
{
	int nLastDrawIndex = min(m_nDrawStartIndex + m_nMaxShowCount, m_nVirtualItemCount);
	//下标从0开始所以需要减去1
	nLastDrawIndex -= 1;
	return nLastDrawIndex;
}

void CListUI::SetSelectControlTag(INT64 iControlTag)
{
	m_iSelectControlTag = iControlTag;
}

INT64 CListUI::GetSelectControlTag()
{
	return m_iSelectControlTag;
}

void CListUI::EnableVirtualOptimize(bool bEnableVirtualO)
{
	m_bEnableVirtualO = bEnableVirtualO;
}

void CListUI::SetSort(int nIndex, ESORT esort, bool bTriggerEvent)
{
	if (m_pHeader)
		m_pHeader->SetSort(nIndex, esort, bTriggerEvent);
}

CListHeaderItemUI*	 CListUI::GetSortHeaderItem()
{
	if (m_pHeader)
		return m_pHeader->GetSortHeaderItem();
	return NULL;
}

void CListUI::ResizeVirtualItemBuffer()
{
	if (!IsUseVirtualList()) return;

 	//if (m_pVirutalItemFormat)
	{
		if (GetItemCount() == 0)
		{
			///> 准备资源
			CControlUI *pControl = m_pVirutalItemFormat ? m_pVirutalItemFormat->CreateVirtualItem() : CreateTemplateControl();
			if (pControl)
			{
				m_nVirtualItemHeight = max(pControl->GetFixedHeight(), pControl->GetHeight());
				m_nVirtualItemHeight <= 0 ? m_nVirtualItemHeight = VIR_ITEM_HEIGHT : 0;
				///> 释放资源
				pControl->Delete();
			}
		}

		int nItemCount = m_pList->GetCount();
		///> 采取进1原则
		int nReverse = (m_pList->GetHeight() % m_nVirtualItemHeight != 0) ? 1:0;
		//if ( m_pList->GetHeight() % m_nVirtualItemHeight > m_nVirtualItemHeight*0.8)
		//	nReverse = 1;

		int nItemSize = m_pList->GetHeight()/ m_nVirtualItemHeight + nReverse;
		m_nMaxShowCount = nItemSize;

		for (int i = nItemCount; i < nItemSize; ++i)
		{
			CControlUI *pControl = m_pVirutalItemFormat ? m_pVirutalItemFormat->CreateVirtualItem() : CreateTemplateControl();
			if (pControl)
			{
				AddVirtualItem(pControl);
			}
		}
	}
}

void CListUI::SetVirtualItemCount(int nCountItem)
{
	m_nVirtualItemCount = nCountItem;
	NeedUpdate();
}

void CListUI::SetPanelHeight(int nHeight)
{
	if (m_nPanelHeight == nHeight) return;
	m_nPanelHeight = nHeight;
	m_pFloatPanel->SetFixedHeight(m_nPanelHeight);
	CalcPanelPos();
}

void CListUI::SetPanelOffset(int nPanelOffset)
{
	if (m_nPanelOffset == nPanelOffset)return;
	m_nPanelOffset = nPanelOffset;
	CalcPanelPos();
}

void CListUI::SetPanelPos(EPANELPOS ePanelPos)
{
	if (m_ePanelPos == ePanelPos) return;
	m_ePanelPos = ePanelPos;
	CalcPanelPos();
}

void CListUI::SetPanelXml(LPCTSTR szXml)
{
	m_pFloatPanel->SetChildLayoutXML(szXml);
}

void CListUI::SetPanelAttributeList(LPCTSTR pstrList)
{
	m_pFloatPanel->SetAttributeList(pstrList);
}

void CListUI::SetPanelVisible(bool bVisible)
{
	m_pFloatPanel->SetVisible(bVisible);
}

CChildLayoutUI *CListUI::GetFloatPanel()
{
	return m_pFloatPanel;
}

bool CListUI::IsEnableMouseWhell()
{
	return m_bEnableMouseWhell;
}

CControlUI *CListUI::CreateTemplateControl()
{
	ASSERT(!m_ItemtemplateXml.IsEmpty());
	CDialogBuilder builder;
	CControlUI* pChildControl = static_cast<CControlUI*>(builder.Create(m_ItemtemplateXml.GetData(), (UINT)0, NULL, m_pManager, NULL));
	return pChildControl; 
}

void CListUI::CalcPanelPos()
{
	RECT rtPanel = m_rcItem;
	int nwOffset = 0;
	if (m_pList && m_pList->GetVerticalScrollBar() && m_pList->GetVerticalScrollBar()->IsVisible())
	{
		nwOffset = m_pList->GetVerticalScrollBar()->GetFixedWidth();
		rtPanel.right -= nwOffset;
	}

	if (m_ePanelPos == E_PANELTOP)
	{
		int nHeightOffset = 0;
		if (m_pHeader->IsVisible())
			nHeightOffset = m_pHeader->GetHeight();
		///> 相对容器的绝对位置
		::OffsetRect(&rtPanel, -m_rcItem.left, -m_rcItem.top + nHeightOffset);
		rtPanel.top += m_nPanelOffset;
		rtPanel.bottom = rtPanel.top + m_nPanelHeight;
	}
	else if (m_ePanelPos == E_PANELBOTTOM)
	{
		::OffsetRect(&rtPanel, -m_rcItem.left, -m_rcItem.top);
		rtPanel.bottom -= m_nPanelOffset;
		rtPanel.top = rtPanel.bottom - m_nPanelHeight;
	}
	m_pFloatPanel->SetPos(rtPanel);
}

bool CListUI::SelectItem(int iIndex, bool bTakeFocus, bool bTriggerEvent)
{
	if( iIndex == m_iCurSel ) return true;

    int iOldSel = m_iCurSel;
    // We should first unselect the currently selected item
    if( m_iCurSel >= 0 ) {
        CControlUI* pControl = GetItemAt(m_iCurSel);
        if( pControl != NULL) {
			if (IsUseVirtualList() && bTriggerEvent && pControl->GetTag() != 0)
			{
				SetSelectControlTag(pControl->GetTag());
			}
            IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
            if( pListItem != NULL ) pListItem->Select(false, bTriggerEvent);
        }

        m_iCurSel = -1;
    }
    if( iIndex < 0 ) return false;

    CControlUI* pControl = GetItemAt(iIndex);
    if( pControl == NULL ) return false;
    if( !pControl->IsVisible() ) return false;
    if( !pControl->IsEnabled() ) return false;

    IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
    if( pListItem == NULL ) return false;
    m_iCurSel = iIndex;
    if( !pListItem->Select(true, bTriggerEvent) ) {
        m_iCurSel = -1;
        return false;
    }
	//#liulei 20160901 不保证选中的Item始终在窗口
   // EnsureVisible(m_iCurSel);
	if (IsUseVirtualList() && pControl->GetTag() != 0)
	{
		SetSelectControlTag(pControl->GetTag());
	}
    if( bTakeFocus ) pControl->SetFocus();
    if( m_pManager != NULL && bTriggerEvent ) {
        m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMSELECT, m_iCurSel, iOldSel);
    }

    return true;
}

TListInfoUI* CListUI::GetListInfo()
{
    return &m_ListInfo;
}

int CListUI::GetChildPadding() const
{
    return m_pList->GetChildPadding();
}

void CListUI::SetChildPadding(int iPadding)
{
    m_pList->SetChildPadding(iPadding);
}

UINT CListUI::GetItemFixedHeight()
{
    return m_ListInfo.uFixedHeight;
}

void CListUI::SetItemFixedHeight(UINT nHeight)
{
    m_ListInfo.uFixedHeight = nHeight;
    NeedUpdate();
}

int CListUI::GetItemFont(int index)
{
    return m_ListInfo.nFont;
}

void CListUI::SetItemFont(int index)
{
    m_ListInfo.nFont = index;
    NeedUpdate();
}

UINT CListUI::GetItemTextStyle()
{
    return m_ListInfo.uTextStyle;
}

void CListUI::SetItemTextStyle(UINT uStyle)
{
    m_ListInfo.uTextStyle = uStyle;
    NeedUpdate();
}

void CListUI::SetItemTextPadding(RECT rc)
{
    m_ListInfo.rcTextPadding = rc;
    NeedUpdate();
}

RECT CListUI::GetItemTextPadding() const
{
	return m_ListInfo.rcTextPadding;
}

void CListUI::SetItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetItemBkImage(LPCTSTR pStrImage)
{
	if( m_ListInfo.diBk.sDrawString == pStrImage && m_ListInfo.diBk.pImageInfo != NULL ) return;
	m_ListInfo.diBk.Clear();
	m_ListInfo.diBk.sDrawString = pStrImage;
	Invalidate();
}

bool CListUI::IsAlternateBk() const
{
	return m_ListInfo.bAlternateBk;
}

void CListUI::SetAlternateBk(bool bAlternateBk)
{
    m_ListInfo.bAlternateBk = bAlternateBk;
    Invalidate();
}

DWORD CListUI::GetItemTextColor() const
{
	return m_ListInfo.dwTextColor;
}

DWORD CListUI::GetItemBkColor() const
{
	return m_ListInfo.dwBkColor;
}

LPCTSTR CListUI::GetItemBkImage() const
{
    return m_ListInfo.diBk.sDrawString;
}

void CListUI::SetSelectedItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwSelectedTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetSelectedItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwSelectedBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetSelectedItemImage(LPCTSTR pStrImage)
{
	if( m_ListInfo.diSelected.sDrawString == pStrImage && m_ListInfo.diSelected.pImageInfo != NULL ) return;
	m_ListInfo.diSelected.Clear();
	m_ListInfo.diSelected.sDrawString = pStrImage;
	Invalidate();
}

DWORD CListUI::GetSelectedItemTextColor() const
{
	return m_ListInfo.dwSelectedTextColor;
}

DWORD CListUI::GetSelectedItemBkColor() const
{
	return m_ListInfo.dwSelectedBkColor;
}

LPCTSTR CListUI::GetSelectedItemImage() const
{
	return m_ListInfo.diSelected.sDrawString;
}

void CListUI::SetHotItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwHotTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetHotItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwHotBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetHotItemImage(LPCTSTR pStrImage)
{
	if( m_ListInfo.diHot.sDrawString == pStrImage && m_ListInfo.diHot.pImageInfo != NULL ) return;
	m_ListInfo.diHot.Clear();
	m_ListInfo.diHot.sDrawString = pStrImage;
	Invalidate();
}

DWORD CListUI::GetHotItemTextColor() const
{
	return m_ListInfo.dwHotTextColor;
}
DWORD CListUI::GetHotItemBkColor() const
{
	return m_ListInfo.dwHotBkColor;
}

LPCTSTR CListUI::GetHotItemImage() const
{
	return m_ListInfo.diHot.sDrawString;
}

void CListUI::SetDisabledItemTextColor(DWORD dwTextColor)
{
    m_ListInfo.dwDisabledTextColor = dwTextColor;
    Invalidate();
}

void CListUI::SetDisabledItemBkColor(DWORD dwBkColor)
{
    m_ListInfo.dwDisabledBkColor = dwBkColor;
    Invalidate();
}

void CListUI::SetDisabledItemImage(LPCTSTR pStrImage)
{
	if( m_ListInfo.diDisabled.sDrawString == pStrImage && m_ListInfo.diDisabled.pImageInfo != NULL ) return;
	m_ListInfo.diDisabled.Clear();
	m_ListInfo.diDisabled.sDrawString = pStrImage;
	Invalidate();
}

DWORD CListUI::GetDisabledItemTextColor() const
{
	return m_ListInfo.dwDisabledTextColor;
}

DWORD CListUI::GetDisabledItemBkColor() const
{
	return m_ListInfo.dwDisabledBkColor;
}

LPCTSTR CListUI::GetDisabledItemImage() const
{
	return m_ListInfo.diDisabled.sDrawString;
}

int CListUI::GetItemHLineSize() const
{
    return m_ListInfo.iHLineSize;
}

void CListUI::SetItemHLineSize(int iSize)
{
    m_ListInfo.iHLineSize = iSize;
    Invalidate();
}

DWORD CListUI::GetItemHLineColor() const
{
    return m_ListInfo.dwHLineColor;
}

void CListUI::SetItemHLineColor(DWORD dwLineColor)
{
    m_ListInfo.dwHLineColor = dwLineColor;
    Invalidate();
}

int CListUI::GetItemVLineSize() const
{
    return m_ListInfo.iVLineSize;
}

void CListUI::SetItemVLineSize(int iSize)
{
    m_ListInfo.iVLineSize = iSize;
    Invalidate();
}

DWORD CListUI::GetItemVLineColor() const
{
    return m_ListInfo.dwVLineColor;
}

void CListUI::SetItemVLineColor(DWORD dwLineColor)
{
    m_ListInfo.dwVLineColor = dwLineColor;
    Invalidate();
}

bool CListUI::IsItemShowHtml()
{
    return m_ListInfo.bShowHtml;
}

void CListUI::SetItemShowHtml(bool bShowHtml)
{
    if( m_ListInfo.bShowHtml == bShowHtml ) return;

    m_ListInfo.bShowHtml = bShowHtml;
    NeedUpdate();
}

void CListUI::SetMultiExpanding(bool bMultiExpandable)
{
    m_ListInfo.bMultiExpandable = bMultiExpandable;
}

bool CListUI::ExpandItem(int iIndex, bool bExpand /*= true*/)
{
    if( m_iExpandedItem >= 0 && !m_ListInfo.bMultiExpandable) {
        CControlUI* pControl = GetItemAt(m_iExpandedItem);
        if( pControl != NULL ) {
            IListItemUI* pItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
            if( pItem != NULL ) pItem->Expand(false);
        }
        m_iExpandedItem = -1;
    }
    if( bExpand ) {
        CControlUI* pControl = GetItemAt(iIndex);
        if( pControl == NULL ) return false;
        if( !pControl->IsVisible() ) return false;
        IListItemUI* pItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
        if( pItem == NULL ) return false;
        m_iExpandedItem = iIndex;
        if( !pItem->Expand(true) ) {
            m_iExpandedItem = -1;
            return false;
        }
    }
    NeedUpdate();
    return true;
}

int CListUI::GetExpandedItem() const
{
    return m_iExpandedItem;
}

void CListUI::EnsureVisible(int iIndex)
{
    if( m_iCurSel < 0 ) return;
    RECT rcItem = m_pList->GetItemAt(iIndex)->GetPos();
    RECT rcList = m_pList->GetPos();
    RECT rcListInset = m_pList->GetInset();

    rcList.left += rcListInset.left;
    rcList.top += rcListInset.top;
    rcList.right -= rcListInset.right;
    rcList.bottom -= rcListInset.bottom;

    CScrollBarUI* pHorizontalScrollBar = m_pList->GetHorizontalScrollBar();
    if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rcList.bottom -= pHorizontalScrollBar->GetFixedHeight();

    int iPos = m_pList->GetScrollPos().cy;
    if( rcItem.top >= rcList.top && rcItem.bottom < rcList.bottom ) return;
    int dx = 0;
    if( rcItem.top < rcList.top ) dx = rcItem.top - rcList.top;
    if( rcItem.bottom > rcList.bottom ) dx = rcItem.bottom - rcList.bottom;
    Scroll(0, dx);
}

void CListUI::Scroll(int dx, int dy)
{
    if( dx == 0 && dy == 0 ) return;
    SIZE sz = m_pList->GetScrollPos();
    m_pList->SetScrollPos(CDuiSize(sz.cx + dx, sz.cy + dy));
}

void CListUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if( _tcscmp(pstrName, _T("header")) == 0 ) GetHeader()->SetVisible(_tcscmp(pstrValue, _T("hidden")) != 0);
	else if (_tcscmp(pstrName, _T("virtual")) == 0) SetVirtual(_tcscmp(pstrValue, _T("true")) == 0);
	else if (_tcscmp(pstrName, _T("virtualo")) == 0) EnableVirtualOptimize(_tcscmp(pstrValue, _T("true")) == 0);
	else if (_tcscmp(pstrName, _T("panelvisible")) == 0) SetPanelVisible(_tcscmp(pstrValue, _T("true")) == 0);
	else if (_tcscmp(pstrName, _T("panelheight")) == 0) SetPanelHeight(_ttoi(pstrValue));
	else if (_tcscmp(pstrName, _T("paneloffset")) == 0) SetPanelOffset(_ttoi(pstrValue));
	else if (_tcscmp(pstrName, _T("panelpos")) == 0) SetPanelPos((_tcscmp(pstrValue, _T("top")) == 0) ? E_PANELTOP:E_PANELBOTTOM);
	else if (_tcscmp(pstrName, _T("panelxml")) == 0) SetPanelXml(pstrValue);
	else if (_tcscmp(pstrName, _T("panelattr")) == 0) SetPanelAttributeList(pstrValue);
    else if( _tcscmp(pstrName, _T("headerbkimage")) == 0 ) GetHeader()->SetBkImage(pstrValue);
    else if( _tcscmp(pstrName, _T("scrollselect")) == 0 ) SetScrollSelect(_tcscmp(pstrValue, _T("true")) == 0);
    else if( _tcscmp(pstrName, _T("multiexpanding")) == 0 ) SetMultiExpanding(_tcscmp(pstrValue, _T("true")) == 0);
    else if( _tcscmp(pstrName, _T("itemheight")) == 0 ) m_ListInfo.uFixedHeight = _ttoi(pstrValue);
    else if( _tcscmp(pstrName, _T("itemfont")) == 0 ) m_ListInfo.nFont = _ttoi(pstrValue);
    else if( _tcscmp(pstrName, _T("itemalign")) == 0 ) {
        if( _tcsstr(pstrValue, _T("left")) != NULL ) {
            m_ListInfo.uTextStyle &= ~(DT_CENTER | DT_RIGHT);
            m_ListInfo.uTextStyle |= DT_LEFT;
        }
        if( _tcsstr(pstrValue, _T("center")) != NULL ) {
            m_ListInfo.uTextStyle &= ~(DT_LEFT | DT_RIGHT);
            m_ListInfo.uTextStyle |= DT_CENTER;
        }
        if( _tcsstr(pstrValue, _T("right")) != NULL ) {
            m_ListInfo.uTextStyle &= ~(DT_LEFT | DT_CENTER);
            m_ListInfo.uTextStyle |= DT_RIGHT;
        }
    }
    else if (_tcscmp(pstrName, _T("itemvalign")) == 0)
    {
        if (_tcsstr(pstrValue, _T("top")) != NULL) {
            m_ListInfo.uTextStyle &= ~(DT_BOTTOM | DT_VCENTER);
            m_ListInfo.uTextStyle |= DT_TOP;
        }
        if (_tcsstr(pstrValue, _T("center")) != NULL) {
            m_ListInfo.uTextStyle &= ~(DT_TOP | DT_BOTTOM);
            m_ListInfo.uTextStyle |= DT_VCENTER;
        }
        if (_tcsstr(pstrValue, _T("bottom")) != NULL) {
            m_ListInfo.uTextStyle &= ~(DT_TOP | DT_VCENTER);
            m_ListInfo.uTextStyle |= DT_BOTTOM;
        }
    }
    else if( _tcscmp(pstrName, _T("itemendellipsis")) == 0 ) {
        if( _tcscmp(pstrValue, _T("true")) == 0 ) m_ListInfo.uTextStyle |= DT_END_ELLIPSIS;
        else m_ListInfo.uTextStyle &= ~DT_END_ELLIPSIS;
    }
    else if( _tcscmp(pstrName, _T("itemmultiline")) == 0 ) {
        if (_tcscmp(pstrValue, _T("true")) == 0) {
            m_ListInfo.uTextStyle &= ~DT_SINGLELINE;
            m_ListInfo.uTextStyle |= DT_WORDBREAK;
        }
        else m_ListInfo.uTextStyle |= DT_SINGLELINE;
    }
    else if( _tcscmp(pstrName, _T("itemtextpadding")) == 0 ) {
        RECT rcTextPadding = { 0 };
        LPTSTR pstr = NULL;
        rcTextPadding.left = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
        rcTextPadding.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);    
        rcTextPadding.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);    
        rcTextPadding.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);    
        SetItemTextPadding(rcTextPadding);
    }
    else if( _tcscmp(pstrName, _T("itemtextcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetItemTextColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itembkcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetItemBkColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itembkimage")) == 0 ) SetItemBkImage(pstrValue);
    else if( _tcscmp(pstrName, _T("itemaltbk")) == 0 ) SetAlternateBk(_tcscmp(pstrValue, _T("true")) == 0);
    else if( _tcscmp(pstrName, _T("itemselectedtextcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetSelectedItemTextColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itemselectedbkcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetSelectedItemBkColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itemselectedimage")) == 0 ) SetSelectedItemImage(pstrValue);
    else if( _tcscmp(pstrName, _T("itemhottextcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetHotItemTextColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itemhotbkcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetHotItemBkColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itemhotimage")) == 0 ) SetHotItemImage(pstrValue);
    else if( _tcscmp(pstrName, _T("itemdisabledtextcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetDisabledItemTextColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itemdisabledbkcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetDisabledItemBkColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itemdisabledimage")) == 0 ) SetDisabledItemImage(pstrValue);
    else if( _tcscmp(pstrName, _T("itemvlinesize")) == 0 ) {
        SetItemVLineSize(_ttoi(pstrValue));
    }
    else if( _tcscmp(pstrName, _T("itemvlinecolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetItemVLineColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itemhlinesize")) == 0 ) {
        SetItemHLineSize(_ttoi(pstrValue));
    }
    else if( _tcscmp(pstrName, _T("itemhlinecolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetItemHLineColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("itemshowhtml")) == 0 ) SetItemShowHtml(_tcscmp(pstrValue, _T("true")) == 0);
	else if (_tcscmp(pstrName, _T("itemtemplate")) == 0) SetItemTemplateXml(pstrValue);
    else CVerticalLayoutUI::SetAttribute(pstrName, pstrValue);
}

IListCallbackUI* CListUI::GetTextCallback() const
{
    return m_pCallback;
}

void CListUI::SetTextCallback(IListCallbackUI* pCallback)
{
    m_pCallback = pCallback;
}

SIZE CListUI::GetScrollPos() const
{
    return m_pList->GetScrollPos();
}

SIZE CListUI::GetScrollRange() const
{
    return m_pList->GetScrollRange();
}

void CListUI::SetScrollPos(SIZE szPos)
{
    m_pList->SetScrollPos(szPos);
}

void CListUI::DrawVirtualItem(CControlUI *pControl, int nDrawRow, int nStartDrwRow)
{
	if (m_pManager)
	{
		//#liulei  20161014在填充的过程中保持不可见，防止刷新太过频繁
		///> 容器是否会导致死循环刷新（有待测试）
		///> ListTextElement 会导致这个问题（实测）listtext-》settext-》invadate-》激发ListUi刷新-》激发ListText从而导致死循环刷新
		///> 启用优化填充数据之后，如果Item还有combo控件，因为combo会弹出窗口激发ListUI刷新，刷新的时候如果启用优化则会填充数据的时候隐藏Item
		///> 隐藏Item会导致Combo不可见，从而使弹出窗口消失。因此弹出窗口就一闪而过，因此如果含有Combo则不能启用刷新
		m_nDrawStartIndex = nStartDrwRow;
		if (m_bEnableVirtualO)
		{
			bool bOldVisible = pControl->IsVisible();
			pControl->SetInternVisible(false);
			m_pManager->SendNotify(this, DUI_MSGTYPE_DRAWITEM, (WPARAM)pControl, (LPARAM)nDrawRow);
			pControl->SetInternVisible(bOldVisible);
		}
		else
		{
			m_pManager->SendNotify(this, DUI_MSGTYPE_DRAWITEM, (WPARAM)pControl, (LPARAM)nDrawRow);
		}
	}
	
}

CDuiString GetContainText(CControlUI *pControl)
{
	if (pControl == NULL) return _T("");

	CDuiString strRet;
	if (pControl->GetInterface(DUI_CTR_CONTAINER))
	{
		CContainerUI *pContain = static_cast<CContainerUI *>(pControl);
		for (int i = 0; i < pContain->GetCount();++i)
		{
			strRet += GetContainText(pContain->GetItemAt(i));
		}
	}
	else
	{
		strRet += pControl->GetText();
	}
	return strRet;
}

BOOL CListUI::Copy(int nMaxRowItemData, bool bUserDefine)
{
	if (nMaxRowItemData <= 1) return FALSE;
	TCHAR *szBuffer = new TCHAR[nMaxRowItemData];
	memset(szBuffer, 0, sizeof(TCHAR)*nMaxRowItemData);
	int nRowCount = GetCount();
	int nColCount = m_pHeader->IsVisible() ? m_pHeader->GetCount():0;
	nColCount = max(nColCount, 1);
	CDuiString strText;

	if (m_pHeader->IsVisible())
	{
		for (int i = 0; i < nColCount; ++i)
		{
			CControlUI *pControl = m_pHeader->GetItemAt(i);
			if (pControl)
			{
				strText += pControl->GetText();
			}
			else
			{
				strText += _T("");
			}

			strText += _T("\t");
		}

		strText += _T("\r\n");
	}

	DUICopyItem item;
	item.szText = szBuffer;
	item.nszText = nMaxRowItemData;
	
	CControlUI *pItem = NULL;
	if (IsUseVirtualList())
	{
		///> 准备数据格式内存，需要释放数据delete
		pItem = m_pVirutalItemFormat ? m_pVirutalItemFormat->CreateVirtualItem() : CreateTemplateControl();
		assert(pItem);
	}

	for (int iRow = 0; iRow < nRowCount; ++iRow)
	{
		
		if (IsUseVirtualList())
		{
			///> 填充当前数据项
			if (m_pManager)
			{
				//#liulei  20161014在填充的过程中保持不可见，防止刷新太过频繁
				///> 容器是否会导致死循环刷新（有待测试）
				///> ListTextElement 会导致这个问题（实测）listtext-》settext-》invadate-》激发ListUi刷新-》激发ListText从而导致死循环刷新
				///> 启用优化填充数据之后，如果Item还有combo控件，因为combo会弹出窗口激发ListUI刷新，刷新的时候如果启用优化则会填充数据的时候隐藏Item
				///> 隐藏Item会导致Combo不可见，从而使弹出窗口消失。因此弹出窗口就一闪而过，因此如果含有Combo则不能启用刷新
				bool bOldVisible = pItem->IsVisible();
				pItem->SetInternVisible(false);
				m_pManager->SendNotify(this, DUI_MSGTYPE_DRAWITEM, (WPARAM)pItem, (LPARAM)iRow);
				pItem->SetInternVisible(bOldVisible);
			}
		}
		else
		{
			pItem = GetItemAt(iRow);
		}
		
		///> 断言当前项一定存在
		ASSERT(pItem);
		if (pItem == NULL) continue;

		///> 如果使用用户自定义的CopyData数据
		if (bUserDefine)
		{
			item.nRow = iRow;
			for (int iCol = 0; iCol < nColCount; ++iCol)
			{
				item.nCol = iCol;
				if (m_pManager)
					m_pManager->SendNotify(this, DUI_MSGTYPE_COPYITEM, (LPARAM)pItem, (WPARAM)&item);

				strText += szBuffer;
				strText += _T("\t");
			}
		}
		else///> 使用默认的CopyItem方式
		{
			if (pItem->GetInterface(DUI_CTR_LISTTEXTELEMENT))
			{
				CListTextElementUI *pText = static_cast<CListTextElementUI *>(pItem);
				int nCountText = pText->GetCount();
				for (int k = 0; k < nCountText;++k)
				{
					strText += pText->GetText(k);
					strText += _T("\t");
				}
			}
			else if (pItem->GetInterface(DUI_CTR_LISTLABELELEMENT))
			{
				strText += pItem->GetText();
			}
			else if (pItem->GetInterface(DUI_CTR_LISTHBOXELEMENT))
			{
				CListHBoxElementUI *pContain = static_cast<CListHBoxElementUI *>(pItem);
				for (int k = 0; k < pContain->GetCount(); ++k)
				{
					strText += GetContainText(pContain->GetItemAt(k));
					strText += _T("\t");
				}

			}
			else if (pItem->GetInterface(DUI_CTR_LISTCONTAINERELEMENT))
			{
				CListContainerElementUI *pContain = static_cast<CListContainerElementUI *>(pItem);
				for (int k = 0; k < pContain->GetCount();++k)
				{
					strText += GetContainText(pContain);
				}
			}
			else
			{
				strText += pItem->GetText();
			}
		}
		strText += _T("\r\n");
	}

	delete[]szBuffer;
	///> 如果使用了虚表释放虚表准备的数据格式内存
	if (IsUseVirtualList() && pItem)
	{
		pItem->Delete();
		pItem = NULL;
	}

	if (strText.GetLength() > 0)
	{
		///> 复制表头
		CListHeaderUI *pHeader = GetHeader();
		///> 打开剪切板
		if (!OpenClipboard(NULL)) return FALSE;
		///>  清空剪切板数据
		::EmptyClipboard();
		///> 分配全局内存
		int nSize = (strText.GetLength() + 1)*sizeof(TCHAR);
		HGLOBAL clipbuffer = ::GlobalAlloc(GMEM_DDESHARE, nSize);
		if (clipbuffer == NULL)
		{
			::CloseClipboard();
			return FALSE;
		}
		///> 锁定资源
		TCHAR *szlockbuf = (TCHAR *)::GlobalLock(clipbuffer);
		if (szlockbuf == NULL)
		{
			::GlobalFree(clipbuffer);
			::CloseClipboard();
			return FALSE;
		}
		//memset(szlockbuf, 0, sizeof(TCHAR)*(strText.GetLength() + 1));
		lstrcpyn(szlockbuf, strText.GetData(), strText.GetLength() + 1);
		///> 解锁资源
		GlobalUnlock(clipbuffer);

		///> 设置剪切板数据
#ifdef _UNICODE
		::SetClipboardData(CF_UNICODETEXT, clipbuffer);
#else
		::SetClipboardData(CF_TEXT, clipbuffer);
#endif // _UNICODE

		
		///> 关闭剪切板
		::CloseClipboard();
	}
	return TRUE;
}

void CListUI::LineUp()
{
    m_pList->LineUp();
}

void CListUI::LineDown()
{
    m_pList->LineDown();
}

void CListUI::PageUp()
{
    m_pList->PageUp();
}

void CListUI::PageDown()
{
    m_pList->PageDown();
}

void CListUI::HomeUp()
{
    m_pList->HomeUp();
}

void CListUI::EndDown()
{
    m_pList->EndDown();
}

void CListUI::LineLeft()
{
    m_pList->LineLeft();
}

void CListUI::LineRight()
{
    m_pList->LineRight();
}

void CListUI::PageLeft()
{
    m_pList->PageLeft();
}

void CListUI::PageRight()
{
    m_pList->PageRight();
}

void CListUI::HomeLeft()
{
    m_pList->HomeLeft();
}

void CListUI::EndRight()
{
    m_pList->EndRight();
}

void CListUI::EnableScrollBar(bool bEnableVertical, bool bEnableHorizontal)
{
    m_pList->EnableScrollBar(bEnableVertical, bEnableHorizontal);
}

CScrollBarUI* CListUI::GetVerticalScrollBar() const
{
    return m_pList->GetVerticalScrollBar();
}

CScrollBarUI* CListUI::GetHorizontalScrollBar() const
{
    return m_pList->GetHorizontalScrollBar();
}

bool CListUI::SortItems(PULVCompareFunc pfnCompare, UINT_PTR dwData)
{
	if (!m_pList) return false;
	int iCurSel = m_iCurSel;
	bool bResult = m_pList->SortItems(pfnCompare, dwData, iCurSel);
	if (bResult) {
		m_iCurSel = iCurSel;
		EnsureVisible(m_iCurSel);
		NeedUpdate();
	}
	return bResult;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListBodyUI::CListBodyUI(CListUI* pOwner) : m_pOwner(pOwner)
{
    ASSERT(m_pOwner);
}

bool CListBodyUI::SortItems(PULVCompareFunc pfnCompare, UINT_PTR dwData, int& iCurSel)
{
	if (!pfnCompare) return false;
	m_pCompareFunc = pfnCompare;
	CControlUI *pCurSelControl = GetItemAt(iCurSel);
	CControlUI **pData = (CControlUI **)m_items.GetData();
	qsort_s(m_items.GetData(), m_items.GetSize(), sizeof(CControlUI*), CListBodyUI::ItemComareFunc, this);
	if (pCurSelControl) iCurSel = GetItemIndex(pCurSelControl);
	IListItemUI *pItem = NULL;
	for (int i = 0; i < m_items.GetSize(); ++i)
	{
		pItem = (IListItemUI*)(static_cast<CControlUI*>(m_items[i])->GetInterface(TEXT("ListItem")));
		if (pItem)
		{
			pItem->SetIndex(i);
		}
	}

	return true;
}

int __cdecl CListBodyUI::ItemComareFunc(void *pvlocale, const void *item1, const void *item2)
{
	CListBodyUI *pThis = (CListBodyUI*)pvlocale;
	if (!pThis || !item1 || !item2)
		return 0;
	return pThis->ItemComareFunc(item1, item2);
}

int __cdecl CListBodyUI::ItemComareFunc(const void *item1, const void *item2)
{
	CControlUI *pControl1 = *(CControlUI**)item1;
	CControlUI *pControl2 = *(CControlUI**)item2;
	return m_pCompareFunc((UINT_PTR)pControl1, (UINT_PTR)pControl2, m_compareData);
}

void CListBodyUI::SetScrollPos(SIZE szPos)
{
    int cx = 0;
    int cy = 0;
    if( m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) {
        int iLastScrollPos = m_pVerticalScrollBar->GetScrollPos();
        m_pVerticalScrollBar->SetScrollPos(szPos.cy);
        cy = m_pVerticalScrollBar->GetScrollPos() - iLastScrollPos;
    }

    if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) {
        int iLastScrollPos = m_pHorizontalScrollBar->GetScrollPos();
        m_pHorizontalScrollBar->SetScrollPos(szPos.cx);
        cx = m_pHorizontalScrollBar->GetScrollPos() - iLastScrollPos;
    }

    if( cx == 0 && cy == 0 ) return;

	if (m_pOwner && m_pOwner->IsUseVirtualList()) cy = 0;

    for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) continue;
		pControl->Move(CDuiSize(-cx, -cy), false);
    }

    Invalidate();

    if( cx != 0 && m_pOwner ) {
        CListHeaderUI* pHeader = m_pOwner->GetHeader();
        if( pHeader == NULL ) return;
		//#liulei 20161118 移动表头内所有的控件,复合控件使用
		for (int i = 0; i < pHeader->GetCount(); i++) {
			CControlUI* pControl = static_cast<CControlUI*>(pHeader->GetItemAt(i));
			if (!pControl->IsVisible()) continue;
			if (pControl->IsFloat()) continue;
			pControl->Move(CDuiSize(-cx, -cy), false);
		}

		//#liulei 调整复合表头的listheaderItem对应的区域关系
        TListInfoUI* pInfo = m_pOwner->GetListInfo();
		CDuiPtrArray ptrAry;
		GetInsideControl(ptrAry, pHeader, DUI_CTR_LISTHEADERITEM);
		pInfo->nColumns = MIN(ptrAry.GetSize(), UILIST_MAX_COLUMNS);
        for( int i = 0; i < pInfo->nColumns; i++ ) {
			CControlUI* pControl = static_cast<CControlUI*>(ptrAry[i]);
			pInfo->rcColumn[i] = pControl->GetPos();
        }
		pHeader->Invalidate();
    }
}

void CListBodyUI::SetPos(RECT rc, bool bNeedInvalidate)
{
    CControlUI::SetPos(rc, bNeedInvalidate);
    rc = m_rcItem;

    // Adjust for inset
    rc.left += m_rcInset.left;
    rc.top += m_rcInset.top;
    rc.right -= m_rcInset.right;
    rc.bottom -= m_rcInset.bottom;
    if( m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) rc.right -= m_pVerticalScrollBar->GetFixedWidth();
    if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) rc.bottom -= m_pHorizontalScrollBar->GetFixedHeight();

    // Determine the minimum size
    SIZE szAvailable = { rc.right - rc.left, rc.bottom - rc.top };
    if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) 
        szAvailable.cx += m_pHorizontalScrollBar->GetScrollRange();

    int iChildPadding = m_iChildPadding;
    TListInfoUI* pInfo = NULL;
    if( m_pOwner ) {
        pInfo = m_pOwner->GetListInfo();
        if( pInfo != NULL ) {
            iChildPadding += pInfo->iHLineSize;
            if (pInfo->nColumns > 0)
			{
				//#liulei 取消限制list Item的宽度始终和列保持一致
				//列的宽度始终和ListUI的宽度保持一致
                //szAvailable.cx = pInfo->rcColumn[pInfo->nColumns - 1].right - pInfo->rcColumn[0].left;
            }
        }
    }

    int cxNeeded = 0;
    int cyFixed = 0;
    int nEstimateNum = 0;
    SIZE szControlAvailable;
    int iControlMaxWidth = 0;
    int iControlMaxHeight = 0;
    for( int it1 = 0; it1 < m_items.GetSize(); it1++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_items[it1]);
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) continue;
        szControlAvailable = szAvailable;
        RECT rcPadding = pControl->GetPadding();
        szControlAvailable.cx -= rcPadding.left + rcPadding.right;
        iControlMaxWidth = pControl->GetFixedWidth();
        iControlMaxHeight = pControl->GetFixedHeight();
        if (iControlMaxWidth <= 0) iControlMaxWidth = pControl->GetMaxWidth(); 
        if (iControlMaxHeight <= 0) iControlMaxHeight = pControl->GetMaxHeight();
        if (szControlAvailable.cx > iControlMaxWidth) szControlAvailable.cx = iControlMaxWidth;
        if (szControlAvailable.cy > iControlMaxHeight) szControlAvailable.cy = iControlMaxHeight;
        SIZE sz = pControl->EstimateSize(szAvailable);
        if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
        if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();
        cyFixed += sz.cy + pControl->GetPadding().top + pControl->GetPadding().bottom;

        sz.cx = MAX(sz.cx, 0);
        if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();
        if( sz.cx > pControl->GetMaxWidth() ) sz.cx = pControl->GetMaxWidth();
        cxNeeded = MAX(cxNeeded, sz.cx);
        nEstimateNum++;
    }
    cyFixed += (nEstimateNum - 1) * iChildPadding;

    if( m_pOwner ) {
        CListHeaderUI* pHeader = m_pOwner->GetHeader();
        if( pHeader != NULL && pHeader->GetCount() > 0 ) {
            cxNeeded = MAX(0, pHeader->EstimateSize(CDuiSize(rc.right - rc.left, rc.bottom - rc.top)).cx);
        }
    }

    // Place elements
    int cyNeeded = 0;
    // Position the elements
    SIZE szRemaining = szAvailable;
    int iPosY = rc.top;
    if(!m_pOwner->IsUseVirtualList() && m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) {
        iPosY -= m_pVerticalScrollBar->GetScrollPos();
    }
    int iPosX = rc.left;
    if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) {
        iPosX -= m_pHorizontalScrollBar->GetScrollPos();
    }
    
    int iAdjustable = 0;
    for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
        CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
        if( !pControl->IsVisible() ) continue;
        if( pControl->IsFloat() ) {
            SetFloatPos(it2);
            continue;
        }

        RECT rcPadding = pControl->GetPadding();
        szRemaining.cy -= rcPadding.top;
        szControlAvailable = szRemaining;
        szControlAvailable.cx -= rcPadding.left + rcPadding.right;
        iControlMaxWidth = pControl->GetFixedWidth();
        iControlMaxHeight = pControl->GetFixedHeight();
        if (iControlMaxWidth <= 0) iControlMaxWidth = pControl->GetMaxWidth(); 
        if (iControlMaxHeight <= 0) iControlMaxHeight = pControl->GetMaxHeight();
        if (szControlAvailable.cx > iControlMaxWidth) szControlAvailable.cx = iControlMaxWidth;
        if (szControlAvailable.cy > iControlMaxHeight) szControlAvailable.cy = iControlMaxHeight;
        SIZE sz = pControl->EstimateSize(szControlAvailable);
        if( sz.cy < pControl->GetMinHeight() ) sz.cy = pControl->GetMinHeight();
        if( sz.cy > pControl->GetMaxHeight() ) sz.cy = pControl->GetMaxHeight();
        sz.cx = pControl->GetMaxWidth();
        if( sz.cx == 0 ) sz.cx = szAvailable.cx - rcPadding.left - rcPadding.right;
        if( sz.cx < 0 ) sz.cx = 0;
        if( sz.cx > szControlAvailable.cx ) sz.cx = szControlAvailable.cx;
        if( sz.cx < pControl->GetMinWidth() ) sz.cx = pControl->GetMinWidth();

        RECT rcCtrl = { iPosX + rcPadding.left, iPosY + rcPadding.top, iPosX + rcPadding.left + sz.cx, iPosY + sz.cy + rcPadding.top + rcPadding.bottom };
        pControl->SetPos(rcCtrl, false);

        iPosY += sz.cy + iChildPadding + rcPadding.top + rcPadding.bottom;
        cyNeeded += sz.cy + rcPadding.top + rcPadding.bottom;
        szRemaining.cy -= sz.cy + iChildPadding + rcPadding.bottom;
    }
    cyNeeded += (nEstimateNum - 1) * iChildPadding;

	if (m_pOwner->IsUseVirtualList())
	{
		cyNeeded = m_pOwner->GetVirtualItemHeight()*(m_pOwner->GetVirtualItemCount()+1);
	}
    // Process the scrollbar
    ProcessScrollBar(rc, cxNeeded, cyNeeded);
}

void CListBodyUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CControlUI::DoEvent(event);
        return;
    }

	if (m_pOwner != NULL ) {
		if (event.Type == UIEVENT_SCROLLWHEEL) {
			if (m_pOwner->IsEnableMouseWhell() && m_pHorizontalScrollBar != NULL && m_pHorizontalScrollBar->IsVisible() && m_pHorizontalScrollBar->IsEnabled()) {
				RECT rcHorizontalScrollBar = m_pHorizontalScrollBar->GetPos();
				if( ::PtInRect(&rcHorizontalScrollBar, event.ptMouse) ) 
				{
					switch( LOWORD(event.wParam) ) {
					case SB_LINEUP:
						m_pOwner->LineLeft();
						return;
					case SB_LINEDOWN:
						m_pOwner->LineRight();
						return;
					}
				}
			}
		}
		m_pOwner->DoEvent(event); }
	else {
		CControlUI::DoEvent(event);
	}
}

bool CListBodyUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
    RECT rcTemp = { 0 };
    if( !::IntersectRect(&rcTemp, &rcPaint, &m_rcItem) ) return true;

    TListInfoUI* pListInfo = NULL;
    if( m_pOwner ) pListInfo = m_pOwner->GetListInfo();

    CRenderClip clip;
    CRenderClip::GenerateClip(hDC, rcTemp, clip);
    CControlUI::DoPaint(hDC, rcPaint, pStopControl);

	 if( m_items.GetSize() > 0 ) {
        RECT rc = m_rcItem;
        rc.left += m_rcInset.left;
        rc.top += m_rcInset.top;
        rc.right -= m_rcInset.right;
        rc.bottom -= m_rcInset.bottom;
        if( m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) rc.right -= m_pVerticalScrollBar->GetFixedWidth();
        if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) rc.bottom -= m_pHorizontalScrollBar->GetFixedHeight();
     
		///> 计算虚拟列表的起始位置
		int nVirtualStartIndex = 0;
		if (m_pOwner->IsUseVirtualList() && m_items.GetSize() > 0 && m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible())
		{
			CControlUI* pControl = static_cast<CControlUI*>(m_items[0]);
			assert(pControl && pControl->GetFixedHeight() > 0);
			nVirtualStartIndex = m_pVerticalScrollBar->GetScrollPos() / pControl->GetFixedHeight();
		}

        if( !::IntersectRect(&rcTemp, &rcPaint, &rc) ) 
		{
			int nItemSize = GetHeight() / (m_pOwner->GetVirtualItemHeight() <= 0 ? VIR_ITEM_HEIGHT : m_pOwner->GetVirtualItemHeight()) + 5;
			if (!m_pOwner->IsUseVirtualList()) nItemSize = m_items.GetSize();

			int nSelectIndex = -1;
			for (int it = 0; it < m_items.GetSize() && it < nItemSize; it++) {
                CControlUI* pControl = static_cast<CControlUI*>(m_items[it]);
				//> 如果使用虚拟列表，则设置虚拟列表数据
				if (m_pOwner->IsUseVirtualList())
				{
					if (nVirtualStartIndex + it < m_pOwner->GetVirtualItemCount() &&
						nVirtualStartIndex + it >= 0)
					{
						pControl->SetVisible(true);
						//m_pVirtualData(pControl, nVirtualStartIndex + it, m_pContext);
						m_pOwner->DrawVirtualItem(pControl, nVirtualStartIndex + it, nVirtualStartIndex);
					}
					else if (m_pOwner->IsUseVirtualList())
					{
						pControl->SetVisible(false);
					}

					if (nSelectIndex == -1 && m_pOwner->GetSelectControlTag() != -1 && m_pOwner->GetSelectControlTag() == pControl->GetTag())
					{
						nSelectIndex = it;
					}
				}

                if( pControl == pStopControl ) return false;
                if( !pControl->IsVisible() ) continue;
                if( !::IntersectRect(&rcTemp, &rcPaint, &pControl->GetPos()) ) continue;
                if( pControl->IsFloat() ) {
                    if( !::IntersectRect(&rcTemp, &m_rcItem, &pControl->GetPos()) ) continue;
                    if( !pControl->Paint(hDC, rcPaint, pStopControl) ) return false;
                }
            }

			if (m_pOwner->IsUseVirtualList() && m_pOwner->GetSelectControlTag() != -1)
			{
				m_pOwner->SelectItem(nSelectIndex, false, false);
			}
        }
        else {
            int iDrawIndex = 0;
            CRenderClip childClip;
            CRenderClip::GenerateClip(hDC, rcTemp, childClip);
			int nItemSize = GetHeight() / (m_pOwner->GetVirtualItemHeight() <= 0 ? VIR_ITEM_HEIGHT : m_pOwner->GetVirtualItemHeight()) + 5;
			if (!m_pOwner->IsUseVirtualList()) nItemSize = m_items.GetSize();

			int nSelectIndex = -1;
            for( int it = 0; it < m_items.GetSize() && it < nItemSize; it++ ) {
                CControlUI* pControl = static_cast<CControlUI*>(m_items[it]);
				//> 如果使用虚拟列表，则设置虚拟列表数据
				if (m_pOwner->IsUseVirtualList())
				{
					if (nVirtualStartIndex + it < m_pOwner->GetVirtualItemCount() &&
						nVirtualStartIndex + it >= 0)
					{
						pControl->SetVisible(true);
						//m_pVirtualData(pControl, nVirtualStartIndex + it, m_pContext);
						m_pOwner->DrawVirtualItem(pControl, nVirtualStartIndex + it, nVirtualStartIndex);
					}
					else if (m_pOwner->IsUseVirtualList())
					{
						pControl->SetVisible(false);
					}

					if (nSelectIndex == -1 && m_pOwner->GetSelectControlTag() != -1 && m_pOwner->GetSelectControlTag() == pControl->GetTag())
					{
						nSelectIndex = it;
					}
				}

                if( pControl == pStopControl ) return false;
                if( !pControl->IsVisible() ) continue;
                if( !pControl->IsFloat() ) {
                    IListItemUI* pListItem = static_cast<IListItemUI*>(pControl->GetInterface(DUI_CTR_ILISTITEM));
                    if( pListItem != NULL ) {
                        pListItem->SetDrawIndex(iDrawIndex);
                        iDrawIndex += 1;
                    }
                    if (pListInfo && pListInfo->iHLineSize > 0) {
                        // 因为没有为最后一个预留分割条长度，如果list铺满，最后一条不会显示
                        RECT rcPadding = pControl->GetPadding();
                        const RECT& rcPos = pControl->GetPos();
                        RECT rcBottomLine = { rcPos.left, rcPos.bottom + rcPadding.bottom, rcPos.right, rcPos.bottom + rcPadding.bottom + pListInfo->iHLineSize };
                        if( ::IntersectRect(&rcTemp, &rcPaint, &rcBottomLine) ) {
                            rcBottomLine.top += pListInfo->iHLineSize / 2;
                            rcBottomLine.bottom = rcBottomLine.top;
                            CRenderEngine::DrawLine(hDC, rcBottomLine, pListInfo->iHLineSize, GetAdjustColor(pListInfo->dwHLineColor));
                        }
                    }
                }
                if( !::IntersectRect(&rcTemp, &rcPaint, &pControl->GetPos()) ) continue;
                if( pControl->IsFloat() ) {
                    if( !::IntersectRect(&rcTemp, &m_rcItem, &pControl->GetPos()) ) continue;
                    CRenderClip::UseOldClipBegin(hDC, childClip);
                    if( !pControl->Paint(hDC, rcPaint, pStopControl) ) return false;
                    CRenderClip::UseOldClipEnd(hDC, childClip);
                }
                else {
                    if( !::IntersectRect(&rcTemp, &rc, &pControl->GetPos()) ) continue;
                    if( !pControl->Paint(hDC, rcPaint, pStopControl) ) return false;
                }
            }

			if (m_pOwner->IsUseVirtualList() && m_pOwner->GetSelectControlTag() != -1)
			{
				m_pOwner->SelectItem(nSelectIndex, false, false);
			}
        }
    }

    if( m_pVerticalScrollBar != NULL ) {
        if( m_pVerticalScrollBar == pStopControl ) return false;
        if (m_pVerticalScrollBar->IsVisible()) {
            if( ::IntersectRect(&rcTemp, &rcPaint, &m_pVerticalScrollBar->GetPos()) ) {
                if( !m_pVerticalScrollBar->Paint(hDC, rcPaint, pStopControl) ) return false;
            }
        }
    }

    if( m_pHorizontalScrollBar != NULL ) {
        if( m_pHorizontalScrollBar == pStopControl ) return false;
        if (m_pHorizontalScrollBar->IsVisible()) {
            if( ::IntersectRect(&rcTemp, &rcPaint, &m_pHorizontalScrollBar->GetPos()) ) {
                if( !m_pHorizontalScrollBar->Paint(hDC, rcPaint, pStopControl) ) return false;
            }
        }
    }
    return true;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListHeaderUI::CListHeaderUI()
{
	m_pSortHeaderItem = NULL;
}

LPCTSTR CListHeaderUI::GetClass() const
{
    return DUI_CTR_LISTHEADER;
}

LPVOID CListHeaderUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcscmp(pstrName, DUI_CTR_LISTHEADER) == 0 ) return this;
    return CHorizontalLayoutUI::GetInterface(pstrName);
}

void CListHeaderUI::SetSort(int nIndex, ESORT esort, bool bTriggerEvent)
{
	CControlUI *pControl = GetItemAt(nIndex);
	if (pControl && pControl->GetInterface(DUI_CTR_LISTHEADERITEM))
	{
		CListHeaderItemUI *pHeader = static_cast<CListHeaderItemUI *>(pControl);
		pHeader->SetSort(esort, bTriggerEvent);
	}
}

CListHeaderItemUI *CListHeaderUI::GetSortHeaderItem()
{
	CDuiPtrArray ptrary;
	GetInsideControl(ptrary,this,DUI_CTR_LISTHEADERITEM);
	for (int i = 0; i < ptrary.GetSize();++i)
	{
		if (m_pSortHeaderItem == static_cast<CListHeaderItemUI *>(ptrary[i]))
		{
			return m_pSortHeaderItem;
		}
	}
	return NULL;
}

///> #liulei 修复表头删除所有的时候，ListBody位置没有重计算的问题
bool CListHeaderUI::Add(CControlUI* pControl)
{
	bool bRet = __super::Add(pControl);
	if (bRet)
		NeedParentUpdate();
	return bRet;
}
///> #liulei 修复表头删除所有的时候，ListBody位置没有重计算的问题
bool CListHeaderUI::AddAt(CControlUI* pControl, int iIndex)
{
	bool bRet = __super::AddAt(pControl, iIndex);
	if (bRet)
		NeedParentUpdate();
	return bRet;
}
///> #liulei 修复表头删除所有的时候，ListBody位置没有重计算的问题
void CListHeaderUI::RemoveAll()
{
	__super::RemoveAll();
	NeedParentUpdate();
}
///> #liulei 修复表头删除所有的时候，ListBody位置没有重计算的问题
bool CListHeaderUI::Remove(CControlUI* pControl, bool bDoNotDestroy)
{
	bool bRet = __super::Remove(pControl, bDoNotDestroy);
	if(bRet)
		NeedParentUpdate();
	return bRet;
}
///> #liulei 修复表头删除所有的时候，ListBody位置没有重计算的问题
bool CListHeaderUI::RemoveAt(int iIndex, bool bDoNotDestroy)
{
	bool bRet = __super::RemoveAt(iIndex, bDoNotDestroy);
	if (bRet)
		NeedParentUpdate();
	return bRet;
}
///> #liulei 修复表头动态显示，ListBody位置没有重计算的问题
void CListHeaderUI::SetVisible(bool bVisible)
{
	__super::SetVisible(bVisible);
	NeedParentUpdate();
}

void CListHeaderUI::SetItemVisible(int nIndex, bool bVisible, bool bInvalidate)
{
	if (GetItemAt(nIndex))
	{
		GetItemAt(nIndex)->SetVisible(bVisible);
		if (bInvalidate)
			NeedParentUpdate();
	}
}

SIZE CListHeaderUI::EstimateSize(SIZE szAvailable)
{
    SIZE cXY = {0, m_cxyFixed.cy};
	if( cXY.cy == 0 && m_pManager != NULL ) {
		for( int it = 0; it < m_items.GetSize(); it++ ) {
			cXY.cy = MAX(cXY.cy,static_cast<CControlUI*>(m_items[it])->EstimateSize(szAvailable).cy);
		}
		int nMin = m_pManager->GetDefaultFontInfo()->tm.tmHeight + 8;
		cXY.cy = MAX(cXY.cy,nMin);
	}

    for( int it = 0; it < m_items.GetSize(); it++ ) {
        cXY.cx +=  static_cast<CControlUI*>(m_items[it])->EstimateSize(szAvailable).cx;
    }

    return cXY;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListHeaderItemUI::CListHeaderItemUI() : m_bDragable(true), m_uButtonState(0), m_iSepWidth(4),
m_uTextStyle(DT_CENTER | DT_VCENTER | DT_SINGLELINE), m_dwTextColor(0), m_dwSepColor(0), 
m_iFont(-1), m_bShowHtml(false)
{
	m_bEnablebSort = false;
	m_nSortHeight = 16;
	m_nSortWidth = 16;
	m_esrot = E_SORTNO;//默认不排序
	SetTextPadding(CDuiRect(2, 0, 2, 0));
    ptLastMouse.x = ptLastMouse.y = 0;
    SetMinWidth(16);
}

LPCTSTR CListHeaderItemUI::GetClass() const
{
    return DUI_CTR_LISTHEADERITEM;
}

LPVOID CListHeaderItemUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcscmp(pstrName, DUI_CTR_LISTHEADERITEM) == 0 ) return this;
	return __super::GetInterface(pstrName);
}

UINT CListHeaderItemUI::GetControlFlags() const
{
    if( IsEnabled() && m_iSepWidth != 0 ) return UIFLAG_SETCURSOR;
    else return 0;
}

void CListHeaderItemUI::SetEnabled(bool bEnable)
{
    CControlUI::SetEnabled(bEnable);
    if( !IsEnabled() ) {
        m_uButtonState = 0;
    }
}

///> #liulei 修复表头动态显示，界面刷新问题
void CListHeaderItemUI::SetVisible(bool bVisible)
{
	if (m_bVisible == bVisible) return;
	__super::SetVisible(bVisible);
	if (GetParent())
		GetParent()->NeedParentUpdate();
}

bool CListHeaderItemUI::IsDragable() const
{
	return m_bDragable;
}

void CListHeaderItemUI::SetDragable(bool bDragable)
{
    m_bDragable = bDragable;
    if ( !m_bDragable ) m_uButtonState &= ~UISTATE_CAPTURED;
}

DWORD CListHeaderItemUI::GetSepWidth() const
{
	return m_iSepWidth;
}

void CListHeaderItemUI::SetSepWidth(int iWidth)
{
    m_iSepWidth = iWidth;
}

DWORD CListHeaderItemUI::GetTextStyle() const
{
	return m_uTextStyle;
}

void CListHeaderItemUI::SetTextStyle(UINT uStyle)
{
    m_uTextStyle = uStyle;
    Invalidate();
}

DWORD CListHeaderItemUI::GetTextColor() const
{
	return m_dwTextColor;
}


void CListHeaderItemUI::SetTextColor(DWORD dwTextColor)
{
    m_dwTextColor = dwTextColor;
    Invalidate();
}

DWORD CListHeaderItemUI::GetSepColor() const
{
    return m_dwSepColor;
}

void CListHeaderItemUI::SetSepColor(DWORD dwSepColor)
{
    m_dwSepColor = dwSepColor;
    Invalidate();
}

RECT CListHeaderItemUI::GetTextPadding() const
{
	return m_rcTextPadding;
}

void CListHeaderItemUI::SetTextPadding(RECT rc)
{
	m_rcTextPadding = rc;
	Invalidate();
}

void CListHeaderItemUI::SetFont(int index)
{
    m_iFont = index;
}

bool CListHeaderItemUI::IsShowHtml()
{
    return m_bShowHtml;
}

void CListHeaderItemUI::SetShowHtml(bool bShowHtml)
{
    if( m_bShowHtml == bShowHtml ) return;

    m_bShowHtml = bShowHtml;
    Invalidate();
}

LPCTSTR CListHeaderItemUI::GetNormalImage() const
{
	return m_diNormal.sDrawString;
}

void CListHeaderItemUI::SetNormalImage(LPCTSTR pStrImage)
{
	if( m_diNormal.sDrawString == pStrImage && m_diNormal.pImageInfo != NULL ) return;
	m_diNormal.Clear();
	m_diNormal.sDrawString = pStrImage;
	Invalidate();
}

LPCTSTR CListHeaderItemUI::GetHotImage() const
{
	return m_diHot.sDrawString;
}

void CListHeaderItemUI::SetHotImage(LPCTSTR pStrImage)
{
	if( m_diHot.sDrawString == pStrImage && m_diHot.pImageInfo != NULL ) return;
	m_diHot.Clear();
	m_diHot.sDrawString = pStrImage;
	Invalidate();
}

LPCTSTR CListHeaderItemUI::GetPushedImage() const
{
	return m_diPushed.sDrawString;
}

void CListHeaderItemUI::SetPushedImage(LPCTSTR pStrImage)
{
	if( m_diPushed.sDrawString == pStrImage && m_diPushed.pImageInfo != NULL ) return;
	m_diPushed.Clear();
	m_diPushed.sDrawString = pStrImage;
	Invalidate();
}

LPCTSTR CListHeaderItemUI::GetFocusedImage() const
{
	return m_diFocused.sDrawString;
}

void CListHeaderItemUI::SetFocusedImage(LPCTSTR pStrImage)
{
	if( m_diFocused.sDrawString == pStrImage && m_diFocused.pImageInfo != NULL ) return;
	m_diFocused.Clear();
	m_diFocused.sDrawString = pStrImage;
	Invalidate();
}

LPCTSTR CListHeaderItemUI::GetSepImage() const
{
	return m_diSep.sDrawString;
}

void CListHeaderItemUI::SetSepImage(LPCTSTR pStrImage)
{
	if( m_diSep.sDrawString == pStrImage && m_diSep.pImageInfo != NULL ) return;
	m_diSep.Clear();
	m_diSep.sDrawString = pStrImage;
	Invalidate();
}

void CListHeaderItemUI::SetEnabledSort(bool bEnableSort)
{
	if (m_bEnablebSort == bEnableSort) return;
	m_bEnablebSort = bEnableSort;
	Invalidate();
}

void CListHeaderItemUI::SetSort(ESORT esort, bool bTriggerEvent)
{
	//> 查找父级Item，重置其他Item的状态
	CControlUI *pParent = GetParent();
	while (pParent && (pParent->GetInterface(DUI_CTR_LISTHEADER) == NULL))
	{
		pParent = pParent->GetParent();
	}

	if (pParent)
	{
		CListHeaderUI *pHeader = static_cast<CListHeaderUI *>(pParent);
		for (int i = 0; i < pHeader->GetCount(); ++i)
		{
			CDuiPtrArray pary;
			GetInsideControl(pary, pHeader->GetItemAt(i), DUI_CTR_LISTHEADERITEM);
			for (int k = 0; k < pary.GetSize();++k)
			{
				CListHeaderItemUI *pHederItem = static_cast<CListHeaderItemUI*>(pary[k]);
				if (pHederItem != this)
				{
					pHederItem->SetSortStatus(E_SORTNO);
				}
			}
		}
		pHeader->m_pSortHeaderItem = this;
	}

	SetSortStatus(esort, bTriggerEvent);
}



void CListHeaderItemUI::SetSortStatus(ESORT esort, bool bTriggerEvent)
{
	if (!m_bEnablebSort) return;
	if (m_esrot == esort) return;
	m_esrot = esort;
	Invalidate();
 	if (bTriggerEvent)
 		m_pManager->SendNotify(this, DUI_MSGTYPE_SORT,m_esrot);
}

ESORT CListHeaderItemUI::GetSort()
{
	return m_esrot;
}

void CListHeaderItemUI::SetSortAscImg(LPCTSTR pStrImage)
{
	if (m_diAscSort.sDrawString == pStrImage && m_diAscSort.pImageInfo != NULL) return;
	m_diAscSort.Clear();
	m_diAscSort.sDrawString = pStrImage;
	Invalidate();
}

void CListHeaderItemUI::SetSortDescImg(LPCTSTR pStrImage)
{
	if (m_diDescSort.sDrawString == pStrImage && m_diDescSort.pImageInfo != NULL) return;
	m_diDescSort.Clear();
	m_diDescSort.sDrawString = pStrImage;
	Invalidate();
}

void CListHeaderItemUI::SetSortWidth(int nSortWidht)
{
	m_nSortWidth = nSortWidht;
	Invalidate();
}

void CListHeaderItemUI::SetSortHeight(int nSrotHeight)
{
	m_nSortHeight = nSrotHeight;
	Invalidate();
}

void CListHeaderItemUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if( _tcscmp(pstrName, _T("dragable")) == 0 ) SetDragable(_tcscmp(pstrValue, _T("true")) == 0);
    else if( _tcscmp(pstrName, _T("align")) == 0 ) {
        if( _tcsstr(pstrValue, _T("left")) != NULL ) {
            m_uTextStyle &= ~(DT_CENTER | DT_RIGHT);
            m_uTextStyle |= DT_LEFT;
        }
        if( _tcsstr(pstrValue, _T("center")) != NULL ) {
            m_uTextStyle &= ~(DT_LEFT | DT_RIGHT);
            m_uTextStyle |= DT_CENTER;
        }
        if( _tcsstr(pstrValue, _T("right")) != NULL ) {
            m_uTextStyle &= ~(DT_LEFT | DT_CENTER);
            m_uTextStyle |= DT_RIGHT;
        }
    }
    else if (_tcscmp(pstrName, _T("valign")) == 0)
    {
        if (_tcsstr(pstrValue, _T("top")) != NULL) {
            m_uTextStyle &= ~(DT_BOTTOM | DT_VCENTER);
            m_uTextStyle |= DT_TOP;
        }
        if (_tcsstr(pstrValue, _T("center")) != NULL) {
            m_uTextStyle &= ~(DT_TOP | DT_BOTTOM);
            m_uTextStyle |= DT_VCENTER;
        }
        if (_tcsstr(pstrValue, _T("bottom")) != NULL) {
            m_uTextStyle &= ~(DT_TOP | DT_VCENTER);
            m_uTextStyle |= DT_BOTTOM;
        }
    }
    else if( _tcscmp(pstrName, _T("endellipsis")) == 0 ) {
        if( _tcscmp(pstrValue, _T("true")) == 0 ) m_uTextStyle |= DT_END_ELLIPSIS;
        else m_uTextStyle &= ~DT_END_ELLIPSIS;
    }  
    else if( _tcscmp(pstrName, _T("multiline")) == 0 ) {
        if (_tcscmp(pstrValue, _T("true")) == 0) {
            m_uTextStyle  &= ~DT_SINGLELINE;
            m_uTextStyle |= DT_WORDBREAK;
        }
        else m_uTextStyle |= DT_SINGLELINE;
    }
    else if( _tcscmp(pstrName, _T("font")) == 0 ) SetFont(_ttoi(pstrValue));
    else if( _tcscmp(pstrName, _T("textcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetTextColor(clrColor);
    }
	else if( _tcscmp(pstrName, _T("textpadding")) == 0 ) {
		RECT rcTextPadding = { 0 };
		LPTSTR pstr = NULL;
		rcTextPadding.left = _tcstol(pstrValue, &pstr, 10);  ASSERT(pstr);    
		rcTextPadding.top = _tcstol(pstr + 1, &pstr, 10);    ASSERT(pstr);    
		rcTextPadding.right = _tcstol(pstr + 1, &pstr, 10);  ASSERT(pstr);    
		rcTextPadding.bottom = _tcstol(pstr + 1, &pstr, 10); ASSERT(pstr);    
		SetTextPadding(rcTextPadding);
	}
    else if( _tcscmp(pstrName, _T("showhtml")) == 0 ) SetShowHtml(_tcscmp(pstrValue, _T("true")) == 0);
    else if( _tcscmp(pstrName, _T("normalimage")) == 0 ) SetNormalImage(pstrValue);
    else if( _tcscmp(pstrName, _T("hotimage")) == 0 ) SetHotImage(pstrValue);
    else if( _tcscmp(pstrName, _T("pushedimage")) == 0 ) SetPushedImage(pstrValue);
    else if( _tcscmp(pstrName, _T("focusedimage")) == 0 ) SetFocusedImage(pstrValue);
    else if( _tcscmp(pstrName, _T("sepwidth")) == 0 ) SetSepWidth(_ttoi(pstrValue));
    else if( _tcscmp(pstrName, _T("sepcolor")) == 0 ) {
        if( *pstrValue == _T('#')) pstrValue = ::CharNext(pstrValue);
        LPTSTR pstr = NULL;
        DWORD clrColor = _tcstoul(pstrValue, &pstr, 16);
        SetSepColor(clrColor);
    }
    else if( _tcscmp(pstrName, _T("sepimage")) == 0 ) SetSepImage(pstrValue);
	else if (_tcscmp(pstrName, _T("ascimage")) == 0) SetSortAscImg(pstrValue);
	else if (_tcscmp(pstrName, _T("descimage")) == 0) SetSortDescImg(pstrValue);
	else if (_tcscmp(pstrName, _T("sort")) == 0) SetEnabledSort(_tcscmp(pstrValue, _T("true")) == 0);
	else if (_tcscmp(pstrName, _T("sortwidth")) == 0) SetSortWidth(_ttoi(pstrValue));
	else if (_tcscmp(pstrName, _T("sortheight")) == 0) SetSortHeight(_ttoi(pstrValue));
	else __super::SetAttribute(pstrName, pstrValue);
}

void CListHeaderItemUI::SetPos(RECT rc, bool bNeedInvalidate)
{
	{
		//#liulei 20161127 calc self min width for composite header
		int nSelfMinWidth = 0;
		CDuiPtrArray prtArySelf;
		GetInsideControl(prtArySelf, this, DUI_CTR_LISTHEADERITEM);
		for (int i = 0; i < prtArySelf.GetSize(); ++i)
		{
			CControlUI *pChild = static_cast<CControlUI *>(prtArySelf[i]);
			nSelfMinWidth += pChild->GetMinWidth();
		}
		nSelfMinWidth = max(nSelfMinWidth, GetMinWidth());
		rc.right = max(rc.right, rc.left + nSelfMinWidth);
	}

	CControlUI *pParent = GetParent();
	while (pParent)
	{
		if (pParent->GetInterface(DUI_CTR_LISTHEADERITEM))
		{
			//#liulei 20161127 calc the min item need width
			CDuiPtrArray prtAry;
			GetInsideControl(prtAry, GetParent(), DUI_CTR_LISTHEADERITEM);
			//> 计算剩剩余项的最小宽度
			int nMinWidth = 0;
			bool bAfterSelf = false;
			for (int i = 0; i < prtAry.GetSize();++i)
			{
				CControlUI *pChild = static_cast<CControlUI *>(prtAry[i]);
				if (pChild == this)
				{
					bAfterSelf = true;
					continue;
				}

				if (!bAfterSelf) continue;

				nMinWidth += pChild->GetMinWidth();
			}

			RECT rtParent = pParent->GetPos();
			rc.right = min(rc.right, rtParent.right - nMinWidth);
			rc.left = min(rc.left, rc.right - GetMinWidth());
			rc.left = max(rc.left, rtParent.left);
			break;
		}
		pParent = pParent->GetParent();
	}

	__super::SetPos(rc, bNeedInvalidate);
}

void CListHeaderItemUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pParent != NULL ) m_pParent->DoEvent(event);
		else __super::DoEvent(event);
        return;
    }

    if( event.Type == UIEVENT_SETFOCUS ) 
    {
        Invalidate();
    }
    if( event.Type == UIEVENT_KILLFOCUS ) 
    {
        Invalidate();
    }
	if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_DBLCLICK)
    {
        if( !IsEnabled() ) return;
        RECT rcSeparator = GetThumbRect();
        if( ::PtInRect(&rcSeparator, event.ptMouse) ) {
            if( m_bDragable ) {
                m_uButtonState |= UISTATE_CAPTURED;
                ptLastMouse = event.ptMouse;
            }
        }
        else {
			if (m_bEnablebSort)
			{
				SetSort((ESORT)((m_esrot + 1) % E_SORT_MAX));
			}

            m_uButtonState |= UISTATE_PUSHED;
            m_pManager->SendNotify(this, DUI_MSGTYPE_HEADERCLICK);
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_BUTTONUP )
    {
        if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
            m_uButtonState &= ~UISTATE_CAPTURED;
            if( GetParent() ) 
                GetParent()->NeedParentUpdate();
        }
        else if( (m_uButtonState & UISTATE_PUSHED) != 0 ) {
            m_uButtonState &= ~UISTATE_PUSHED;
            Invalidate();
        }
        return;
    }
    if( event.Type == UIEVENT_MOUSEMOVE )
    {
        if( (m_uButtonState & UISTATE_CAPTURED) != 0 ) {
            RECT rc = m_rcItem;
            if( m_iSepWidth >= 0 ) {
                rc.right -= ptLastMouse.x - event.ptMouse.x;
            }
            else {
                rc.left -= ptLastMouse.x - event.ptMouse.x;
            }
            
            if( rc.right - rc.left > GetMinWidth() ) {
                m_cxyFixed.cx = rc.right - rc.left;
                ptLastMouse = event.ptMouse;
				///> #liulei 为了实现多表头优化一下 20161117
				CControlUI *pParent = GetParent();
				///> 刷新listHeader的父容器
				if (pParent)
					pParent->NeedParentUpdate();

				///> 检测Panret是否为ListHeaderUI，如果是则刷新Header
				while (pParent && (pParent->GetInterface(DUI_CTR_LISTHEADER) == NULL))
				{
					pParent = pParent->GetParent();
				}
				if (pParent != NULL && pParent != GetParent())
					pParent->NeedParentUpdate();
            }
        }
        return;
    }
    if( event.Type == UIEVENT_SETCURSOR )
    {
        RECT rcSeparator = GetThumbRect();
        if( IsEnabled() && m_bDragable && ::PtInRect(&rcSeparator, event.ptMouse) ) {
            ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_SIZEWE)));
            return;
        }
    }
    if( event.Type == UIEVENT_MOUSEENTER )
    {
        if( ::PtInRect(&m_rcItem, event.ptMouse ) ) {
            if( IsEnabled() ) {
                if( (m_uButtonState & UISTATE_HOT) == 0  ) {
                    m_uButtonState |= UISTATE_HOT;
                    Invalidate();
                }
            }
        }
    }
    if( event.Type == UIEVENT_MOUSELEAVE )
    {
        if( !::PtInRect(&m_rcItem, event.ptMouse ) ) {
            if( IsEnabled() ) {
                if( (m_uButtonState & UISTATE_HOT) != 0  ) {
                    m_uButtonState &= ~UISTATE_HOT;
                    Invalidate();
                }
            }
            if (m_pManager) m_pManager->RemoveMouseLeaveNeeded(this);
        }
        else {
            if (m_pManager) m_pManager->AddMouseLeaveNeeded(this);
            return;
        }
    }
	__super::DoEvent(event);
}

SIZE CListHeaderItemUI::EstimateSize(SIZE szAvailable)
{
	///#liulei 20160613 修复表头不可见的时候 位置计算,修复水平滚动条位置计算错误
    if( m_cxyFixed.cy == 0 ) return CDuiSize(IsVisible() ? m_cxyFixed.cx:0, m_pManager->GetDefaultFontInfo()->tm.tmHeight + 8);
	return __super::EstimateSize(szAvailable);
}

RECT CListHeaderItemUI::GetThumbRect() const
{
    if( m_iSepWidth >= 0 ) return CDuiRect(m_rcItem.right - m_iSepWidth, m_rcItem.top, m_rcItem.right, m_rcItem.bottom);
    else return CDuiRect(m_rcItem.left, m_rcItem.top, m_rcItem.left - m_iSepWidth, m_rcItem.bottom);
}

RECT CListHeaderItemUI::GetSortRect() const
{
	return CDuiRect(
		m_nSortWidth > 0 ? m_rcItem.right - m_nSortWidth - 3 : m_rcItem.left - 3,
		m_nSortHeight > 0 ? (m_rcItem.top + (m_rcItem.bottom - m_rcItem.top - m_nSortHeight) / 2) : m_rcItem.top,
		m_nSortWidth > 0 ? m_rcItem.right - 3 : m_rcItem.left - 3,
		m_nSortHeight > 0 ? (m_rcItem.top + (m_rcItem.bottom - m_rcItem.top - m_nSortHeight) / 2 + m_nSortHeight):m_rcItem.top
		);
}

void CListHeaderItemUI::PaintStatusImage(HDC hDC)
{
	if( IsFocused() ) m_uButtonState |= UISTATE_FOCUSED;
	else m_uButtonState &= ~ UISTATE_FOCUSED;

	if( (m_uButtonState & UISTATE_PUSHED) != 0 ) {
		if( !DrawImage(hDC, m_diPushed) )  DrawImage(hDC, m_diNormal);
	}
	else if( (m_uButtonState & UISTATE_HOT) != 0 ) {
		if( !DrawImage(hDC, m_diHot) )  DrawImage(hDC, m_diNormal);
	}
	else if( (m_uButtonState & UISTATE_FOCUSED) != 0 ) {
		if( !DrawImage(hDC, m_diFocused) )  DrawImage(hDC, m_diNormal);
	}
	else {
		DrawImage(hDC, m_diNormal);
	}

	if (m_bEnablebSort)
	{
		if (m_esrot == E_SORT_ASC)
		{
			RECT rcThumb = GetSortRect();
			m_diAscSort.rcDestOffset.left = rcThumb.left - m_rcItem.left;
			m_diAscSort.rcDestOffset.top = rcThumb.top - m_rcItem.top;
			m_diAscSort.rcDestOffset.right = rcThumb.right - m_rcItem.left;
			m_diAscSort.rcDestOffset.bottom = rcThumb.bottom - m_rcItem.top;
			DrawImage(hDC, m_diAscSort);
		}
		else if (m_esrot == E_SORT_DESC)
		{
			RECT rcThumb = GetSortRect();
			m_diDescSort.rcDestOffset.left = rcThumb.left - m_rcItem.left;
			m_diDescSort.rcDestOffset.top = rcThumb.top - m_rcItem.top;
			m_diDescSort.rcDestOffset.right = rcThumb.right - m_rcItem.left;
			m_diDescSort.rcDestOffset.bottom = rcThumb.bottom - m_rcItem.top;
			DrawImage(hDC, m_diDescSort);
		}
	}

    if (m_iSepWidth > 0) {
        RECT rcThumb = GetThumbRect();
        m_diSep.rcDestOffset.left = rcThumb.left - m_rcItem.left;
        m_diSep.rcDestOffset.top = rcThumb.top - m_rcItem.top;
        m_diSep.rcDestOffset.right = rcThumb.right - m_rcItem.left;
        m_diSep.rcDestOffset.bottom = rcThumb.bottom - m_rcItem.top;
        if( !DrawImage(hDC, m_diSep) ) {
            if (m_dwSepColor != 0) {
                RECT rcSepLine = { rcThumb.left + m_iSepWidth/2, rcThumb.top, rcThumb.left + m_iSepWidth/2, rcThumb.bottom};
                CRenderEngine::DrawLine(hDC, rcSepLine, m_iSepWidth, GetAdjustColor(m_dwSepColor));
            }
        }
    }
}

void CListHeaderItemUI::PaintText(HDC hDC)
{
    if( m_dwTextColor == 0 ) m_dwTextColor = m_pManager->GetDefaultFontColor();

	RECT rcText = m_rcItem;
	rcText.left += m_rcTextPadding.left;
	rcText.top += m_rcTextPadding.top;
	rcText.right -= m_rcTextPadding.right;
	rcText.bottom -= m_rcTextPadding.bottom;

    if( m_sText.IsEmpty() ) return;
    int nLinks = 0;
    if( m_bShowHtml )
        CRenderEngine::DrawHtmlText(hDC, m_pManager, rcText, m_sText, m_dwTextColor, \
        NULL, NULL, nLinks, m_iFont, m_uTextStyle);
    else
        CRenderEngine::DrawText(hDC, m_pManager, rcText, m_sText, m_dwTextColor, \
        m_iFont, m_uTextStyle);
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListElementUI::CListElementUI() : 
m_iIndex(-1),
m_iDrawIndex(0),
m_pOwner(NULL), 
m_bSelected(false),
m_uButtonState(0)
{
}

LPCTSTR CListElementUI::GetClass() const
{
    return DUI_CTR_LISTELEMENT;
}

UINT CListElementUI::GetControlFlags() const
{
    return UIFLAG_WANTRETURN;
}

LPVOID CListElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcscmp(pstrName, DUI_CTR_ILISTITEM) == 0 ) return static_cast<IListItemUI*>(this);
	if( _tcscmp(pstrName, DUI_CTR_LISTELEMENT) == 0 ) return static_cast<CListElementUI*>(this);
    return CControlUI::GetInterface(pstrName);
}

IListOwnerUI* CListElementUI::GetOwner()
{
    return m_pOwner;
}

void CListElementUI::SetOwner(CControlUI* pOwner)
{
    if (pOwner != NULL) m_pOwner = static_cast<IListOwnerUI*>(pOwner->GetInterface(DUI_CTR_ILISTOWNER));
}

void CListElementUI::SetVisible(bool bVisible)
{
    CControlUI::SetVisible(bVisible);
    if( !IsVisible() && m_bSelected)
    {
        m_bSelected = false;
        if( m_pOwner != NULL ) m_pOwner->SelectItem(-1);
    }
}

void CListElementUI::SetEnabled(bool bEnable)
{
    CControlUI::SetEnabled(bEnable);
    if( !IsEnabled() ) {
        m_uButtonState = 0;
    }
}

int CListElementUI::GetIndex() const
{
    return m_iIndex;
}

void CListElementUI::SetIndex(int iIndex)
{
    m_iIndex = iIndex;
}

int CListElementUI::GetDrawIndex() const
{
    return m_iDrawIndex;
}

void CListElementUI::SetDrawIndex(int iIndex)
{
    m_iDrawIndex = iIndex;
}

void CListElementUI::Invalidate()
{
    if( !IsVisible() ) return;

    if( GetParent() ) {
        CContainerUI* pParentContainer = static_cast<CContainerUI*>(GetParent()->GetInterface(DUI_CTR_CONTAINER));
        if( pParentContainer ) {
            RECT rc = pParentContainer->GetPos();
            RECT rcInset = pParentContainer->GetInset();
            rc.left += rcInset.left;
            rc.top += rcInset.top;
            rc.right -= rcInset.right;
            rc.bottom -= rcInset.bottom;
            CScrollBarUI* pVerticalScrollBar = pParentContainer->GetVerticalScrollBar();
            if( pVerticalScrollBar && pVerticalScrollBar->IsVisible() ) rc.right -= pVerticalScrollBar->GetFixedWidth();
            CScrollBarUI* pHorizontalScrollBar = pParentContainer->GetHorizontalScrollBar();
            if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rc.bottom -= pHorizontalScrollBar->GetFixedHeight();

            RECT invalidateRc = m_rcItem;
            if( !::IntersectRect(&invalidateRc, &m_rcItem, &rc) ) 
            {
                return;
            }

            CControlUI* pParent = GetParent();
            RECT rcTemp;
            RECT rcParent;
            while( pParent = pParent->GetParent() )
            {
                rcTemp = invalidateRc;
                rcParent = pParent->GetPos();
                if( !::IntersectRect(&invalidateRc, &rcTemp, &rcParent) ) 
                {
                    return;
                }
            }

            if( m_pManager != NULL ) m_pManager->Invalidate(invalidateRc);
        }
        else {
            CControlUI::Invalidate();
        }
    }
    else {
        CControlUI::Invalidate();
    }
}

bool CListElementUI::Activate()
{
    if( !CControlUI::Activate() ) return false;
    if( m_pManager != NULL ) m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMACTIVATE);
    return true;
}

bool CListElementUI::IsSelected() const
{
    return m_bSelected;
}

bool CListElementUI::Select(bool bSelect, bool bTriggerEvent)
{
    if( !IsEnabled() ) return false;
    if( bSelect == m_bSelected ) return true;
    m_bSelected = bSelect;
    if( bSelect && m_pOwner != NULL ) m_pOwner->SelectItem(m_iIndex, bTriggerEvent);
    Invalidate();

    return true;
}

bool CListElementUI::IsExpanded() const
{
    return false;
}

bool CListElementUI::Expand(bool /*bExpand = true*/)
{
    return false;
}

void CListElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CControlUI::DoEvent(event);
        return;
    }

	if (event.Type == UIEVENT_DBLCLICK)
    {
        if( IsEnabled() )
		{
            Activate();
            Invalidate();
			//#liulei 增加双击事件 20160531
			m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMDBCLICK,0,0,true);
        }
        return;
    }
    if( event.Type == UIEVENT_KEYDOWN )
    {
        if (IsKeyboardEnabled() && IsEnabled()) {
            if( event.chKey == VK_RETURN ) {
                Activate();
                Invalidate();
                return;
            }
        }
    }
    // An important twist: The list-item will send the event not to its immediate
    // parent but to the "attached" list. A list may actually embed several components
    // in its path to the item, but key-presses etc. needs to go to the actual list.
    if( m_pOwner != NULL ) m_pOwner->DoEvent(event); else CControlUI::DoEvent(event);
}

void CListElementUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if( _tcscmp(pstrName, _T("selected")) == 0 ) Select();
    else CControlUI::SetAttribute(pstrName, pstrValue);
}

void CListElementUI::DrawItemBk(HDC hDC, const RECT& rcItem)
{
    ASSERT(m_pOwner);
    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if( pInfo == NULL ) return;
    DWORD iBackColor = 0;
    if( !pInfo->bAlternateBk || m_iDrawIndex % 2 == 0 ) iBackColor = pInfo->dwBkColor;
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iBackColor = pInfo->dwHotBkColor;
    }
    if( IsSelected() ) {
        iBackColor = pInfo->dwSelectedBkColor;
    }
    if( !IsEnabled() ) {
        iBackColor = pInfo->dwDisabledBkColor;
    }

    if ( iBackColor != 0 ) {
        CRenderEngine::DrawColor(hDC, rcItem, GetAdjustColor(iBackColor));
    }

    if( !IsEnabled() ) {
        if( DrawImage(hDC, pInfo->diDisabled) ) return;
    }
    if( IsSelected() ) {
        if( DrawImage(hDC, pInfo->diSelected) ) return;
    }
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        if( DrawImage(hDC, pInfo->diHot) ) return;
    }

    if( !DrawImage(hDC, m_diBk) ) {
        if( !pInfo->bAlternateBk || m_iDrawIndex % 2 == 0 ) {
            if( DrawImage(hDC, pInfo->diBk) ) return;
        }
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListLabelElementUI::CListLabelElementUI() : m_bNeedEstimateSize(true), m_uFixedHeightLast(0),
    m_nFontLast(-1), m_uTextStyleLast(0)
{
    m_szAvailableLast.cx = m_szAvailableLast.cy = 0;
    m_cxyFixedLast.cx = m_cxyFixedLast.cy = 0;
    ::ZeroMemory(&m_rcTextPaddingLast, sizeof(m_rcTextPaddingLast));
}

LPCTSTR CListLabelElementUI::GetClass() const
{
    return DUI_CTR_LISTLABELELEMENT;
}

LPVOID CListLabelElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcscmp(pstrName, DUI_CTR_LISTLABELELEMENT) == 0 ) return static_cast<CListLabelElementUI*>(this);
    return CListElementUI::GetInterface(pstrName);
}

void CListLabelElementUI::SetOwner(CControlUI* pOwner)
{
    m_bNeedEstimateSize = true;
    CListElementUI::SetOwner(pOwner);
}

void CListLabelElementUI::SetFixedWidth(int cx)
{
    m_bNeedEstimateSize = true;
    CControlUI::SetFixedWidth(cx);
}

void CListLabelElementUI::SetFixedHeight(int cy)
{
    m_bNeedEstimateSize = true;
    CControlUI::SetFixedHeight(cy);
}

void CListLabelElementUI::SetText(LPCTSTR pstrText)
{
    m_bNeedEstimateSize = true;
    CControlUI::SetText(pstrText);
}

void CListLabelElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CListElementUI::DoEvent(event);
        return;
    }

	if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN)
    {
        if( IsEnabled() ) {
            m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMCLICK);
            Select();
            Invalidate();
        }
		if (m_pOwner != NULL) m_pOwner->DoEvent(event);
        return;
    }
    if( event.Type == UIEVENT_MOUSEMOVE ) 
    {
        return;
    }
    if( event.Type == UIEVENT_BUTTONUP )
    {
        return;
    }
    if( event.Type == UIEVENT_MOUSEENTER )
    {
        if( ::PtInRect(&m_rcItem, event.ptMouse ) ) {
            if( IsEnabled() ) {
                if( (m_uButtonState & UISTATE_HOT) == 0  ) {
                    m_uButtonState |= UISTATE_HOT;
                    Invalidate();
                }
            }
        }
    }
    if( event.Type == UIEVENT_MOUSELEAVE )
    {
        if( !::PtInRect(&m_rcItem, event.ptMouse ) ) {
            if( IsEnabled() ) {
                if( (m_uButtonState & UISTATE_HOT) != 0  ) {
                    m_uButtonState &= ~UISTATE_HOT;
                    Invalidate();
                }
            }
            if (m_pManager) m_pManager->RemoveMouseLeaveNeeded(this);
        }
        else {
            if (m_pManager) m_pManager->AddMouseLeaveNeeded(this);
            return;
        }
    }
    CListElementUI::DoEvent(event);
}

SIZE CListLabelElementUI::EstimateSize(SIZE szAvailable)
{
    if( m_pOwner == NULL ) return CDuiSize(0, 0);
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if (pInfo == NULL) return CDuiSize(0, 0);
    if (m_cxyFixed.cx > 0) {
        if (m_cxyFixed.cy > 0) return m_cxyFixed;
        else if (pInfo->uFixedHeight > 0) return CDuiSize(m_cxyFixed.cx, pInfo->uFixedHeight);
    }

    if ((pInfo->uTextStyle & DT_SINGLELINE) == 0 && 
        (szAvailable.cx != m_szAvailableLast.cx || szAvailable.cy != m_szAvailableLast.cy)) {
            m_bNeedEstimateSize = true;
    }
    if (m_uFixedHeightLast != pInfo->uFixedHeight || m_nFontLast != pInfo->nFont ||
        m_uTextStyleLast != pInfo->uTextStyle ||
        m_rcTextPaddingLast.left != pInfo->rcTextPadding.left || m_rcTextPaddingLast.right != pInfo->rcTextPadding.right ||
        m_rcTextPaddingLast.top != pInfo->rcTextPadding.top || m_rcTextPaddingLast.bottom != pInfo->rcTextPadding.bottom) {
            m_bNeedEstimateSize = true;
    }

    if (m_bNeedEstimateSize) {
        m_bNeedEstimateSize = false;
        m_szAvailableLast = szAvailable;
        m_uFixedHeightLast = pInfo->uFixedHeight;
        m_nFontLast = pInfo->nFont;
        m_uTextStyleLast = pInfo->uTextStyle;
        m_rcTextPaddingLast = pInfo->rcTextPadding;
        
        m_cxyFixedLast = m_cxyFixed;
        if (m_cxyFixedLast.cy == 0) {
            m_cxyFixedLast.cy = pInfo->uFixedHeight;
        }

        if ((pInfo->uTextStyle & DT_SINGLELINE) != 0) {
            if( m_cxyFixedLast.cy == 0 ) {
                m_cxyFixedLast.cy = m_pManager->GetFontInfo(pInfo->nFont)->tm.tmHeight + 8;
                m_cxyFixedLast.cy += pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
            }
            if (m_cxyFixedLast.cx == 0) {
                RECT rcText = { 0, 0, 9999, m_cxyFixedLast.cy };
                if( pInfo->bShowHtml ) {
                    int nLinks = 0;
                    CRenderEngine::DrawHtmlText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, 0, NULL, NULL, nLinks, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
                }
                else {
                    CRenderEngine::DrawText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, 0, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
                }
                m_cxyFixedLast.cx = rcText.right - rcText.left + pInfo->rcTextPadding.left + pInfo->rcTextPadding.right;
            }
        }
        else {
            if( m_cxyFixedLast.cx == 0 ) {
                m_cxyFixedLast.cx = szAvailable.cx;
            }
            RECT rcText = { 0, 0, m_cxyFixedLast.cx, 9999 };
            rcText.left += pInfo->rcTextPadding.left;
            rcText.right -= pInfo->rcTextPadding.right;
            if( pInfo->bShowHtml ) {
                int nLinks = 0;
                CRenderEngine::DrawHtmlText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, 0, NULL, NULL, nLinks, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
            }
            else {
                CRenderEngine::DrawText(m_pManager->GetPaintDC(), m_pManager, rcText, m_sText, 0, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
            }
            m_cxyFixedLast.cy = rcText.bottom - rcText.top + pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
        }
    }
    return m_cxyFixedLast;
}

bool CListLabelElementUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
    DrawItemBk(hDC, m_rcItem);
    DrawItemText(hDC, m_rcItem);
    return true;
}

void CListLabelElementUI::DrawItemText(HDC hDC, const RECT& rcItem)
{
    if( m_sText.IsEmpty() ) return;

    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if( pInfo == NULL ) return;
    DWORD iTextColor = pInfo->dwTextColor;
    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iTextColor = pInfo->dwHotTextColor;
    }
    if( IsSelected() ) {
        iTextColor = pInfo->dwSelectedTextColor;
    }
    if( !IsEnabled() ) {
        iTextColor = pInfo->dwDisabledTextColor;
    }
    int nLinks = 0;
    RECT rcText = rcItem;
    rcText.left += pInfo->rcTextPadding.left;
    rcText.right -= pInfo->rcTextPadding.right;
    rcText.top += pInfo->rcTextPadding.top;
    rcText.bottom -= pInfo->rcTextPadding.bottom;

    if( pInfo->bShowHtml )
        CRenderEngine::DrawHtmlText(hDC, m_pManager, rcText, m_sText, iTextColor, \
        NULL, NULL, nLinks, pInfo->nFont, pInfo->uTextStyle);
    else
        CRenderEngine::DrawText(hDC, m_pManager, rcText, m_sText, iTextColor, \
        pInfo->nFont, pInfo->uTextStyle);
}


/////////////////////////////////////////////////////////////////////////////////////
//
//

CListTextElementUI::CListTextElementUI() : m_nLinks(0), m_nHoverLink(-1), m_pOwner(NULL)
{
	::ZeroMemory(m_uTextsStyle,sizeof(m_uTextsStyle));
    ::ZeroMemory(m_rcLinks, sizeof(m_rcLinks));
}

CListTextElementUI::~CListTextElementUI()
{
    CDuiString* pText;
    for( int it = 0; it < m_aTexts.GetSize(); it++ ) {
        pText = static_cast<CDuiString*>(m_aTexts[it]);
        if( pText ) delete pText;
    }
    m_aTexts.Empty();	
}

LPCTSTR CListTextElementUI::GetClass() const
{
    return DUI_CTR_LISTTEXTELEMENT;
}

LPVOID CListTextElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcscmp(pstrName, DUI_CTR_LISTTEXTELEMENT) == 0 ) return static_cast<CListTextElementUI*>(this);
    return CListLabelElementUI::GetInterface(pstrName);
}

UINT CListTextElementUI::GetControlFlags() const
{
    return UIFLAG_WANTRETURN | ( (IsEnabled() && m_nLinks > 0) ? UIFLAG_SETCURSOR : 0);
}

int CListTextElementUI::GetCount() const
{
	return m_aTexts.GetSize();
}

LPCTSTR CListTextElementUI::GetText(int iIndex) const
{
    CDuiString* pText = static_cast<CDuiString*>(m_aTexts.GetAt(iIndex));
    if( pText ) return pText->GetData();
    return NULL;
}

void CListTextElementUI::SetText(int iIndex, LPCTSTR pstrText, UINT uTextStyle)
{
    //if( m_pOwner == NULL ) return;
   // TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if( iIndex < 0 /*|| iIndex >= pInfo->nColumns*/ ) return;
    m_bNeedEstimateSize = true;
	if (uTextStyle > 0 && iIndex >= 0 && iIndex < UILIST_MAX_COLUMNS) m_uTextsStyle[iIndex] = uTextStyle;
	while (m_aTexts.GetSize() <= iIndex/*pInfo->nColumns*/) { m_aTexts.Add(NULL); }

    CDuiString* pText = static_cast<CDuiString*>(m_aTexts[iIndex]);
    if( (pText == NULL && pstrText == NULL) || (pText && *pText == pstrText) ) return;

	if ( pText ) //by cddjr 2011/10/20
		pText->Assign(pstrText);
	else
		m_aTexts.SetAt(iIndex, new CDuiString(pstrText));
    Invalidate();
}

void CListTextElementUI::SetOwner(CControlUI* pOwner)
{
    if (pOwner != NULL) {
        m_bNeedEstimateSize = true;
        CListElementUI::SetOwner(pOwner);
        m_pOwner = static_cast<IListUI*>(pOwner->GetInterface(DUI_CTR_ILIST));
    }
}

CDuiString* CListTextElementUI::GetLinkContent(int iIndex)
{
    if( iIndex >= 0 && iIndex < m_nLinks ) return &m_sLinks[iIndex];
    return NULL;
}

void CListTextElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CListLabelElementUI::DoEvent(event);
        return;
    }

    // When you hover over a link
    if( event.Type == UIEVENT_SETCURSOR ) {
        for( int i = 0; i < m_nLinks; i++ ) {
            if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                ::SetCursor(::LoadCursor(NULL, MAKEINTRESOURCE(IDC_HAND)));
                return;
            }
        }      
    }
    if( event.Type == UIEVENT_BUTTONUP && IsEnabled() ) {
        for( int i = 0; i < m_nLinks; i++ ) {
            if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                m_pManager->SendNotify(this, DUI_MSGTYPE_LINK, i);
                return;
            }
        }
    }
    if( m_nLinks > 0 && event.Type == UIEVENT_MOUSEMOVE ) {
        int nHoverLink = -1;
        for( int i = 0; i < m_nLinks; i++ ) {
            if( ::PtInRect(&m_rcLinks[i], event.ptMouse) ) {
                nHoverLink = i;
                break;
            }
        }

        if(m_nHoverLink != nHoverLink) {
            Invalidate();
            m_nHoverLink = nHoverLink;
        }
    }
    if( m_nLinks > 0 && event.Type == UIEVENT_MOUSELEAVE ) {
        if(m_nHoverLink != -1) {
            if( !::PtInRect(&m_rcLinks[m_nHoverLink], event.ptMouse) ) {
                m_nHoverLink = -1;
                Invalidate();
                if (m_pManager) m_pManager->RemoveMouseLeaveNeeded(this);
            }
            else {
                if (m_pManager) m_pManager->AddMouseLeaveNeeded(this);
                return;
            }
        }
    }
    CListLabelElementUI::DoEvent(event);
}

SIZE CListTextElementUI::EstimateSize(SIZE szAvailable)
{
    if( m_pOwner == NULL ) return CDuiSize(0, 0);
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if (pInfo == NULL) return CDuiSize(0, 0);
    SIZE cxyFixed = m_cxyFixed;
    if (cxyFixed.cx == 0 && pInfo->nColumns > 0) {
        cxyFixed.cx = pInfo->rcColumn[pInfo->nColumns - 1].right - pInfo->rcColumn[0].left;
        if (m_cxyFixedLast.cx != cxyFixed.cx) m_bNeedEstimateSize = true;
    }
    if (cxyFixed.cx > 0) {
        if (cxyFixed.cy > 0) return cxyFixed;
        else if (pInfo->uFixedHeight > 0) return CDuiSize(cxyFixed.cx, pInfo->uFixedHeight);
    }
    
    if ((pInfo->uTextStyle & DT_SINGLELINE) == 0 && 
        (szAvailable.cx != m_szAvailableLast.cx || szAvailable.cy != m_szAvailableLast.cy)) {
            m_bNeedEstimateSize = true;
    }
    if (m_uFixedHeightLast != pInfo->uFixedHeight || m_nFontLast != pInfo->nFont ||
        m_uTextStyleLast != pInfo->uTextStyle ||
        m_rcTextPaddingLast.left != pInfo->rcTextPadding.left || m_rcTextPaddingLast.right != pInfo->rcTextPadding.right ||
        m_rcTextPaddingLast.top != pInfo->rcTextPadding.top || m_rcTextPaddingLast.bottom != pInfo->rcTextPadding.bottom) {
            m_bNeedEstimateSize = true;
    }

    CDuiString strText;
    IListCallbackUI* pCallback = m_pOwner->GetTextCallback();
    if( pCallback ) strText = pCallback->GetItemText(this, m_iIndex, 0);
    else if (m_aTexts.GetSize() > 0) strText.Assign(GetText(0));
    else strText = m_sText;
    if (m_sTextLast != strText) m_bNeedEstimateSize = true;

    if (m_bNeedEstimateSize) {
        m_bNeedEstimateSize = false;
        m_szAvailableLast = szAvailable;
        m_uFixedHeightLast = pInfo->uFixedHeight;
        m_nFontLast = pInfo->nFont;
        m_uTextStyleLast = pInfo->uTextStyle;
        m_rcTextPaddingLast = pInfo->rcTextPadding;
        m_sTextLast = strText;

        m_cxyFixedLast = m_cxyFixed;
        if (m_cxyFixedLast.cx == 0 && pInfo->nColumns > 0) {
            m_cxyFixedLast.cx = pInfo->rcColumn[pInfo->nColumns - 1].right - pInfo->rcColumn[0].left;
        }
        if (m_cxyFixedLast.cy == 0) {
            m_cxyFixedLast.cy = pInfo->uFixedHeight;
        }

        if ((pInfo->uTextStyle & DT_SINGLELINE) != 0) {
            if( m_cxyFixedLast.cy == 0 ) {
                m_cxyFixedLast.cy = m_pManager->GetFontInfo(pInfo->nFont)->tm.tmHeight + 8;
                m_cxyFixedLast.cy += pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
            }
            if (m_cxyFixedLast.cx == 0) {
                RECT rcText = { 0, 0, 9999, m_cxyFixedLast.cy };
                if( pInfo->bShowHtml ) {
                    int nLinks = 0;
                    CRenderEngine::DrawHtmlText(m_pManager->GetPaintDC(), m_pManager, rcText, strText, 0, NULL, NULL, nLinks, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
                }
                else {
                    CRenderEngine::DrawText(m_pManager->GetPaintDC(), m_pManager, rcText, strText, 0, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
                }
                m_cxyFixedLast.cx = rcText.right - rcText.left + pInfo->rcTextPadding.left + pInfo->rcTextPadding.right;
            }
        }
        else {
            if( m_cxyFixedLast.cx == 0 ) {
                m_cxyFixedLast.cx = szAvailable.cx;
            }
            RECT rcText = { 0, 0, m_cxyFixedLast.cx, 9999 };
            rcText.left += pInfo->rcTextPadding.left;
            rcText.right -= pInfo->rcTextPadding.right;
            if( pInfo->bShowHtml ) {
                int nLinks = 0;
                CRenderEngine::DrawHtmlText(m_pManager->GetPaintDC(), m_pManager, rcText, strText, 0, NULL, NULL, nLinks, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
            }
            else {
                CRenderEngine::DrawText(m_pManager->GetPaintDC(), m_pManager, rcText, strText, 0, pInfo->nFont, DT_CALCRECT | pInfo->uTextStyle & ~DT_RIGHT & ~DT_CENTER);
            }
            m_cxyFixedLast.cy = rcText.bottom - rcText.top + pInfo->rcTextPadding.top + pInfo->rcTextPadding.bottom;
        }
    }
    return m_cxyFixedLast;
}

void CListTextElementUI::DrawItemText(HDC hDC, const RECT& rcItem)
{
    if( m_pOwner == NULL ) return;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if (pInfo == NULL) return;
    DWORD iTextColor = pInfo->dwTextColor;

    if( (m_uButtonState & UISTATE_HOT) != 0 ) {
        iTextColor = pInfo->dwHotTextColor;
    }
    if( IsSelected() ) {
        iTextColor = pInfo->dwSelectedTextColor;
    }
    if( !IsEnabled() ) {
        iTextColor = pInfo->dwDisabledTextColor;
    }
    IListCallbackUI* pCallback = m_pOwner->GetTextCallback();
	CListHeaderUI*	pHeader = m_pOwner->GetHeader();
    m_nLinks = 0;
    int nLinks = lengthof(m_rcLinks);
    if (pInfo->nColumns > 0) {
        for( int i = 0; i < pInfo->nColumns; i++ )
        {
            RECT rcItem = { pInfo->rcColumn[i].left, m_rcItem.top, pInfo->rcColumn[i].right, m_rcItem.bottom };
			//#liulei 如果表头项不可见则item 宽度置为0,当前列内容不绘画
			if (pHeader&&!pHeader->GetItemAt(i)->IsVisible())
			{
				rcItem.right = rcItem.left;
				continue;
			}

            if (pInfo->iVLineSize > 0 && i < pInfo->nColumns - 1) {
                RECT rcLine = { rcItem.right - pInfo->iVLineSize / 2, rcItem.top, rcItem.right - pInfo->iVLineSize / 2, rcItem.bottom};
                CRenderEngine::DrawLine(hDC, rcLine, pInfo->iVLineSize, GetAdjustColor(pInfo->dwVLineColor));
                rcItem.right -= pInfo->iVLineSize;
            }

            rcItem.left += pInfo->rcTextPadding.left;
            rcItem.right -= pInfo->rcTextPadding.right;
            rcItem.top += pInfo->rcTextPadding.top;
            rcItem.bottom -= pInfo->rcTextPadding.bottom;

            CDuiString strText;//不使用LPCTSTR，否则限制太多 by cddjr 2011/10/20
            if( pCallback ) strText = pCallback->GetItemText(this, m_iIndex, i);
            else strText.Assign(GetText(i));
            if( pInfo->bShowHtml )
                CRenderEngine::DrawHtmlText(hDC, m_pManager, rcItem, strText.GetData(), iTextColor, \
				&m_rcLinks[m_nLinks], &m_sLinks[m_nLinks], nLinks, pInfo->nFont, m_uTextsStyle[i] == 0 ? pInfo->uTextStyle : m_uTextsStyle[i]);
            else
                CRenderEngine::DrawText(hDC, m_pManager, rcItem, strText.GetData(), iTextColor, \
				pInfo->nFont, m_uTextsStyle[i] == 0 ? pInfo->uTextStyle : m_uTextsStyle[i]);

            m_nLinks += nLinks;
            nLinks = lengthof(m_rcLinks) - m_nLinks; 
        }
    }
    else {
        RECT rcItem = m_rcItem;
        rcItem.left += pInfo->rcTextPadding.left;
        rcItem.right -= pInfo->rcTextPadding.right;
        rcItem.top += pInfo->rcTextPadding.top;
        rcItem.bottom -= pInfo->rcTextPadding.bottom;

        CDuiString strText;
        if( pCallback ) strText = pCallback->GetItemText(this, m_iIndex, 0);
        else if (m_aTexts.GetSize() > 0) strText.Assign(GetText(0));
        else strText = m_sText;
        if( pInfo->bShowHtml )
            CRenderEngine::DrawHtmlText(hDC, m_pManager, rcItem, strText.GetData(), iTextColor, \
			&m_rcLinks[m_nLinks], &m_sLinks[m_nLinks], nLinks, pInfo->nFont, pInfo->uTextStyle);
        else
            CRenderEngine::DrawText(hDC, m_pManager, rcItem, strText.GetData(), iTextColor, \
			pInfo->nFont,pInfo->uTextStyle);

        m_nLinks += nLinks;
        nLinks = lengthof(m_rcLinks) - m_nLinks; 
    }

    for( int i = m_nLinks; i < lengthof(m_rcLinks); i++ ) {
        ::ZeroMemory(m_rcLinks + i, sizeof(RECT));
        ((CDuiString*)(m_sLinks + i))->Empty();
    }
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListContainerElementUI::CListContainerElementUI() : 
m_iIndex(-1),
m_iDrawIndex(0),
m_pOwner(NULL), 
m_bSelected(false),
m_bExpandable(false),
m_bExpand(false),
m_uButtonState(0)
{
}

LPCTSTR CListContainerElementUI::GetClass() const
{
    return DUI_CTR_LISTCONTAINERELEMENT;
}

UINT CListContainerElementUI::GetControlFlags() const
{
    return UIFLAG_WANTRETURN;
}

LPVOID CListContainerElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcscmp(pstrName, DUI_CTR_ILISTITEM) == 0 ) return static_cast<IListItemUI*>(this);
	if( _tcscmp(pstrName, DUI_CTR_LISTCONTAINERELEMENT) == 0 ) return static_cast<CListContainerElementUI*>(this);
    return CContainerUI::GetInterface(pstrName);
}

IListOwnerUI* CListContainerElementUI::GetOwner()
{
    return m_pOwner;
}

void CListContainerElementUI::SetOwner(CControlUI* pOwner)
{
    if (pOwner != NULL) m_pOwner = static_cast<IListOwnerUI*>(pOwner->GetInterface(DUI_CTR_ILISTOWNER));
}

void CListContainerElementUI::SetVisible(bool bVisible)
{
    CContainerUI::SetVisible(bVisible);
    if( !IsVisible() && m_bSelected)
    {
        m_bSelected = false;
        if( m_pOwner != NULL ) m_pOwner->SelectItem(-1);
    }
}

void CListContainerElementUI::SetEnabled(bool bEnable)
{
    CControlUI::SetEnabled(bEnable);
    if( !IsEnabled() ) {
        m_uButtonState = 0;
    }
}

int CListContainerElementUI::GetIndex() const
{
    return m_iIndex;
}

void CListContainerElementUI::SetIndex(int iIndex)
{
    m_iIndex = iIndex;
}

int CListContainerElementUI::GetDrawIndex() const
{
    return m_iDrawIndex;
}

void CListContainerElementUI::SetDrawIndex(int iIndex)
{
    m_iDrawIndex = iIndex;
}

void CListContainerElementUI::Invalidate()
{
    if( !IsVisible() ) return;

    if( GetParent() ) {
        CContainerUI* pParentContainer = static_cast<CContainerUI*>(GetParent()->GetInterface(DUI_CTR_CONTAINER));
        if( pParentContainer ) {
            RECT rc = pParentContainer->GetPos();
            RECT rcInset = pParentContainer->GetInset();
            rc.left += rcInset.left;
            rc.top += rcInset.top;
            rc.right -= rcInset.right;
            rc.bottom -= rcInset.bottom;
            CScrollBarUI* pVerticalScrollBar = pParentContainer->GetVerticalScrollBar();
            if( pVerticalScrollBar && pVerticalScrollBar->IsVisible() ) rc.right -= pVerticalScrollBar->GetFixedWidth();
            CScrollBarUI* pHorizontalScrollBar = pParentContainer->GetHorizontalScrollBar();
            if( pHorizontalScrollBar && pHorizontalScrollBar->IsVisible() ) rc.bottom -= pHorizontalScrollBar->GetFixedHeight();

            RECT invalidateRc = m_rcItem;
            if( !::IntersectRect(&invalidateRc, &m_rcItem, &rc) ) 
            {
                return;
            }

            CControlUI* pParent = GetParent();
            RECT rcTemp;
            RECT rcParent;
            while( pParent = pParent->GetParent() )
            {
                rcTemp = invalidateRc;
                rcParent = pParent->GetPos();
                if( !::IntersectRect(&invalidateRc, &rcTemp, &rcParent) ) 
                {
                    return;
                }
            }

            if( m_pManager != NULL ) m_pManager->Invalidate(invalidateRc);
        }
        else {
            CContainerUI::Invalidate();
        }
    }
    else {
        CContainerUI::Invalidate();
    }
}

bool CListContainerElementUI::Activate()
{
    if( !CContainerUI::Activate() ) return false;
    if( m_pManager != NULL ) m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMACTIVATE);
    return true;
}

bool CListContainerElementUI::IsSelected() const
{
    return m_bSelected;
}

bool CListContainerElementUI::Select(bool bSelect, bool bTriggerEvent)
{
    if( !IsEnabled() ) return false;
    if( bSelect == m_bSelected ) return true;
    m_bSelected = bSelect;
    if( bSelect && m_pOwner != NULL ) m_pOwner->SelectItem(m_iIndex, bTriggerEvent);
    Invalidate();

    return true;
}

bool CListContainerElementUI::IsExpandable() const
{
    return m_bExpandable;
}

void CListContainerElementUI::SetExpandable(bool bExpandable)
{
    m_bExpandable = bExpandable;
}

bool CListContainerElementUI::IsExpanded() const
{
    return m_bExpand;
}

bool CListContainerElementUI::Expand(bool bExpand)
{
    ASSERT(m_pOwner);
    if( m_pOwner == NULL ) return false;  
    if( bExpand == m_bExpand ) return true;
    m_bExpand = bExpand;
    if( m_bExpandable ) {
        if( !m_pOwner->ExpandItem(m_iIndex, bExpand) ) return false;
        if( m_pManager != NULL ) {
            if( bExpand ) m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMEXPAND, false);
            else m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMCOLLAPSE, false);
        }
    }

    return true;
}

void CListContainerElementUI::DoEvent(TEventUI& event)
{
    if( !IsMouseEnabled() && event.Type > UIEVENT__MOUSEBEGIN && event.Type < UIEVENT__MOUSEEND ) {
        if( m_pOwner != NULL ) m_pOwner->DoEvent(event);
        else CContainerUI::DoEvent(event);
        return;
    }

	if (event.Type == UIEVENT_DBLCLICK)
    {
        if( IsEnabled() )
		{
            Activate();
            Invalidate();
			//#liulei 增加双击事件 20160531
			m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMDBCLICK,0,0,true);
        }
        return;
    }
    if( event.Type == UIEVENT_KEYDOWN )
    {
        if (IsKeyboardEnabled() && IsEnabled()) {
            if( event.chKey == VK_RETURN ) {
                Activate();
                Invalidate();
                return;
            }
        }
    }
	if (event.Type == UIEVENT_BUTTONDOWN || event.Type == UIEVENT_RBUTTONDOWN)
    {
        if( IsEnabled() ) {
            m_pManager->SendNotify(this, DUI_MSGTYPE_ITEMCLICK);
            Select();
            Invalidate();
        }
		if (m_pOwner != NULL) m_pOwner->DoEvent(event);
        return;
    }
    if( event.Type == UIEVENT_BUTTONUP ) 
    {
        return;
    }
    if( event.Type == UIEVENT_MOUSEMOVE )
    {
        return;
    }
    if( event.Type == UIEVENT_MOUSEENTER )
    {
        if( ::PtInRect(&m_rcItem, event.ptMouse ) ) {
            if( IsEnabled() ) {
                if( (m_uButtonState & UISTATE_HOT) == 0  ) {
                    m_uButtonState |= UISTATE_HOT;
                    Invalidate();
                }
            }
        }
    }
    if( event.Type == UIEVENT_MOUSELEAVE )
    {
        if( !::PtInRect(&m_rcItem, event.ptMouse ) ) {
            if( IsEnabled() ) {
                if( (m_uButtonState & UISTATE_HOT) != 0  ) {
                    m_uButtonState &= ~UISTATE_HOT;
                    Invalidate();
                }
            }
            if (m_pManager) m_pManager->RemoveMouseLeaveNeeded(this);
        }
        else {
            if (m_pManager) m_pManager->AddMouseLeaveNeeded(this);
            return;
        }
    }

    // An important twist: The list-item will send the event not to its immediate
    // parent but to the "attached" list. A list may actually embed several components
    // in its path to the item, but key-presses etc. needs to go to the actual list.
    if( m_pOwner != NULL ) m_pOwner->DoEvent(event); else CControlUI::DoEvent(event);
}

void CListContainerElementUI::SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue)
{
    if( _tcscmp(pstrName, _T("selected")) == 0 ) Select();
    else if( _tcscmp(pstrName, _T("expandable")) == 0 ) SetExpandable(_tcscmp(pstrValue, _T("true")) == 0);
    else CContainerUI::SetAttribute(pstrName, pstrValue);
}

bool CListContainerElementUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
    DrawItemBk(hDC, m_rcItem);
    return CContainerUI::DoPaint(hDC, rcPaint, pStopControl);
}

void CListContainerElementUI::DrawItemText(HDC hDC, const RECT& rcItem)
{
    return;
}

void CListContainerElementUI::DrawItemBk(HDC hDC, const RECT& rcItem)
{
	ASSERT(m_pOwner);
	if( m_pOwner == NULL ) return;
	TListInfoUI* pInfo = m_pOwner->GetListInfo();
	if( pInfo == NULL ) return;
	DWORD iBackColor = 0;
	if( !pInfo->bAlternateBk || m_iDrawIndex % 2 == 0 ) iBackColor = pInfo->dwBkColor;

	if( (m_uButtonState & UISTATE_HOT) != 0 ) {
		iBackColor = pInfo->dwHotBkColor;
	}
	if( IsSelected() ) {
		iBackColor = pInfo->dwSelectedBkColor;
	}
	if( !IsEnabled() ) {
		iBackColor = pInfo->dwDisabledBkColor;
	}
	if ( iBackColor != 0 ) {
		CRenderEngine::DrawColor(hDC, m_rcItem, GetAdjustColor(iBackColor));
	}

	if( !IsEnabled() ) {
		if( DrawImage(hDC, pInfo->diDisabled) ) return;
	}
	if( IsSelected() ) {
		if( DrawImage(hDC, pInfo->diSelected) ) return;
	}
	if( (m_uButtonState & UISTATE_HOT) != 0 ) {
		if( DrawImage(hDC, pInfo->diHot) ) return;
	}
	if( !DrawImage(hDC, m_diBk) ) {
		if( !pInfo->bAlternateBk || m_iDrawIndex % 2 == 0 ) {
			if( DrawImage(hDC, pInfo->diBk) ) return;
		}
	}
}

SIZE CListContainerElementUI::EstimateSize(SIZE szAvailable)
{
    TListInfoUI* pInfo = NULL;
    if( m_pOwner ) pInfo = m_pOwner->GetListInfo();

    SIZE cXY = m_cxyFixed;

    if( cXY.cy == 0 ) {
        cXY.cy = pInfo->uFixedHeight;
    }

    return cXY;
}

/////////////////////////////////////////////////////////////////////////////////////
//
//

CListHBoxElementUI::CListHBoxElementUI()
{
    
}

LPCTSTR CListHBoxElementUI::GetClass() const
{
    return DUI_CTR_LISTHBOXELEMENT;
}

LPVOID CListHBoxElementUI::GetInterface(LPCTSTR pstrName)
{
    if( _tcscmp(pstrName, DUI_CTR_LISTHBOXELEMENT) == 0 ) return static_cast<CListHBoxElementUI*>(this);
    return CListContainerElementUI::GetInterface(pstrName);
}

void CListHBoxElementUI::SetPos(RECT rc, bool bNeedInvalidate)
{
    if( m_pOwner == NULL ) return CListContainerElementUI::SetPos(rc, bNeedInvalidate);

    CControlUI::SetPos(rc, bNeedInvalidate);
    rc = m_rcItem;

    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if (pInfo == NULL) return;
    if (pInfo->nColumns > 0) {
        int iColumnIndex = 0;
        for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
            CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
            if( !pControl->IsVisible() ) continue;
            if( pControl->IsFloat() ) {
                SetFloatPos(it2);
                continue;
            }

			//>#调整复合表头对应的listbody的区域
			while (iColumnIndex < pInfo->nColumns && pInfo->bUsedHeaderContain[iColumnIndex])
				++iColumnIndex;

            if( iColumnIndex >= pInfo->nColumns ) continue;

            RECT rcPadding = pControl->GetPadding();
            RECT rcItem = { pInfo->rcColumn[iColumnIndex].left + rcPadding.left, m_rcItem.top + rcPadding.top, 
                pInfo->rcColumn[iColumnIndex].right - rcPadding.right, m_rcItem.bottom - rcPadding.bottom };
            if (pInfo->iVLineSize > 0 && iColumnIndex < pInfo->nColumns - 1) {
                rcItem.right -= pInfo->iVLineSize;
            }
            pControl->SetPos(rcItem, false);
            iColumnIndex += 1;
        }
    }
    else {
        for( int it2 = 0; it2 < m_items.GetSize(); it2++ ) {
            CControlUI* pControl = static_cast<CControlUI*>(m_items[it2]);
            if( !pControl->IsVisible() ) continue;
            if( pControl->IsFloat() ) {
                SetFloatPos(it2);
                continue;
            }

            RECT rcPadding = pControl->GetPadding();
            RECT rcItem = { m_rcItem.left + rcPadding.left, m_rcItem.top + rcPadding.top, 
                m_rcItem.right - rcPadding.right, m_rcItem.bottom - rcPadding.bottom };
            pControl->SetPos(rcItem, false);
        }
    }
}

bool CListHBoxElementUI::DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl)
{
   // ASSERT(m_pOwner);
    if( m_pOwner == NULL ) return true;
    TListInfoUI* pInfo = m_pOwner->GetListInfo();
    if( pInfo == NULL ) return true;

    DrawItemBk(hDC, m_rcItem);
    for( int i = 0; i < pInfo->nColumns; i++ ) {
        RECT rcItem = { pInfo->rcColumn[i].left, m_rcItem.top, pInfo->rcColumn[i].right, m_rcItem.bottom };
        if (pInfo->iVLineSize > 0 && i < pInfo->nColumns - 1) {
            RECT rcLine = { rcItem.right - pInfo->iVLineSize / 2, rcItem.top, rcItem.right - pInfo->iVLineSize / 2, rcItem.bottom};
            CRenderEngine::DrawLine(hDC, rcLine, pInfo->iVLineSize, GetAdjustColor(pInfo->dwVLineColor));
        }
    }
    return CContainerUI::DoPaint(hDC, rcPaint, pStopControl);
}

} // namespace DuiLib
