#pragma once


class CIntentFlowView : public CView
{
protected: // 仅从序列化创建
	CIntentFlowView() noexcept;
	DECLARE_DYNCREATE(CIntentFlowView)

// 特性
public:
	CIntentFlowDoc* GetDocument() const;

// 操作
public:

// 重写
public:
	virtual void OnDraw(CDC* pDC);  // 重写以绘制该视图
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:

// 实现
public:
	virtual ~CIntentFlowView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// 生成的消息映射函数
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()
};

#ifndef _DEBUG  // IntentFlowView.cpp 中的调试版本
inline CIntentFlowDoc* CIntentFlowView::GetDocument() const
   { return reinterpret_cast<CIntentFlowDoc*>(m_pDocument); }
#endif

