// TestViewDlg.cpp: Implementation file
//

#include "pch.h"
#include "framework.h"
#include "IntentFlow.h"
#include "TestViewDlg.h"
#include "afxdialogex.h"


// CTestViewDlg dialog

IMPLEMENT_DYNAMIC(CTestViewDlg, CDialogEx)

CTestViewDlg::CTestViewDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TESTVIEW_FORM, pParent)
	, m_strAPIKey(_T(""))
	, m_strImagePath(_T(""))
	, m_strQuestion(_T(""))
	, m_strResult(_T(""))
	, m_nTaskType(0)
{
	// Initialize smart pointers
	m_qwenAPI = std::make_shared<QwenAPI>();
	m_testInterface = std::make_shared<TestInterface>(m_qwenAPI);
}

CTestViewDlg::~CTestViewDlg()
{
}

void CTestViewDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_EDIT_API_KEY, m_editAPIKey);
	DDX_Control(pDX, IDC_EDIT_IMAGE_PATH, m_editImagePath);
	DDX_Control(pDX, IDC_EDIT_QUESTION, m_editQuestion);
	DDX_Control(pDX, IDC_EDIT_RESULT, m_editResult);
	DDX_Control(pDX, IDC_BTN_LOAD_IMAGE, m_btnLoadImage);
	DDX_Control(pDX, IDC_BTN_RUN_TEST, m_btnRunTest);
	DDX_Control(pDX, IDC_BTN_EXPORT, m_btnExport);
	DDX_Control(pDX, IDC_COMBO_TASK_TYPE, m_comboTaskType);
	DDX_Control(pDX, IDC_STATIC_IMAGE_PREVIEW, m_staticImagePreview);
	DDX_Text(pDX, IDC_EDIT_API_KEY, m_strAPIKey);
	DDX_Text(pDX, IDC_EDIT_IMAGE_PATH, m_strImagePath);
	DDX_Text(pDX, IDC_EDIT_QUESTION, m_strQuestion);
	DDX_Text(pDX, IDC_EDIT_RESULT, m_strResult);
	DDX_CBIndex(pDX, IDC_COMBO_TASK_TYPE, m_nTaskType);
}

BEGIN_MESSAGE_MAP(CTestViewDlg, CDialogEx)
	ON_BN_CLICKED(IDC_BTN_LOAD_IMAGE, &CTestViewDlg::OnBnClickedBtnLoadImage)
	ON_BN_CLICKED(IDC_BTN_RUN_TEST, &CTestViewDlg::OnBnClickedBtnRunTest)
	ON_BN_CLICKED(IDC_BTN_EXPORT, &CTestViewDlg::OnBnClickedBtnExport)
	ON_CBN_SELCHANGE(IDC_COMBO_TASK_TYPE, &CTestViewDlg::OnCbnSelchangeComboTaskType)
END_MESSAGE_MAP()

// CTestViewDlg message handlers

BOOL CTestViewDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Initialize task type combo box
	m_comboTaskType.AddString(_T("GUI Grounding"));
	m_comboTaskType.AddString(_T("GUI Referring"));
	m_comboTaskType.AddString(_T("GUI VQA"));
	m_comboTaskType.SetCurSel(0);

	// Initialize test interface
	m_testInterface->Initialize();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CTestViewDlg::OnBnClickedBtnLoadImage()
{
	// Update data to variables
	UpdateData(TRUE);

	// Create file open dialog
	CFileDialog fileDlg(TRUE, _T("png"), NULL, OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Image Files (*.png;*.jpg;*.jpeg;*.bmp)|*.png;*.jpg;*.jpeg;*.bmp|All Files (*.*)|*.*||"));

	if (fileDlg.DoModal() == IDOK)
	{
		m_strImagePath = fileDlg.GetPathName();
		UpdateData(FALSE);
	}
}

void CTestViewDlg::OnBnClickedBtnRunTest()
{
	// Update data to variables
	UpdateData(TRUE);

	// Check image path
	if (m_strImagePath.IsEmpty())
	{
		AfxMessageBox(_T("Please select an image first!"));
		return;
	}

	// Check question input
	if (m_strQuestion.IsEmpty())
	{
		AfxMessageBox(_T("Please enter a question or coordinates!"));
		return;
	}

	// Set API key
	m_qwenAPI->setApiKey(CStringA(m_strAPIKey).GetString());

	// Execute test
	TestInterface::TestResult result;
	
	switch (m_nTaskType)
	{
	case 0: // GUI Grounding
		result = m_testInterface->executeGroundingTest(CStringA(m_strImagePath).GetString(), CStringA(m_strQuestion).GetString());
		break;
	case 1: // GUI Referring
		{
			// Parse coordinate format "x,y"
			int commaPos = m_strQuestion.Find(_T(','));
			if (commaPos == -1)
			{
				AfxMessageBox(_T("For GUI Referring, please enter coordinates in format \"x,y\""));
				return;
			}
			
			CString strX = m_strQuestion.Left(commaPos);
			CString strY = m_strQuestion.Mid(commaPos + 1);
			
			int x = _ttoi(strX);
			int y = _ttoi(strY);
			
			result = m_testInterface->executeReferringTest(CStringA(m_strImagePath).GetString(), x, y);
		}
		break;
	case 2: // GUI VQA
		result = m_testInterface->executeVQATest(CStringA(m_strImagePath).GetString(), CStringA(m_strQuestion).GetString());
		break;
	default:
		AfxMessageBox(_T("Invalid task type selected"));
		return;
	}

	// Display result
	if (result.success)
	{
		// Use UTF-8 to Unicode conversion to ensure Chinese display correctly
		std::string utf8Result = result.output;
		std::wstring unicodeResult = QwenAPI::UTF8ToUnicode(utf8Result);
		m_strResult = unicodeResult.c_str();
	}
	else
	{
		// Error message also needs correct conversion
		std::string utf8Error = result.errorMessage;
		std::wstring unicodeError = QwenAPI::UTF8ToUnicode(utf8Error);
		m_strResult = unicodeError.c_str();
	}

	// Update data display
	UpdateData(FALSE);
}

void CTestViewDlg::OnBnClickedBtnExport()
{
	// Update data to variables
	UpdateData(TRUE);

	// Check if there is result to export
	if (m_strResult.IsEmpty())
	{
		AfxMessageBox(_T("No result to export!"));
		return;
	}

	// Create file save dialog
	CFileDialog fileDlg(FALSE, _T("txt"), _T("result.txt"), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));

	if (fileDlg.DoModal() == IDOK)
	{
		CString filePath = fileDlg.GetPathName();
		
		// Write result to file, ensure using UTF-8 encoding
		CFile file;
		if (file.Open(filePath, CFile::modeCreate | CFile::modeWrite))
		{
			// Add UTF-8 BOM
			BYTE utf8Bom[] = { 0xEF, 0xBB, 0xBF };
			file.Write(utf8Bom, 3);
			
			// Convert Unicode to UTF-8 then write to file
			CStringW unicodeStr(m_strResult);
			std::string utf8Str = QwenAPI::UnicodeToUTF8(std::wstring(unicodeStr));
			file.Write(utf8Str.c_str(), static_cast<UINT>(utf8Str.length()));
			file.Close();
			
			AfxMessageBox(_T("Result exported successfully!"));
		}
		else
		{
			AfxMessageBox(_T("Failed to export result!"));
		}
	}
}

void CTestViewDlg::OnCbnSelchangeComboTaskType()
{
	// Update data to variables
	UpdateData(TRUE);
	
	// Update interface prompt based on task type
	switch (m_nTaskType)
	{
	case 0: // GUI Grounding
		SetDlgItemText(IDC_STATIC, _T("Question:"));
		break;
	case 1: // GUI Referring
		SetDlgItemText(IDC_STATIC, _T("Coordinates (x,y):"));
		break;
	case 2: // GUI VQA
		SetDlgItemText(IDC_STATIC, _T("Question:"));
		break;
	}
}