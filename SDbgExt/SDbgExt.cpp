#include "stdafx.h"
#include "SDbgExt.h"
#include "SDbgExtApi.h"
#include "DbgEngCLRDataTarget.h"
#include "DbgEngMemoryAccess.h"
#include <DbgHelp.h>

#define CORDAC_FORMAT L"%s\\Microsoft.NET\\Framework%s\\v%s\\mscordacwks.dll"

#ifdef _M_IX86
	#define CORDAC_BITNESS L""
#else
	#define CORDAC_BITNESS L"64"
#endif

#ifdef CLR2
	#define CORDAC_CLRVER L"2.0.50727"
#else
	#define CORDAC_CLRVER L"4.0.30319"
#endif

HRESULT SDBGEXT_API CreateDbgEngMemoryAccess(IDebugDataSpaces *data, IDacMemoryAccess **ret)
{
	CComObject<DbgEngMemoryAccess> *tmp;
	CComObject<DbgEngMemoryAccess>::CreateInstance(&tmp);
	tmp->AddRef();
	tmp->Init(data);

	*ret = tmp;
	return S_OK;
}

HRESULT SDBGEXT_API CreateSDbgExt(IClrProcess *p, ISDbgExt **ext)
{
	*ext = CSDbgExt::Construct(p);
	return S_OK;
}

typedef HRESULT (__stdcall *CLRDataCreateInstancePtr)(REFIID iid, ICLRDataTarget* target, void** iface);

HRESULT SDBGEXT_API InitIXCLRData(IDebugClient *cli, IXCLRDataProcess3 **ppDac)
{
	CComPtr<IDebugSymbols3> dSym;
	CComPtr<IDebugDataSpaces> dds;
	CComPtr<IDebugSystemObjects> dso;

	HRESULT hr = S_OK;

	RETURN_IF_FAILED(cli->QueryInterface(__uuidof(IDebugSymbols3), (PVOID*)&dSym));
	RETURN_IF_FAILED(cli->QueryInterface(__uuidof(IDebugDataSpaces), (PVOID*)&dds));
	RETURN_IF_FAILED(cli->QueryInterface(__uuidof(IDebugSystemObjects), (PVOID*)&dso));

	// Init CORDAC
	WCHAR winDirBuffer[MAX_PATH] = { 0 };
	WCHAR corDacBuffer[MAX_PATH] = { 0 };

	GetWindowsDirectory(winDirBuffer, ARRAYSIZE(winDirBuffer));
	swprintf_s(corDacBuffer, CORDAC_FORMAT, winDirBuffer, CORDAC_BITNESS, CORDAC_CLRVER);

	HMODULE hCorDac = LoadLibrary(corDacBuffer);
	if (hCorDac == NULL)
	{
		return FALSE;
	}
	CLRDataCreateInstancePtr clrData = (CLRDataCreateInstancePtr)GetProcAddress(hCorDac, "CLRDataCreateInstance");
	CComObject<DbgEngCLRDataTarget> *dataTarget;
	CComObject<DbgEngCLRDataTarget>::CreateInstance(&dataTarget);
	dataTarget->AddRef();
	dataTarget->Init(dSym, dds, dso);
	
	RETURN_IF_FAILED(clrData(__uuidof(IXCLRDataProcess3), dataTarget, (PVOID*)ppDac));
		
	return S_OK;
}

HRESULT SDBGEXT_API InitFromLiveProcess(DWORD dwProcessId, ISDbgExt **ret)
{
	CComPtr<IDebugClient> cli;
	CComPtr<IDebugControl> ctrl;
		
	HRESULT hr = S_OK;

	RETURN_IF_FAILED(DebugCreate(__uuidof(IDebugClient), (PVOID*)&cli));
	RETURN_IF_FAILED(cli->AttachProcess(NULL, dwProcessId, DEBUG_ATTACH_NONINVASIVE | DEBUG_ATTACH_NONINVASIVE_NO_SUSPEND));
	RETURN_IF_FAILED(cli->QueryInterface(__uuidof(IDebugControl), (PVOID*)&ctrl));

	RETURN_IF_FAILED(ctrl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE));	

	IXCLRDataProcess3Ptr pcdp;
	RETURN_IF_FAILED(InitIXCLRData(cli, &pcdp));

	CComPtr<IDebugDataSpaces> dds;
	cli.QueryInterface<IDebugDataSpaces>(&dds);

	IDacMemoryAccessPtr dcma;
	RETURN_IF_FAILED(CreateDbgEngMemoryAccess(dds, &dcma));

	IClrProcessPtr proc;
	RETURN_IF_FAILED(CreateClrProcess(pcdp, dcma, &proc));
	return CreateSDbgExt(proc, ret);
}

HRESULT SDBGEXT_API InitFromDump(const WCHAR *dumpFile, ISDbgExt **ext)
{
	CComPtr<IDebugClient> cli;
	CComPtr<IDebugClient4> cli4;
	CComPtr<IDebugControl> ctrl;
	CComPtr<IXCLRDataProcess3> dac; 
	CComPtr<IDacMemoryAccess> dcma;
	CComPtr<IClrProcess> p;
		
	HRESULT hr = S_OK;
	RETURN_IF_FAILED(DebugCreate(__uuidof(IDebugClient), (PVOID*)&cli));
	RETURN_IF_FAILED(cli.QueryInterface<IDebugClient4>(&cli4));
	RETURN_IF_FAILED(cli.QueryInterface<IDebugControl>(&ctrl));
	RETURN_IF_FAILED(cli4->OpenDumpFileWide(dumpFile, NULL));
	RETURN_IF_FAILED(ctrl->WaitForEvent(DEBUG_WAIT_DEFAULT, INFINITE));

	RETURN_IF_FAILED(InitIXCLRData(cli, &dac));

	CComPtr<IDebugDataSpaces> dds;
	cli.QueryInterface<IDebugDataSpaces>(&dds);

	CreateDbgEngMemoryAccess(dds, &dcma);

	CreateClrProcess(dac, dcma, &p);
	CreateSDbgExt(p, ext);

	return hr;
}