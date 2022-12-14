// dui-demo.cpp : main source file
//

#include "stdafx.h"
#include "MainDlg.h"
#include "trayicon\SShellNotifyIcon.h"

#ifdef _DEBUG
#define SYS_NAMED_RESOURCE _T("soui-sys-resourced.dll")
#else
#define SYS_NAMED_RESOURCE _T("soui-sys-resource.dll")
#endif

#include "GlobalUnits.h"

#include "snapshot/SSnapshotCtrl.h"
#include "snapshot/CEdit9527.h"

//定义唯一的一个R,UIRES对象,ROBJ_IN_CPP是resource.h中定义的宏。
ROBJ_IN_CPP

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int /*nCmdShow*/)
{
	HRESULT hRes = OleInitialize(NULL);
	SASSERT(SUCCEEDED(hRes));

	int nRet = 0;

	SComMgr *pComMgr = new SComMgr;
	TCHAR szCurrentDir[MAX_PATH] = { 0 };
	GetModuleFileName(NULL, szCurrentDir, sizeof(szCurrentDir));

	LPTSTR lpInsertPos = _tcsrchr(szCurrentDir, _T('\\'));
	_tcscpy(lpInsertPos + 1, _T("\0"));
	SStringT strRes = SStringT().Format(_T("%s\\default_res"), szCurrentDir);
	if (GetFileAttributes(strRes) == INVALID_FILE_ATTRIBUTES)
	{
		_tcscpy(lpInsertPos + 1, _T(".."));
	}
	strRes = SStringT().Format(_T("%s\\default_res"), szCurrentDir);
	SASSERT(GetFileAttributes(strRes) != INVALID_FILE_ATTRIBUTES);

	SetCurrentDirectory(szCurrentDir);
	HMODULE hRiched20 = LoadLibrary(_T("riched20.dll"));
	SASSERT(hRiched20);
	{
		BOOL bLoaded = FALSE;
		CAutoRefPtr<SOUI::IImgDecoderFactory> pImgDecoderFactory;
		CAutoRefPtr<SOUI::IRenderFactory> pRenderFactory;
		bLoaded = pComMgr->CreateRender_GDI((IObjRef**)&pRenderFactory);
		SASSERT_FMT(bLoaded, _T("load interface [render] failed!"));
		bLoaded = pComMgr->CreateImgDecoder((IObjRef**)&pImgDecoderFactory);
		SASSERT_FMT(bLoaded, _T("load interface [%s] failed!"), _T("imgdecoder"));

		pRenderFactory->SetImgDecoderFactory(pImgDecoderFactory);
		SApplication *theApp = new SApplication(pRenderFactory, hInstance);
		theApp->SetAppDir(szCurrentDir);

		//extened skins
		theApp->RegisterSkinClass<SColorMask>();
		theApp->RegisterSkinClass<SAntialiasSkin>();
		theApp->RegisterSkinClass<SSkinVScrollbar>();
		theApp->RegisterSkinClass<SSkinGif>();
		theApp->RegisterSkinClass<SSkinAPNG>();

		theApp->RegisterWindowClass<SShellNotifyIcon>();
		theApp->RegisterWindowClass<SImRichEdit>();
		theApp->RegisterWindowClass<SWindowEx>();
		theApp->RegisterWindowClass<SImageView>();
		theApp->RegisterWindowClass<SButtonEx>();
		theApp->RegisterWindowClass<SSplitBar>();
		theApp->RegisterWindowClass<SSnapshotCtrl>();
		theApp->RegisterWindowClass<CEdit9527>();

		{
			HMODULE hModSysResource = LoadLibrary(SYS_NAMED_RESOURCE);
			if (hModSysResource)
			{
				CAutoRefPtr<IResProvider> sysResProvider;
				CreateResProvider(RES_PE, (IObjRef**)&sysResProvider);
				sysResProvider->Init((WPARAM)hModSysResource, 0);
				theApp->LoadSystemNamedResource(sysResProvider);
				FreeLibrary(hModSysResource);
			}
			else
			{
				SASSERT(0);
			}
		}

		CAutoRefPtr<IResProvider>   pResProvider;

#ifdef _DEBUG		
		//选择了仅在Release版本打包资源则在DEBUG下始终使用文件加载
		{
			CreateResProvider(RES_FILE, (IObjRef**)&pResProvider);
			bLoaded = pResProvider->Init((LPARAM)_T("uires"), 0);
			if (!bLoaded)
			{
				bLoaded = pResProvider->Init((LPARAM)_T("..\\uires"), 0);
				SASSERT(bLoaded);
			}
		}
#else
		{
			CreateResProvider(RES_PE, (IObjRef**)&pResProvider);
			bLoaded = pResProvider->Init((WPARAM)hInstance, 0);
			SASSERT(bLoaded);
		}
#endif
		theApp->InitXmlNamedID(namedXmlID, ARRAYSIZE(namedXmlID), TRUE);
		theApp->AddResProvider(pResProvider);


		//处理程序所需假数据
		CGlobalUnits::GetInstance()->OperateShamDate();

		std::string strPYMapPath = S_CW2A(szCurrentDir);
		strPYMapPath += "\\default_res\\PYmap\\mapfile.txt";
		//处理汉字拼音对照表
		CGlobalUnits::GetInstance()->OperatePinyinMap(strPYMapPath);
		CGlobalUnits::GetInstance()->OperateSerachIndex();

		CGlobalUnits::GetInstance()->OperateEmojis();

		SNotifyCenter *pNotifyCenter = new SNotifyCenter;
		// BLOCK: Run application
		{
			CMainDlg dlgMain;
			dlgMain.Create(GetActiveWindow());
			dlgMain.SendMessage(WM_INITDIALOG);
			dlgMain.CenterWindow(dlgMain.m_hWnd);
			dlgMain.ShowWindow(SW_SHOWNORMAL);
			nRet = theApp->Run(dlgMain.m_hWnd);
		}
		delete pNotifyCenter;

		delete theApp;
	}

	delete pComMgr;
	FreeLibrary(hRiched20);

	OleUninitialize();
	return nRet;
}
