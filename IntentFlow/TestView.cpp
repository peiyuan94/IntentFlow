// TestView.cpp : implementation of the CTestView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project that implements preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "IntentFlow.h"
#endif

#include "TestView.h"
#include "afxdialogex.h"
#include "TestInterface.h"
#include "QwenAPI.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// CTestView

IMPLEMENT_DYNCREATE(CTestView, CFormView)

BEGIN_MESSAGE_MAP(CTestView, CFormView)
	ON_BN_CLICKED(IDC_BTN_LOAD_IMAGE, &CTestView::OnBnClickedBtnLoadImage)
	ON_BN_CLICKED(IDC_BTN_RUN_TEST, &CTestView::OnBnClickedBtnRunTest)
	ON_CBN_SELCHANGE(IDC_COMBO_TASK_TYPE, &CTestView::OnCbnSelchangeComboTaskType)
	ON_WM_SIZE()
	ON_BN_CLICKED(IDC_BTN_EXPORT, &CTestView::OnBnClickedBtnExport)
END_MESSAGE_MAP()

// CTestView construction/destruction

CTestView::CTestView() noexcept
	: CFormView(IDD_TESTVIEW_FORM)
{
	// Initialize QwenAPI and TestInterface
	m_qwenAPI = std::make_shared<QwenAPI>();
	m_testInterface = std::make_unique<TestInterface>(m_qwenAPI);
}

CTestView::~CTestView()
{
}

void CTestView::DoDataExchange(CDataExchange* pDX)
{
	CFormView::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_IMAGE_PATH, m_editImagePath);
	DDX_Control(pDX, IDC_EDIT_QUESTION, m_editQuestion);
	DDX_Control(pDX, IDC_EDIT_RESULT, m_editResult);
	DDX_Control(pDX, IDC_BTN_LOAD_IMAGE, m_btnLoadImage);
	DDX_Control(pDX, IDC_BTN_RUN_TEST, m_btnRunTest);
	DDX_Control(pDX, IDC_COMBO_TASK_TYPE, m_comboTaskType);
	DDX_Control(pDX, IDC_STATIC_IMAGE_PREVIEW, m_staticImagePreview);
}

BOOL CTestView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CFormView::PreCreateWindow(cs);
}

void CTestView::OnInitialUpdate()
{
	CFormView::OnInitialUpdate();
	GetParentFrame()->RecalcLayout();
	ResizeParentToFit();

	// Initialize task type combo box
	m_comboTaskType.AddString(_T("GUI Grounding"));
	m_comboTaskType.AddString(_T("GUI Referring"));
	m_comboTaskType.AddString(_T("GUI VQA"));
	m_comboTaskType.SetCurSel(0); // Default select first task type

	// Set default placeholder text
	m_editImagePath.SetWindowText(_T("Click 'Load Image' to select an image file"));
	m_editQuestion.SetWindowText(_T("Enter your question here"));
	m_editResult.SetWindowText(_T("Test results will be displayed here"));
}

// CTestView diagnostics

#ifdef _DEBUG
void CTestView::AssertValid() const
{
	CFormView::AssertValid();
}

void CTestView::Dump(CDumpContext& dc) const
{
	CFormView::Dump(dc);
}

CIntentFlowDoc* CTestView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(CIntentFlowDoc)));
	return (CIntentFlowDoc*)m_pDocument;
}
#endif //_DEBUG

// CTestView message handlers

void CTestView::OnBnClickedBtnLoadImage()
{
	// Create file dialog to select image file
	CFileDialog fileDlg(TRUE, NULL, NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Image Files (*.bmp;*.jpg;*.jpeg;*.png)|*.bmp;*.jpg;*.jpeg;*.png|All Files (*.*)|*.*||"));

	if (fileDlg.DoModal() == IDOK)
	{
		CString filePath = fileDlg.GetPathName();
		m_editImagePath.SetWindowText(filePath);

		// Update image preview
		UpdateImagePreview();
	}
}

void CTestView::OnBnClickedBtnRunTest()
{
	RunSelectedTest();
}

void CTestView::OnCbnSelchangeComboTaskType()
{
	// Handle task type change
	int selectedIndex = m_comboTaskType.GetCurSel();
	switch (selectedIndex)
	{
	case 0: // GUI Grounding
		m_editQuestion.SetWindowText(_T("Enter your question about the image"));
		break;
	case 1: // GUI Referring
		m_editQuestion.SetWindowText(_T("Enter coordinates (x,y) of the control"));
		break;
	case 2: // GUI VQA
		m_editQuestion.SetWindowText(_T("Enter your question about the image"));
		break;
	}
}

void CTestView::OnSize(UINT nType, int cx, int cy)
{
	CFormView::OnSize(nType, cx, cy);

	// TODO: Add your message handler code here
}

void CTestView::UpdateImagePreview()
{
	// Implementation to update image preview
	CString imagePath;
	m_editImagePath.GetWindowText(imagePath);

	if (!imagePath.IsEmpty())
	{
		// Here we can load and display image preview
		// For simplicity, we only display the file name
	}
}

void CTestView::RunSelectedTest()
{
	CString imagePath, question;
	m_editImagePath.GetWindowText(imagePath);
	m_editQuestion.GetWindowText(question);

	if (imagePath.IsEmpty() || question.IsEmpty())
	{
		AfxMessageBox(_T("Please provide both image path and question."));
		return;
	}

	// Get selected task type
	int taskType = m_comboTaskType.GetCurSel();
	TestInterface::TestResult result;

	// Convert CString to std::string
	CStringA imagePathA(imagePath);
	CStringA questionA(question);
	std::string imagePathStr(imagePathA);
	std::string questionStr(questionA);

	switch (taskType)
	{
	case 0: // GUI Grounding
		result = m_testInterface->executeGroundingTest(imagePathStr, questionStr);
		break;
	case 1: // GUI Referring
		{
			// Parse coordinate input
			int x = 0, y = 0;
			// Simplified processing, actually should parse "x,y" format input
			result = m_testInterface->executeReferringTest(imagePathStr, x, y);
		}
		break;
	case 2: // GUI VQA
		result = m_testInterface->executeVQATest(imagePathStr, questionStr);
		break;
	default:
		AfxMessageBox(_T("Invalid task type selected."));
		return;
	}

	// Display result
	CString resultText;
	if (result.success)
	{
		resultText = CString(result.output.c_str());
	}
	else
	{
		resultText = CString(result.errorMessage.c_str());
	}
	
	m_editResult.SetWindowText(resultText);
}

void CTestView::OnBnClickedBtnExport()
{
	// Export results to file
	CString resultText;
	m_editResult.GetWindowText(resultText);

	if (resultText.IsEmpty())
	{
		AfxMessageBox(_T("No results to export."));
		return;
	}

	// Create file save dialog
	CFileDialog fileDlg(FALSE, _T("txt"), _T("test_result.txt"), 
		OFN_OVERWRITEPROMPT, _T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));

	if (fileDlg.DoModal() == IDOK)
	{
		CString filePath = fileDlg.GetPathName();
		
		// Convert to std::string
		CStringA filePathA(filePath);
		std::string filePathStr(filePathA);
		
		// Write to file
		std::ofstream outFile(filePathStr);
		if (outFile.is_open())
		{
			// Convert result text to std::string
			CStringA resultTextA(resultText);
			std::string resultTextStr(resultTextA);
			
			outFile << resultTextStr;
			outFile.close();
			AfxMessageBox(_T("Results exported successfully."));
		}
		else
		{
			AfxMessageBox(_T("Failed to export results."));
		}
	}
}