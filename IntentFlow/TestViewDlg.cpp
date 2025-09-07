#include "pch.h"
#include "afxdialogex.h"
#include "IntentFlow.h"

#include "TestViewDlg.h"

// CTestViewDlg 对话框

IMPLEMENT_DYNAMIC(CTestViewDlg, CDialogEx)

CTestViewDlg::CTestViewDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_TESTVIEW_FORM, pParent)
	, m_strAPIKey(_T(""))
	, m_strImagePath(_T(""))
	, m_strQuestion(_T(""))
	, m_strResult(_T(""))
	, m_nTaskType(0)
{
	// 初始化智能指针
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

// CTestViewDlg 消息处理程序

BOOL CTestViewDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 初始化任务类型组合框
	m_comboTaskType.AddString(_T("GUI Grounding"));
	m_comboTaskType.AddString(_T("GUI Referring"));
	m_comboTaskType.AddString(_T("GUI VQA"));
	m_comboTaskType.SetCurSel(0);

	// 初始化测试接口
	m_testInterface->Initialize();

	return TRUE;  // return TRUE unless you set the focus to a control
}

void CTestViewDlg::OnBnClickedBtnLoadImage()
{
	// 更新数据到变量
	UpdateData(TRUE);

	// 创建文件打开对话框
	CFileDialog fileDlg(TRUE, _T("png"), NULL, 
		OFN_FILEMUSTEXIST | OFN_HIDEREADONLY,
		_T("Image Files (*.png;*.jpg;*.jpeg;*.bmp)|*.png;*.jpg;*.jpeg;*.bmp|All Files (*.*)|*.*||"));

	if (fileDlg.DoModal() == IDOK)
	{
		m_strImagePath = fileDlg.GetPathName();
		UpdateData(FALSE);
	}
}

void CTestViewDlg::OnBnClickedBtnRunTest()
{
	// 更新数据到变量
	UpdateData(TRUE);

	// 检查图像路径
	if (m_strImagePath.IsEmpty())
	{
		AfxMessageBox(_T("Please select an image first!"));
		return;
	}

	// 检查问题输入
	if (m_strQuestion.IsEmpty())
	{
		AfxMessageBox(_T("Please enter a question or coordinates!"));
		return;
	}

	// 设置API密钥
	m_qwenAPI->setApiKey(CStringA(m_strAPIKey).GetString());

	// 执行测试
	TestInterface::TestResult result;
	
	switch (m_nTaskType)
	{
	case 0: // GUI Grounding
		result = m_testInterface->executeGroundingTest(CStringA(m_strImagePath).GetString(), CStringA(m_strQuestion).GetString());
		break;
	case 1: // GUI Referring
		{
			// 解析坐标格式 "x,y"
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

	// 显示结果
	if (result.success)
	{
		m_strResult = CString(result.output.c_str());
	}
	else
	{
		m_strResult = CString(result.errorMessage.c_str());
	}

	// 更新数据显示
	UpdateData(FALSE);
}

void CTestViewDlg::OnBnClickedBtnExport()
{
	// 更新数据到变量
	UpdateData(TRUE);

	// 检查是否有结果可导出
	if (m_strResult.IsEmpty())
	{
		AfxMessageBox(_T("No result to export!"));
		return;
	}

	// 创建文件保存对话框
	CFileDialog fileDlg(FALSE, _T("txt"), _T("result.txt"), 
		OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		_T("Text Files (*.txt)|*.txt|All Files (*.*)|*.*||"));

	if (fileDlg.DoModal() == IDOK)
	{
		CString filePath = fileDlg.GetPathName();
		
		// 写入结果到文件
		CFile file;
		if (file.Open(filePath, CFile::modeCreate | CFile::modeWrite))
		{
			CArchive ar(&file, CArchive::store);
			ar << m_strResult;
			ar.Close();
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
	// 更新数据到变量
	UpdateData(TRUE);
	
	// 根据任务类型更新界面提示
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