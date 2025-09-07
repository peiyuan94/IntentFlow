#pragma once
#include "afxdialogex.h"
#include "QwenAPI.h"
#include "TestInterface.h"

// Test View Dialog class
class CTestViewDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CTestViewDlg)

public:
	CTestViewDlg(CWnd* pParent = nullptr);   // Standard constructor
	virtual ~CTestViewDlg();

	// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_TESTVIEW_FORM };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()

private:
	// Control variables
	CEdit m_editAPIKey;
	CEdit m_editImagePath;
	CEdit m_editQuestion;
	CEdit m_editResult;
	CButton m_btnLoadImage;
	CButton m_btnRunTest;
	CButton m_btnExport;
	CComboBox m_comboTaskType;
	CStatic m_staticImagePreview;

	// Function variables
	std::shared_ptr<QwenAPI> m_qwenAPI;
	std::shared_ptr<TestInterface> m_testInterface;
	CString m_strAPIKey;
	CString m_strImagePath;
	CString m_strQuestion;
	CString m_strResult;
	int m_nTaskType;

public:
	virtual BOOL OnInitDialog();
	afx_msg void OnBnClickedBtnLoadImage();
	afx_msg void OnBnClickedBtnRunTest();
	afx_msg void OnBnClickedBtnExport();
	afx_msg void OnCbnSelchangeComboTaskType();
};