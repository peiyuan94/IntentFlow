#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "IntentFlow.h"
#include "MainFrm.h"

#include "ChildFrm.h"
#include "IntentFlowDoc.h"
#include "IntentFlowView.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CIntentFlowApp

BEGIN_MESSAGE_MAP(CIntentFlowApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CIntentFlowApp::OnAppAbout)
	// Standard document commands based on files
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
END_MESSAGE_MAP()


// CIntentFlowApp construction

CIntentFlowApp::CIntentFlowApp() noexcept
{

	m_nAppLook = 0;
	// Support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// If the application is built with Common Language Runtime support (/clr): 
	//     1) This additional setting is required for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build your project.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: Replace the following application ID string with your own unique ID string; 
	// recommended format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("IntentFlow.AppID.NoVersion"));

	// TODO: Add construction code here,
	// Place all significant initialization in InitInstance
}

// The one and only CIntentFlowApp object

CIntentFlowApp theApp;


// CIntentFlowApp initialization

BOOL CIntentFlowApp::InitInstance()
{
	CWinAppEx::InitInstance();


	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction();

	// Use RichEdit control	Need to include afxrich.h
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the final executable size,
	// you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as your company or organization name
	SetRegistryKey(_T("Application Wizard Generated Local Application"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)


	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	// serve as the connection between documents, frame windows and views
	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(IDR_IntentFlowTYPE,
		RUNTIME_CLASS(CIntentFlowDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CIntentFlowView));
	if (!pDocTemplate)
		return FALSE;
	AddDocTemplate(pDocTemplate);
	
	// Save the document template pointer
	m_pDocTemplate = pDocTemplate;

	// Create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame || !pMainFrame->LoadFrame(IDR_MAINFRAME))
	{
		delete pMainFrame;
		return FALSE;
	}
	m_pMainWnd = pMainFrame;


	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);



	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
		return FALSE;
	// The main window has been initialized, so show and update it
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}

int CIntentFlowApp::ExitInstance()
{
	//TODO: Handle additional resources you may have added
	AfxOleTerm(FALSE);

	return CWinAppEx::ExitInstance();
}

// CIntentFlowApp message handlers


// Used to display the "About" dialog box

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() noexcept;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CIntentFlowApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CIntentFlowApp customization load/save methods

void CIntentFlowApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
}

void CIntentFlowApp::LoadCustomState()
{
}

void CIntentFlowApp::SaveCustomState()
{
}
