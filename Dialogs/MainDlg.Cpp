#ifndef _MAINDLG_CPP
#define _MAINDLG_CPP
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include <Windows.H>
#include <Stdio.H>
#include <Stdlib.H>
#include <Process.H>
#include <CommCtrl.H>

#include "../Source/Entry.H"

#include "../Dialogs/MainDlg.H"
#include "../Dialogs/NetLogoDlg.H"

#include "../../@Libraries/CCRC32/CCRC32.H"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

using namespace NSWFL::Windows;
using namespace NSWFL::File;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//---------------------(Variable Declarations)
HWND hMainDialog = NULL;
HWND hCRC32Hex = NULL;
HWND hCRC32Dec = NULL;
HWND hProgress = NULL;
HWND hGoButton = NULL;
HANDLE hThread = NULL;

char sFileName[MAX_PATH];

volatile bool bCancel = false;
volatile double dPercentComplete = 0;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CalcCRC(void)
{
	CCRC32 CRC32;

	CRC32.Initialize();

	unsigned long ulCRC32 = 0xffffffff;

	FILE *fSource = NULL;
	unsigned char *sBuf = NULL;
	int iBytesRead = 0;
	int iBufferSize = 1024 * 1024;
	__int64 iTotalRead = 0;
	__int64 iFileSize = 0;

	if(!FileSize(sFileName, &iFileSize))
	{
		MessageBox(hMainDialog,
			"Failed to retrieve the size of the specified file.", gsTitleCaption, MB_ICONERROR);
		return false;
	}

	if(iFileSize > (1024*1024)*100) //If file is over 100 megabytes.
	{
		MEMORYSTATUSEX MS;
		memset(&MS, 0, sizeof(MS));
		MS.dwLength = sizeof(MS);
		GlobalMemoryStatusEx(&MS);

		if(iFileSize > (1024*1024)*500) //If file is over 500 megabytes.
		{
			iBufferSize = (1024*1024)*10; //25.0 Megabytes.
		}
	
		while(MS.ullAvailPhys < iBufferSize && iBufferSize > (1024*1024))
		{
			iBufferSize -= (1024 * 1024); //Reduce by 128k at a time untill under avail memory.
		}
	}

	//Enforce minimum buffer size.
	if(iBufferSize < 1024*64)
	{
		iBufferSize = 1024*64;
	}

	if(fopen_s(&fSource, sFileName, "rb") != 0)
	{
		MessageBox(hMainDialog,
			"Failed to open the specified file.", gsTitleCaption, MB_ICONERROR);
		return false;
	}

	if(!(sBuf = (unsigned char *)malloc(iBufferSize))) //Allocate memory for file buffering.
	{
		fclose(fSource);
		MessageBox(hMainDialog,
			"Failed to allocate enough RAM.\r\n"
			"Please close some other applications and try again.", gsTitleCaption, MB_ICONERROR);
		return false;
	}

	while((iBytesRead = fread(sBuf, sizeof(char), iBufferSize, fSource)) && !bCancel)
	{
		CRC32.PartialCRC(&ulCRC32, sBuf, iBytesRead);
		iTotalRead += iBytesRead;
		dPercentComplete = (((double)iTotalRead) / ((double)iFileSize)) * 100.0;
	}

	free(sBuf);
	fclose(fSource);

	ulCRC32 ^= 0xffffffff; //Finalize the CRC.

	if(!bCancel)
	{
		char sText[1024];
		sprintf_s(sText, sizeof(sText), "%X", ulCRC32);
	    Set_Text(hCRC32Hex, sText);
		sprintf_s(sText, sizeof(sText), "%d", ulCRC32);
		Set_Text(hCRC32Dec, sText);
		return true;
	}
	else{
		MessageBox(hMainDialog, "Cancelled.", gsTitleCaption, MB_ICONINFORMATION);
		return false;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

unsigned __stdcall CalcProc(void *pData)
{
    Set_Text(hCRC32Hex, "");
	Set_Text(hCRC32Dec, "");

	dPercentComplete = 0;
	SendMessage(hProgress, PBM_SETPOS, (int)dPercentComplete, 0);
	ShowWindow(hProgress, SW_SHOW);
	SetTimer(hMainDialog, 0, 500, NULL);

	CalcCRC();

	KillTimer(hMainDialog, 0);
	ShowWindow(hProgress, SW_HIDE);
	Set_Text(hGoButton, "Process");
	EnableWindow(hGoButton, TRUE);

	_endthreadex(CloseHandle(GetCurrentThread()));
	return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

BOOL CALLBACK MainDialog(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //--------------------------------------------------------------------------

    static HWND hFileName = NULL;

    //--------------------------------------------------------------------------

    if(uMsg == WM_INITDIALOG)
    {
        hMainDialog = hWnd;

        HMENU hMenu = GetSystemMenu(hWnd, FALSE);
        AppendMenu(hMenu, MF_SEPARATOR, 0, 0);
        AppendMenu(hMenu, MF_STRING, MAINDIALOG_MENU_ABOUT, "About");

        SendMessage(hWnd, (UINT)WM_SETTEXT, (WPARAM)0, (LPARAM)gsTitleCaption);
        SendMessage(hWnd, WM_SETICON, TRUE, (LPARAM) LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN)));

        hFileName = GetDlgItem(hWnd, IDC_FILENAME);
        hCRC32Dec = GetDlgItem(hWnd, IDC_CRCDEC);
        hCRC32Hex = GetDlgItem(hWnd, IDC_CRCHEX);
		hGoButton = GetDlgItem(hWnd, IDC_PROCESS);
        hProgress = GetDlgItem(hWnd, IDC_FILEPROGRESS);

		Set_Text(hCRC32Dec, "");
		Set_Text(hCRC32Hex, "");

        CenterWindow(hWnd);

        return TRUE; // Return TRUE to set the keyboard focus, Otherwise return FALSE.
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_TIMER)
    {
		SendMessage(hProgress, PBM_SETPOS, (int)dPercentComplete, 0);
		return TRUE;
	}
	
	//--------------------------------------------------------------------------

    else if(uMsg == WM_COMMAND)
    {
        if(wParam == IDC_PROCESS)
        {
			char sText[255];
			Get_Text(hGoButton, sText, sizeof(sText));
			if(_strcmpi(sText, "Cancel") == 0)
			{
				SuspendThread(hThread);

				if(MessageBox(hWnd,
					"Are you sure you want to cancel the current process?",
					gsTitleCaption, MB_ICONQUESTION | MB_YESNO | MB_DEFBUTTON2) == IDYES)
				{
					EnableWindow(hGoButton, FALSE);
					bCancel = true;
				}
				ResumeThread(hThread);
			}
			else{
				bCancel = false;

				memset(sFileName, 0, sizeof(sFileName));

				int iFileNameSz = Get_Text(hFileName, sFileName, sizeof(sFileName));
				if(iFileNameSz <= 0 || iFileNameSz >= sizeof(sFileName))
				{
					char sInitialDirectory[MAX_PATH];
					Get_DesktopDirectory(sInitialDirectory, sizeof(sInitialDirectory));
					if(OSFileDialog(hWnd, sFileName, sizeof(sFileName), "",
						"Select a file to have its CRC calculated.", "All-Files\0*.*\0\0",
						sInitialDirectory, OFD, OFN_EXPLORER + OFN_LONGNAMES + OFN_PATHMUSTEXIST))
					{
						Set_Text(hFileName, sFileName);
					}
				}

				iFileNameSz = Get_Text(hFileName, sFileName, sizeof(sFileName));
				if(iFileNameSz > 0 || iFileNameSz < sizeof(sFileName))
				{
					hThread = (HANDLE)
						_beginthreadex(NULL, 0, CalcProc, (LPVOID) NULL, CREATE_SUSPENDED, NULL);
					Set_Text(hGoButton, "Cancel");
					ResumeThread(hThread);
				}
			}
            return TRUE;
        }
		else if(wParam == IDC_SELECTFILE)
		{
			char sInitialDirectory[MAX_PATH];
			Get_DesktopDirectory(sInitialDirectory, sizeof(sInitialDirectory));
			if(OSFileDialog(hWnd, sFileName, sizeof(sFileName), "",
				"Select a file to have its CRC calculated.", "All-Files\0*.*\0\0",
				sInitialDirectory, OFD, OFN_EXPLORER + OFN_LONGNAMES + OFN_PATHMUSTEXIST))
			{
				Set_Text(hFileName, sFileName);
			}
		}
		else if(wParam == IDC_COPYTOCC_DEC)
		{
			CopyTextBoxToClipboard(hCRC32Dec);
		}
		else if(wParam == IDC_COPYTOCC_HEX)
		{
			CopyTextBoxToClipboard(hCRC32Hex);
		}

		return FALSE;
    }

    //--------------------------------------------------------------------------

	else if(uMsg == WM_PAINT)
    {
        HDC ThisHDC;
        PAINTSTRUCT ThisPS;

        ThisHDC = BeginPaint(hWnd, &ThisPS);

        //- Any painting should be done here.

        EndPaint(hWnd, &ThisPS);
        return TRUE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_SYSCOMMAND) //- Received a system menu message.
    {
        if(LOWORD(wParam) == MAINDIALOG_MENU_ABOUT) //- About.
        {
            _AboutDialogInfo ADI;
    
            ADI.DisplayIcon  = LoadIcon(ghAppInstance, MAKEINTRESOURCE(IDI_MAIN));
            ADI.TitleCaption = gsTitleCaption;
            ADI.FileVersion  = gsFileVersion;
            ADI.BuildDate    = __DATE__;
            ADI.BuildTime    = __TIME__;
            ADI.CopyRight    = gsAppCopyRight;
            ADI.OwnerHandle  = hWnd;

            NetLogo(&ADI);

            return TRUE;
        }

        return FALSE;
    }

    //--------------------------------------------------------------------------

    else if(uMsg == WM_CLOSE) //- Received a close message.
    {
        EndDialog(hWnd, 0);
        DestroyWindow(hWnd);

		hMainDialog = NULL;

        return TRUE;
    }

    //--------------------------------------------------------------------------

    return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#endif
