// 这段 MFC 示例源代码演示如何使用 MFC Microsoft Office Fluent 用户界面
// (“Fluent UI”)。该示例仅供参考，
// 用以补充《Microsoft 基础类参考》和
// MFC C++ 库软件随附的相关电子文档。
// 复制、使用或分发 Fluent UI 的许可条款是单独提供的。
// 若要了解有关 Fluent UI 许可计划的详细信息，请访问
// https://go.microsoft.com/fwlink/?LinkId=238214.
//
// 版权所有(C) Microsoft Corporation
// 保留所有权利。

// IntentFlowView.cpp: CIntentFlowView 类的实现
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS 可以在实现预览、缩略图和搜索筛选器句柄的
// ATL 项目中进行定义，并允许与该项目共享文档代码。
#ifndef SHARED_HANDLERS
#include "IntentFlow.h"
#endif

#include "IntentFlowDoc.h"
#include "IntentFlowView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CIntentFlowView

IMPLEMENT_DYNCREATE(CIntentFlowView, CView)

BEGIN_MESSAGE_MAP(CIntentFlowView, CView)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
END_MESSAGE_MAP()

// CIntentFlowView 构造/析构

CIntentFlowView::CIntentFlowView() noexcept
{
	// TODO: 在此处添加构造代码

}

CIntentFlowView::~CIntentFlowView()
{
}

BOOL CIntentFlowView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: 在此处通过修改
	//  CREATESTRUCT cs 来修改窗口类或样式

	return CView::PreCreateWindow(cs);
}

// CIntentFlowView 绘图

void CIntentFlowView::OnDraw(CDC* /*pDC*/)
{
	CIntentFlowDoc* pDoc = GetDocument();
	ASSERT_VALID(pDoc);
	if (!pDoc)
		return;

	// TODO: 在此处为本机数据添加绘制代码
}

void CIntentFlowView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void CIntentFlowView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}


// CIntentFlowView 诊断

#ifdef _DEBUG
void CIntentFlowView::AssertValid() const
{
	CView::AssertValid();
}

void CIntentFlowView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

CIntentFlowDoc* CIntentFlowView::GetDocument() const // 非调试版本是内联的
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CIntentFlowDoc)));
	return (CIntentFlowDoc*)m_pDocument;
}
#endif //_DEBUG


// CIntentFlowView 消息处理程序
