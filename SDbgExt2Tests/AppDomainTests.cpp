#include "stdafx.h"
#include "CppUnitTest.h"
#include "..\SDbgExt2\inc\ClrProcess.h"
#include "TestCommon.h"

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SDbgExt2Tests2
{		
	TEST_CLASS(AppDomainTests)
	{
	public:
		ADD_BASIC_TEST_INIT
		
		TEST_METHOD(ClrAppDomainStoreData_Basic)
		{
			ClrAppDomainStoreData ads = {};
			auto hr = p->GetProcess()->GetAppDomainStoreData(&ads);	

			Assert::AreEqual(S_OK, hr);
			Assert::AreEqual((LONG)1, ads.DomainCount);
			Assert::IsTrue(ads.SharedDomain != 0);
			Assert::IsTrue(ads.SystemDomain != 0);
		}

		TEST_METHOD(ClrAppDomainList_Basic)
		{
			ClrAppDomainStoreData ads = {};
			auto proc = p->GetProcess();
			auto hr = proc->GetAppDomainStoreData(&ads);

			ULONG32 numDomains = ads.DomainCount + 2;
			auto domains = std::vector<CLRDATA_ADDRESS>(numDomains);
	
			hr = proc->GetAppDomainList(ads.DomainCount, domains.data() + 2, 0);
	
			ASSERT_SOK(hr);
			ASSERT_NOT_ZERO(domains[2]);
		}

		TEST_METHOD(ClrAssemblyList_Basic)
		{
			CLRDATA_ADDRESS domain;
			auto proc = p->GetProcess();
			auto hr = proc->GetAppDomainList(1, &domain, 0);

			ClrAppDomainData adData = {};
			hr = proc->GetAppDomainData(domain, &adData);

			auto asms = std::vector<CLRDATA_ADDRESS>(adData.AssemblyCount);
			hr = proc->GetAssemblyList(domain, adData.AssemblyCount, asms.data(), 0);

			ASSERT_SOK(hr);
			ASSERT_NOT_ZERO(asms[0]);
		}

		TEST_METHOD(ClrAppDomainData_Basic)
		{
			CLRDATA_ADDRESS domain;
			auto proc = p->GetProcess();
			auto hr = proc->GetAppDomainList(1, &domain, 0);

			ClrAppDomainData adData = {};
			hr = proc->GetAppDomainData(domain, &adData);

			ASSERT_SOK(hr);
			ASSERT_NOT_ZERO(adData.AppDomainPtr);
			ASSERT_EQUAL((LONG)3, adData.AssemblyCount);
			ASSERT_EQUAL((LONG)0, adData.FailedAssemblyCount);
			ASSERT_EQUAL((int)STAGE_OPEN, (int)adData.AppDomainStage);
		}
	};
}
