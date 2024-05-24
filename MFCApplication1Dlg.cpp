#include "pch.h"
#include "framework.h"
#include "MFCApplication1.h"
#include "MFCApplication1Dlg.h"
#include "afxdialogex.h"
#include <thread>
#include <vector>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

class CAboutDlg : public CDialogEx
{
public:
    CAboutDlg();
#ifdef AFX_DESIGN_TIME
    enum { IDD = IDD_ABOUTBOX };
#endif
protected:
    virtual void DoDataExchange(CDataExchange* pDX);
protected:
    DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

CMFCApplication1Dlg::CMFCApplication1Dlg(CWnd* pParent /*=nullptr*/)
    : CDialogEx(IDD_MFCAPPLICATION1_DIALOG, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CMFCApplication1Dlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_EDIT_EXEPATH, m_editExePath);
    DDX_Control(pDX, IDC_EDIT_OUTPUT, m_editOutput);
}

BEGIN_MESSAGE_MAP(CMFCApplication1Dlg, CDialogEx)
    ON_WM_SYSCOMMAND()
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_SELECT_EXE, &CMFCApplication1Dlg::OnBnClickedButtonSelectExe)
    ON_BN_CLICKED(IDC_BUTTON_RUN_EXE, &CMFCApplication1Dlg::OnBnClickedButtonRunExe)
    ON_BN_CLICKED(IDC_BUTTON_EXIT, &CMFCApplication1Dlg::OnBnClickedButtonExit)
    ON_MESSAGE(WM_USER + 1, &CMFCApplication1Dlg::OnReceiveOutput)
END_MESSAGE_MAP()

BOOL CMFCApplication1Dlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
    ASSERT(IDM_ABOUTBOX < 0xF000);

    CMenu* pSysMenu = GetSystemMenu(FALSE);
    if (pSysMenu != nullptr)
    {
        BOOL bNameValid;
        CString strAboutMenu;
        bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
        ASSERT(bNameValid);
        if (!strAboutMenu.IsEmpty())
        {
            pSysMenu->AppendMenu(MF_SEPARATOR);
            pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
        }
    }

    SetIcon(m_hIcon, TRUE);
    SetIcon(m_hIcon, FALSE);

    return TRUE;
}

void CMFCApplication1Dlg::OnSysCommand(UINT nID, LPARAM lParam)
{
    if ((nID & 0xFFF0) == IDM_ABOUTBOX)
    {
        CAboutDlg dlgAbout;
        dlgAbout.DoModal();
    }
    else
    {
        CDialogEx::OnSysCommand(nID, lParam);
    }
}

void CMFCApplication1Dlg::OnPaint()
{
    if (IsIconic())
    {
        CPaintDC dc(this);

        SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

        int cxIcon = GetSystemMetrics(SM_CXICON);
        int cyIcon = GetSystemMetrics(SM_CYICON);
        CRect rect;
        GetClientRect(&rect);
        int x = (rect.Width() - cxIcon + 1) / 2;
        int y = (rect.Height() - cyIcon + 1) / 2;

        dc.DrawIcon(x, y, m_hIcon);
    }
    else
    {
        CDialogEx::OnPaint();
    }
}

HCURSOR CMFCApplication1Dlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}

void CMFCApplication1Dlg::OnBnClickedButtonSelectExe()
{
    CFileDialog fileDlg(TRUE, _T("exe"), NULL, OFN_FILEMUSTEXIST, _T("Executable Files (*.exe)|*.exe|All Files (*.*)|*.*||"));
    if (fileDlg.DoModal() == IDOK)
    {
        CString filePath = fileDlg.GetPathName();
        m_editExePath.SetWindowText(filePath);
    }
}

void CMFCApplication1Dlg::OnBnClickedButtonRunExe()
{
    CString exePath;
    m_editExePath.GetWindowText(exePath);

    if (exePath.IsEmpty())
    {
        AfxMessageBox(_T("Please select an executable first."));
        return;
    }

    HANDLE hReadPipe, hWritePipe;
    SECURITY_ATTRIBUTES sa;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.bInheritHandle = TRUE;
    sa.lpSecurityDescriptor = NULL;

    if (!CreatePipe(&hReadPipe, &hWritePipe, &sa, 0))
    {
        AfxMessageBox(_T("Failed to create pipe."));
        return;
    }

    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(STARTUPINFO));
    si.cb = sizeof(STARTUPINFO);
    si.hStdError = hWritePipe;
    si.hStdOutput = hWritePipe;
    si.dwFlags |= STARTF_USESTDHANDLES;

    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    if (!CreateProcess(NULL, exePath.GetBuffer(), NULL, NULL, TRUE, CREATE_NO_WINDOW, NULL, NULL, &si, &pi))
    {
        CloseHandle(hReadPipe);
        CloseHandle(hWritePipe);
        AfxMessageBox(_T("Failed to create process."));
        return;
    }

    CloseHandle(hWritePipe);

    std::thread readThread([hReadPipe, this]() {
        CHAR buffer[4096];
        DWORD bytesRead;
        CString output;

        while (ReadFile(hReadPipe, buffer, sizeof(buffer) - 1, &bytesRead, NULL) && bytesRead != 0)
        {
            buffer[bytesRead] = '\0';

            // Convert buffer to wide string
            CStringA ansiStr(buffer);
            CStringW wideStr(ansiStr);

            output += wideStr;

            // Clear the buffer for the next read
            ZeroMemory(buffer, sizeof(buffer));
        }

        CloseHandle(hReadPipe);

        this->PostMessage(WM_USER + 1, (WPARAM)new CString(output));
        });

    readThread.detach();

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

void CMFCApplication1Dlg::OnBnClickedButtonExit()
{
    EndDialog(IDCANCEL);
}

LRESULT CMFCApplication1Dlg::OnReceiveOutput(WPARAM wParam, LPARAM lParam)
{
    CString* pOutput = (CString*)wParam;

    m_editOutput.SetWindowText(*pOutput);
    delete pOutput;

    return 0;
}
