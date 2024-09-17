#ifndef __UILIST_H__
#define __UILIST_H__

#pragma once
#include "Layout/UIVerticalLayout.h"
#include "Layout/UIHorizontalLayout.h"

namespace DuiLib {
/////////////////////////////////////////////////////////////////////////////////////
//

typedef int (CALLBACK *PULVCompareFunc)(UINT_PTR, UINT_PTR, UINT_PTR);

////////////////////////////////////#lilei 20160627 虚拟列表接口/////////////////////////////////////////////////
//PULVirtualPrepareItem 未虚拟列表准备行数据行为（格式）
typedef CControlUI* (* PULVirtualItemFormat)();

class CListHeaderUI;
 
#define UILIST_MAX_COLUMNS 64

enum ESORT{ E_SORTNO, E_SORT_ASC, E_SORT_DESC, E_SORT_MAX };
enum EPANELPOS{E_PANELTOP, E_PANELBOTTOM };

struct DUICopyItem
{
	int nRow;
	int nCol;
	TCHAR *szText;
	int nszText;
};

typedef struct tagTListInfoUI
{
    int nColumns;
	BOOL bUsedHeaderContain[UILIST_MAX_COLUMNS];//用于复合表头标记该表头项是否作为表头项的容器使用
    RECT rcColumn[UILIST_MAX_COLUMNS];
    UINT uFixedHeight; 
    int nFont;
    UINT uTextStyle;
    RECT rcTextPadding;
    DWORD dwTextColor;
    DWORD dwBkColor;
    TDrawInfo diBk;
    bool bAlternateBk;
    DWORD dwSelectedTextColor;
    DWORD dwSelectedBkColor;
    TDrawInfo diSelected;
    DWORD dwHotTextColor;
    DWORD dwHotBkColor;
    TDrawInfo diHot;
    DWORD dwDisabledTextColor;
    DWORD dwDisabledBkColor;
    TDrawInfo diDisabled;
    int iHLineSize;
    DWORD dwHLineColor;
    int iVLineSize;
    DWORD dwVLineColor;
    bool bShowHtml;
    bool bMultiExpandable;
} TListInfoUI;


/////////////////////////////////////////////////////////////////////////////////////
//

class IListCallbackUI
{
public:
    virtual LPCTSTR GetItemText(CControlUI* pList, int iItem, int iSubItem) = 0;
};

class IListVirtalCallbackUI {
public:
	virtual CControlUI* CreateVirtualItem() = 0;
};

class IListOwnerUI
{
public:
    virtual TListInfoUI* GetListInfo() = 0;
    virtual int GetCurSel() const = 0;
    virtual bool SelectItem(int iIndex, bool bTakeFocus = false, bool bTriggerEvent=true) = 0;
    virtual void DoEvent(TEventUI& event) = 0;
    virtual bool ExpandItem(int iIndex, bool bExpand = true) = 0;
    virtual int GetExpandedItem() const = 0;
};

class IListUI : public IListOwnerUI
{
public:
    virtual CListHeaderUI* GetHeader() const = 0;
    virtual CContainerUI* GetList() const = 0;
    virtual IListCallbackUI* GetTextCallback() const = 0;
    virtual void SetTextCallback(IListCallbackUI* pCallback) = 0;
};

class IListItemUI
{
public:
    virtual int GetIndex() const = 0;
    virtual void SetIndex(int iIndex) = 0;
    virtual int GetDrawIndex() const = 0;
    virtual void SetDrawIndex(int iIndex) = 0;
    virtual IListOwnerUI* GetOwner() = 0;
    virtual void SetOwner(CControlUI* pOwner) = 0;
    virtual bool IsSelected() const = 0;
    virtual bool Select(bool bSelect = true, bool bTriggerEvent=true) = 0;
    virtual bool IsExpanded() const = 0;
    virtual bool Expand(bool bExpand = true) = 0;
    virtual void DrawItemText(HDC hDC, const RECT& rcItem) = 0;
};


/////////////////////////////////////////////////////////////////////////////////////
//
class CListBodyUI;
class CListHeaderUI;
class CListHeaderItemUI;
//support virtual list
//support composite header，example see listHeaderui decalre
class DUILIB_API CListUI : public CVerticalLayoutUI, public IListUI
{
public:
    CListUI();
    LPCTSTR GetClass() const;
    UINT GetControlFlags() const;
    LPVOID GetInterface(LPCTSTR pstrName);
	///////////////////////////////虚拟列表接口 #liulei 20160627/////////////////////////////////////////
	//如果使用了虚拟列表则外部调用Rmove,RemoveAt,Add,AddAt,RemoveAll 无效
	//> 设置虚表行的数据格式,需要指定行高（原理类似于MFC的虚表）
	void SetVirtualItemFormat(IListVirtalCallbackUI* vrtualitemfroamt);
	//> 设置是否为虚表显示数据
	void SetVirtual(bool bUse = false);
	//> 设置虚表数据个数
	void SetVirtualItemCount(int nCountItem);
	bool IsUseVirtualList() const;
	//> 获取虚表的行高度（虚表行高必须保持一致，不支持动态行高）
	int GetVirtualItemHeight();
	//> 获取虚表数据个数
	int GetVirtualItemCount() const;
	//> 获取容器可以显示的最大个数,采取的7舍8入原则
	int GetShowMaxItemCount() const;
	//> 获去从哪一行开始绘画的
	int GetDrawStartIndex() const;
	//> 获去最后绘画的控件行下标，用于绘画汇总最后一行
	int GetDrawLastIndex() const;
	//> 复制List数据到剪切板,需要响应 DUI_MSGTYPE_COPYITEM 事件 nMaxRowItemData 标记为Item的组成的最大Text
	//>  bUserDefine 是否为用户自定义数据，对于复杂的数据结构，copy不能盲足，需要用户自定义Copy
	//> bUserDefine 如果为TRUE 则会触发 DUI_MSGTYPE_COPYITEM Wparam 为ListItem Lparam 为DUICopyItem需要返回数据的地址，
	BOOL Copy(int nMaxRowItemData = 1024,bool bUserDefine = false);

	INT64 GetSelectControlTag();
	///> 是否启用虚表填充数据优化（优化：填充虚表数据的时候Item不可见，填充完毕之后恢复状态，减少刷新次数）
	///> 默认开启, 如果含有Item 含有 combo  则必须关闭优化，否则combo显示有问题
	void EnableVirtualOptimize(bool bEnableVirtualO = true);
	/////////////////////////////////////////////排序标记接口 #liulei 20161107//////////////////////////////////////////////////////
	void SetSort(int nIndex, ESORT esort, bool bTriggerEvent = false);
	CListHeaderItemUI*GetSortHeaderItem();
	////////////////////////////////////////////////浮动窗口的属性接口 #liulei 20161109///////////////////////////////////////////////////////
	void SetPanelHeight(int nHeight);
	void SetPanelPos(EPANELPOS ePanelPos);
	void SetPanelOffset(int nPanelOffset);
	void SetPanelXml(LPCTSTR szXml);
	void SetPanelAttributeList(LPCTSTR pstrList);
	void SetPanelVisible(bool bVisible = true);
	CChildLayoutUI *GetFloatPanel();
	///> 只有鼠标单击之后才能响应MouseWhell消息
	bool IsEnableMouseWhell();
	/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	//支持设置子项模板
	void SetItemTemplateXml(CDuiString xml);

    bool GetScrollSelect();
    void SetScrollSelect(bool bScrollSelect);
    int GetCurSel() const;
    bool SelectItem(int iIndex, bool bTakeFocus = false, bool bTriggerEvent=true);

    CControlUI* GetItemAt(int iIndex) const;
    int GetItemIndex(CControlUI* pControl) const;
    bool SetItemIndex(CControlUI* pControl, int iIndex);
    bool SetMultiItemIndex(CControlUI* pStartControl, int iCount, int iNewStartIndex);
    int GetCount() const;
	int GetItemCount() const;//获取实际Item的个数
    bool Add(CControlUI* pControl);
    bool AddAt(CControlUI* pControl, int iIndex);
	CControlUI* AddTemplate();
	CControlUI* AddTemplateAt(int iIndex);
    bool Remove(CControlUI* pControl, bool bDoNotDestroy=false);
    bool RemoveAt(int iIndex, bool bDoNotDestroy=false);
    void RemoveAll();
	void ResetSortStatus();//重置所有表头排序状态
    void EnsureVisible(int iIndex);
    void Scroll(int dx, int dy);

    int GetChildPadding() const;
    void SetChildPadding(int iPadding);

    CListHeaderUI* GetHeader() const;  
    CContainerUI* GetList() const;
    TListInfoUI* GetListInfo();

    UINT GetItemFixedHeight();
    void SetItemFixedHeight(UINT nHeight);
    int GetItemFont(int index);
    void SetItemFont(int index);
    UINT GetItemTextStyle();
    void SetItemTextStyle(UINT uStyle);
    RECT GetItemTextPadding() const;
    void SetItemTextPadding(RECT rc);
    DWORD GetItemTextColor() const;
    void SetItemTextColor(DWORD dwTextColor);
    DWORD GetItemBkColor() const;
    void SetItemBkColor(DWORD dwBkColor);
    LPCTSTR GetItemBkImage() const;
    void SetItemBkImage(LPCTSTR pStrImage);
    bool IsAlternateBk() const;
    void SetAlternateBk(bool bAlternateBk);
    DWORD GetSelectedItemTextColor() const;
    void SetSelectedItemTextColor(DWORD dwTextColor);
    DWORD GetSelectedItemBkColor() const;
    void SetSelectedItemBkColor(DWORD dwBkColor);
    LPCTSTR GetSelectedItemImage() const;
    void SetSelectedItemImage(LPCTSTR pStrImage);
    DWORD GetHotItemTextColor() const;
    void SetHotItemTextColor(DWORD dwTextColor);
    DWORD GetHotItemBkColor() const;
    void SetHotItemBkColor(DWORD dwBkColor);
    LPCTSTR GetHotItemImage() const;
    void SetHotItemImage(LPCTSTR pStrImage);
    DWORD GetDisabledItemTextColor() const;
    void SetDisabledItemTextColor(DWORD dwTextColor);
    DWORD GetDisabledItemBkColor() const;
    void SetDisabledItemBkColor(DWORD dwBkColor);
    LPCTSTR GetDisabledItemImage() const;
    void SetDisabledItemImage(LPCTSTR pStrImage);
    int GetItemHLineSize() const;
    void SetItemHLineSize(int iSize);
    DWORD GetItemHLineColor() const;
    void SetItemHLineColor(DWORD dwLineColor);
    int GetItemVLineSize() const;
    void SetItemVLineSize(int iSize);
    DWORD GetItemVLineColor() const;
    void SetItemVLineColor(DWORD dwLineColor);
    bool IsItemShowHtml();
    void SetItemShowHtml(bool bShowHtml = true);

    void SetMultiExpanding(bool bMultiExpandable); 
    int GetExpandedItem() const;
    bool ExpandItem(int iIndex, bool bExpand = true);

	void SetPos(RECT rc, bool bNeedInvalidate = true);
	void Move(SIZE szOffset, bool bNeedInvalidate = true);
    void DoEvent(TEventUI& event);
    void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);

    IListCallbackUI* GetTextCallback() const;
    void SetTextCallback(IListCallbackUI* pCallback);

    SIZE GetScrollPos() const;
    SIZE GetScrollRange() const;
    void SetScrollPos(SIZE szPos);
	///>@param pControl		需要绘画的当前控件
	///>@param nDrawRow		当前控件所在的行
	///>@param nStartDrwRow 从哪一行开始绘画的
	void DrawVirtualItem(CControlUI *pControl, int nDrawRow,int nStartDrwRow);
    void LineUp();
    void LineDown();
    void PageUp();
    void PageDown();
    void HomeUp();
    void EndDown();
    void LineLeft();
    void LineRight();
    void PageLeft();
    void PageRight();
    void HomeLeft();
    void EndRight();
    void EnableScrollBar(bool bEnableVertical = true, bool bEnableHorizontal = false);
    virtual CScrollBarUI* GetVerticalScrollBar() const;
    virtual CScrollBarUI* GetHorizontalScrollBar() const;
    bool SortItems(PULVCompareFunc pfnCompare, UINT_PTR dwData);

private:
	///> 虚表的时候设置选中行的标记,这个标记根据Item的Tag来标记
	void SetSelectControlTag(INT64 iControlTag);

	void ResizeVirtualItemBuffer();//动态调整虚拟表的缓冲区
	bool AddVirtualItem(CControlUI* pControl);
	void CalcPanelPos();
	CControlUI *CreateTemplateControl();
protected:
	INT64 m_iSelectControlTag;
	bool m_bUseVirtualList;
    bool m_bScrollSelect;
	bool m_bEnableVirtualO;//是否启用虚表优化
    int m_iCurSel;
    int m_iExpandedItem;
    IListCallbackUI* m_pCallback;
    CListBodyUI* m_pList;
    CListHeaderUI* m_pHeader;
	//#liulei 是否允许使用鼠标滚动，
	//只有在鼠标点击之后才可以使用
	bool m_bEnableMouseWhell;
	//#liulei 20161109 增加list中的浮动Panel，类似于酷狗的搜索
	CChildLayoutUI *m_pFloatPanel;//list中浮动的Panel
	EPANELPOS		m_ePanelPos;//Panel的位置，目前仅仅支持上下
	int				m_nPanelHeight;//Panel高度
	int				m_nPanelOffset;//panel的位置便宜量
    TListInfoUI m_ListInfo;
	IListVirtalCallbackUI*  m_pVirutalItemFormat;//虚拟数据格式指针
	int m_nVirtualItemHeight;
	int m_nVirtualItemCount;
	int	m_nMaxShowCount;
	int	m_nDrawStartIndex;//从哪一行开始绘画的，从0开始
	CDuiString m_ItemtemplateXml;
};

/////////////////////////////////////////////////////////////////////////////////////
//
class CListHeaderItemUI;
/* suport composite header add by liulei 20161122
example:
<ListHeader height="40">
	<ListHeaderItem text="" name="10" sort="false" sortwidth="9" sortheight="8" width="120" inset="2,2,2,2" >
		<VerticalLayout>
			<HorizontalLayout bkcolor="#ffffff00">
				<Label text="parent" estimate="true" align="center" />
			</HorizontalLayout>

			<HorizontalLayout bkcolor="#ffff00ff">
				<ListHeaderItem text="child1" name="18" sort="true" sortwidth="9" sortheight="8" />
				<ListHeaderItem text="child2" name="19" sort="true" sortwidth="9" sortheight="8" />
				<ListHeaderItem text="child3" name="110" sort="true" sortwidth="9" sortheight="8" sepwidth="0" />
			</HorizontalLayout>
		</VerticalLayout>
	</ListHeaderItem>
</ListHeader>
*/
class DUILIB_API CListHeaderUI : public CHorizontalLayoutUI
{
public:
	friend CListHeaderItemUI;
	CListHeaderUI();

    LPCTSTR GetClass() const;
    LPVOID GetInterface(LPCTSTR pstrName);
	bool Add(CControlUI* pControl);
	bool AddAt(CControlUI* pControl, int iIndex);
	void RemoveAll();
	bool Remove(CControlUI* pControl, bool bDoNotDestroy = false);
	bool RemoveAt(int iIndex, bool bDoNotDestroy = false);
	void SetVisible(bool bVisible = true);
	void SetItemVisible(int nIndex, bool bVisible = true, bool bInvalidate = true);
	void SetSort(int nIndex, ESORT esort, bool bTriggerEvent = false);
	CListHeaderItemUI *GetSortHeaderItem();
    SIZE EstimateSize(SIZE szAvailable);

private:
	CListHeaderItemUI	  *m_pSortHeaderItem;//排序Item
};


/////////////////////////////////////////////////////////////////////////////////////
//

class DUILIB_API CListHeaderItemUI : public CContainerUI
{
public:
    CListHeaderItemUI();

    LPCTSTR GetClass() const;
    LPVOID GetInterface(LPCTSTR pstrName);
    UINT GetControlFlags() const;
    void SetEnabled(bool bEnable = true);
	void SetVisible(bool bVisible = true);
	bool IsDragable() const;
    void SetDragable(bool bDragable);
	DWORD GetSepWidth() const;
    void SetSepWidth(int iWidth);
	DWORD GetTextStyle() const;
    void SetTextStyle(UINT uStyle);
	DWORD GetTextColor() const;
    void SetTextColor(DWORD dwTextColor);
    DWORD GetSepColor() const;
    void SetSepColor(DWORD dwSepColor);
	void SetTextPadding(RECT rc);
	RECT GetTextPadding() const;
    void SetFont(int index);
    bool IsShowHtml();
    void SetShowHtml(bool bShowHtml = true);
    LPCTSTR GetNormalImage() const;
    void SetNormalImage(LPCTSTR pStrImage);
    LPCTSTR GetHotImage() const;
    void SetHotImage(LPCTSTR pStrImage);
    LPCTSTR GetPushedImage() const;
    void SetPushedImage(LPCTSTR pStrImage);
    LPCTSTR GetFocusedImage() const;
    void SetFocusedImage(LPCTSTR pStrImage);
    LPCTSTR GetSepImage() const;
    void SetSepImage(LPCTSTR pStrImage);

	/// 增加表头排序图标//////////////////////////////////////////////
	void SetEnabledSort(bool bEnableSort);
	void SetSort(ESORT esort, bool bTriggerEvent = true);
	ESORT GetSort();
	void SetSortAscImg(LPCTSTR pStrImage);
	void SetSortDescImg(LPCTSTR pStrImage);
	void SetSortWidth(int nSortWidht);
	void SetSortHeight(int nSrotHeight);
	//////////////////////////////////////////////////////////////

    void DoEvent(TEventUI& event);
	void SetPos(RECT rc, bool bNeedInvalidate);
    SIZE EstimateSize(SIZE szAvailable);
    void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
    RECT GetThumbRect() const;
	RECT GetSortRect() const;
    void PaintText(HDC hDC);
    void PaintStatusImage(HDC hDC);
private:
	void SetSortStatus(ESORT esort, bool bTriggerEvent = false);
protected:
    POINT ptLastMouse;
    bool m_bDragable;
	bool m_bEnablebSort;
    UINT m_uButtonState;
    int m_iSepWidth;
    DWORD m_dwTextColor;
    DWORD m_dwSepColor;
    int m_iFont;
    UINT m_uTextStyle;
    bool m_bShowHtml;
	RECT m_rcTextPadding;
    TDrawInfo m_diNormal;
    TDrawInfo m_diHot;
    TDrawInfo m_diPushed;
    TDrawInfo m_diFocused;
    TDrawInfo m_diSep;
	TDrawInfo m_diAscSort;
	TDrawInfo m_diDescSort;
	ESORT m_esrot;
	int m_nSortWidth;
	int m_nSortHeight;
};


/////////////////////////////////////////////////////////////////////////////////////
//

class DUILIB_API CListElementUI : public CControlUI, public IListItemUI
{
public:
    CListElementUI();

    LPCTSTR GetClass() const;
    UINT GetControlFlags() const;
    LPVOID GetInterface(LPCTSTR pstrName);

    void SetEnabled(bool bEnable = true);

    int GetIndex() const;
    void SetIndex(int iIndex);
    int GetDrawIndex() const;
    void SetDrawIndex(int iIndex);

    IListOwnerUI* GetOwner();
    void SetOwner(CControlUI* pOwner);
    void SetVisible(bool bVisible = true);

    bool IsSelected() const;
    bool Select(bool bSelect = true, bool bTriggerEvent=true);
    bool IsExpanded() const;
    bool Expand(bool bExpand = true);

    void Invalidate(); // 直接CControl::Invalidate会导致滚动条刷新，重写减少刷新区域
    bool Activate();

    void DoEvent(TEventUI& event);
    void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);

    void DrawItemBk(HDC hDC, const RECT& rcItem);

protected:
    int m_iIndex;
    int m_iDrawIndex;
    bool m_bSelected;
    UINT m_uButtonState;
    IListOwnerUI* m_pOwner;
};


/////////////////////////////////////////////////////////////////////////////////////
//

class DUILIB_API CListLabelElementUI : public CListElementUI
{
public:
    CListLabelElementUI();

    LPCTSTR GetClass() const;
    LPVOID GetInterface(LPCTSTR pstrName);

    void SetOwner(CControlUI* pOwner);

    void SetFixedWidth(int cx);
    void SetFixedHeight(int cy);
    void SetText(LPCTSTR pstrText);

    void DoEvent(TEventUI& event);
    SIZE EstimateSize(SIZE szAvailable);
    bool DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl);

    void DrawItemText(HDC hDC, const RECT& rcItem);

protected:
    SIZE    m_cxyFixedLast;
    bool    m_bNeedEstimateSize;

    SIZE    m_szAvailableLast;
    UINT    m_uFixedHeightLast; 
    int     m_nFontLast;
    UINT    m_uTextStyleLast;
    RECT    m_rcTextPaddingLast;
};


/////////////////////////////////////////////////////////////////////////////////////
//

class DUILIB_API CListTextElementUI : public CListLabelElementUI
{
public:
    CListTextElementUI();
    ~CListTextElementUI();

    LPCTSTR GetClass() const;
    LPVOID GetInterface(LPCTSTR pstrName);
    UINT GetControlFlags() const;
	int GetCount() const;
    LPCTSTR GetText(int iIndex) const;
    void SetText(int iIndex, LPCTSTR pstrText,UINT uTextStyle = 0);

    void SetOwner(CControlUI* pOwner);
    CDuiString* GetLinkContent(int iIndex);

    void DoEvent(TEventUI& event);
    SIZE EstimateSize(SIZE szAvailable);

    void DrawItemText(HDC hDC, const RECT& rcItem);

protected:
    enum { MAX_LINK = 8 };
    int m_nLinks;
    RECT m_rcLinks[MAX_LINK];
    CDuiString m_sLinks[MAX_LINK];
    int m_nHoverLink;
    IListUI* m_pOwner;
    CDuiPtrArray m_aTexts;
	UINT m_uTextsStyle[UILIST_MAX_COLUMNS];
    CDuiString m_sTextLast;
};

/////////////////////////////////////////////////////////////////////////////////////
//

class DUILIB_API CListContainerElementUI : public CContainerUI, public IListItemUI
{
public:
    CListContainerElementUI();

    LPCTSTR GetClass() const;
    UINT GetControlFlags() const;
    LPVOID GetInterface(LPCTSTR pstrName);

    int GetIndex() const;
    void SetIndex(int iIndex);
    int GetDrawIndex() const;
    void SetDrawIndex(int iIndex);

    IListOwnerUI* GetOwner();
    void SetOwner(CControlUI* pOwner);
    void SetVisible(bool bVisible = true);
    void SetEnabled(bool bEnable = true);

    bool IsSelected() const;
    bool Select(bool bSelect = true, bool bTriggerEvent=true);
    bool IsExpandable() const;
    void SetExpandable(bool bExpandable);
    bool IsExpanded() const;
    bool Expand(bool bExpand = true);

    void Invalidate(); // 直接CControl::Invalidate会导致滚动条刷新，重写减少刷新区域
    bool Activate();

    void DoEvent(TEventUI& event);
    void SetAttribute(LPCTSTR pstrName, LPCTSTR pstrValue);
    bool DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl);

    void DrawItemText(HDC hDC, const RECT& rcItem);    
    void DrawItemBk(HDC hDC, const RECT& rcItem);

    SIZE EstimateSize(SIZE szAvailable);

protected:
    int m_iIndex;
    int m_iDrawIndex;
    bool m_bSelected;
    bool m_bExpandable;
    bool m_bExpand;
    UINT m_uButtonState;
    IListOwnerUI* m_pOwner;
};

/////////////////////////////////////////////////////////////////////////////////////
//

class DUILIB_API CListHBoxElementUI : public CListContainerElementUI
{
public:
    CListHBoxElementUI();

    LPCTSTR GetClass() const;
    LPVOID GetInterface(LPCTSTR pstrName);

    void SetPos(RECT rc, bool bNeedInvalidate = true);
    bool DoPaint(HDC hDC, const RECT& rcPaint, CControlUI* pStopControl);
};

} // namespace DuiLib

#endif // __UILIST_H__
