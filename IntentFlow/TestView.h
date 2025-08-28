#pragma once

class TestInterface;
class QwenAPI;

#include "IntentFlowDoc.h"
#include <memory>

class CTestView : public CFormView
{
protected: // create from serialization only
	CTestView() noexcept;
	DECLARE_DYNCREATE(CTestView)

public:
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TESTVIEW_FORM };
#endif

// Attributes
public:
	CIntentFlowDoc* GetDocument() const;

// Operations
public:

// Overrides
public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	virtual void OnInitialUpdate(); // called first time after construct

// Implementation
public:
	virtual ~CTestView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	// Control variables
	CEdit m_editImagePath;
	CEdit m_editQuestion;
	CEdit m_editResult;
	CButton m_btnLoadImage;
	CButton m_btnRunTest;
	CComboBox m_comboTaskType;
	CStatic m_staticImagePreview;

	// Test interface
	std::shared_ptr<QwenAPI> m_qwenAPI;
	std::unique_ptr<TestInterface> m_testInterface;

	// Internal methods
	void UpdateImagePreview();
	void RunSelectedTest();

// Generated message map functions
protected:
	afx_msg void OnBnClickedBtnLoadImage();
	afx_msg void OnBnClickedBtnRunTest();
	afx_msg void OnCbnSelchangeComboTaskType();
	afx_msg void OnSize(UINT nType, int cx, int cy);
	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnBnClickedBtnExport();
};