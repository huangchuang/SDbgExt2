#include "stdafx.h"
#include "CppUnitTest.h"
#include "TestCommon.h"
#include <vector>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace SDbgExt2Tests2
{
	TEST_CLASS(ThreadTests)
	{
	public:
		ADD_BASIC_TEST_INIT

		TEST_METHOD(ClrThreadStoreData_Basic)
		{
			ClrThreadStoreData tsData;
			auto hr = p->GetProcess()->GetThreadStoreData(&tsData);

			ASSERT_SOK(hr);
			ASSERT_EQUAL((DWORD)BITNESS_CONDITIONAL(3, 2), tsData.ThreadCount);
			ASSERT_NOT_ZERO(tsData.FirstThreadObj);
		}

		TEST_METHOD(ClrThreadData_Basic)
		{
			ClrThreadData tData = {};
			ClrThreadStoreData tsData = {};
			CComPtr<IXCLRDataProcess3> proc = p->GetProcess();

			auto hr = proc->GetThreadStoreData(&tsData);
			hr = proc->GetThreadData(tsData.FirstThreadObj, &tData);

			ASSERT_SOK(hr);
			ASSERT_EQUAL((DWORD)1, tData.CorThreadId);
			ASSERT_EQUAL((DWORD)0x2a020, tData.State);
			ASSERT_EQUAL((DWORD)1, tData.LockCount);
		}

		TEST_METHOD(ClrThreadData_Iterator)
		{
			ClrThreadData tData = {};
			ClrThreadStoreData tsData = {};
			CComPtr<IXCLRDataProcess3> proc = p->GetProcess();

			auto hr = proc->GetThreadStoreData(&tsData);
			int threads = 0;
			CLRDATA_ADDRESS currThread = tsData.FirstThreadObj;
			do
			{
				hr = proc->GetThreadData(currThread, &tData);
	
				ASSERT_SOK(hr);

				currThread = tData.NextThread;
				threads++;
			} while (tData.NextThread != 0);

			ASSERT_EQUAL(BITNESS_CONDITIONAL(3, 2), threads);
		}

		TEST_METHOD(EnumThreads_Basic)
		{
			struct EnumThreadState
			{
				int NumTimesCalled;
			};

			EnumThreadState state = {};

			auto func = [](CLRDATA_ADDRESS threadObj, ClrThreadData threadData, PVOID state)->BOOL {
				((EnumThreadState*)state)->NumTimesCalled++;
				return TRUE;
			};

			p->EnumThreads(func, &state);

			ASSERT_EQUAL(BITNESS_CONDITIONAL(3,2), state.NumTimesCalled);
		}

		TEST_METHOD(FindThreadByCorThreadId_Basic)
		{
			CLRDATA_ADDRESS threadObj = 0;
			auto hr = p->FindThreadByCorThreadId(1, &threadObj);

			ASSERT_SOK(hr);
			ASSERT_NOT_ZERO(threadObj);
			Assert::AreEqual((CLRDATA_ADDRESS)BITNESS_CONDITIONAL(0x0149de68, 0x0000000000c3e790), threadObj);
		}

		TEST_METHOD(GetThreadStackInfo_Basic)
		{
			CLRDATA_ADDRESS threadObj = 0;
			auto hr = p->FindThreadByCorThreadId(1, &threadObj);
			ClrThreadData tData = {};
			p->GetProcess()->GetThreadData(threadObj, &tData);

			CLRDATA_ADDRESS stackBase, stackLimit;
			hr = p->GetDataAccess()->GetThreadStack(tData.osThreadId, &stackBase, &stackLimit);

			ASSERT_SOK(hr);
			Assert::IsTrue(stackBase > stackLimit);
		}

		TEST_METHOD(EnumStackObjects_Basic)
		{
			std::vector<ClrObjectData> seenObjects;

			auto cb = [](CLRDATA_ADDRESS object, ClrObjectData objData, PVOID state)->BOOL {
				auto so = (std::vector<ClrObjectData>*)state;

				printf("%lx\r\n", object);
				if (object == 0x02ec23ec)
				{
					so->push_back(objData);
				}
				return TRUE;
			};

			auto hr = p->EnumStackObjects((DWORD)1, cb, &seenObjects);

			ASSERT_SOK(hr);
		}

		TEST_METHOD(GetThreadContext)
		{
			ClrThreadContext ctx;
			auto hr = p->GetThreadExecutionContext((CLRDATA_ADDRESS)BITNESS_CONDITIONAL(0x02ec4148, 0x0000000002ec5cd8), &ctx);

			ASSERT_SOK(hr);
		}

		TEST_METHOD(GetManagedThreadObject)
		{
			CLRDATA_ADDRESS managedObj;
			auto hr = p->GetManagedThreadObject((CLRDATA_ADDRESS)BITNESS_CONDITIONAL(0x0149de68, 0x0000000000c3e790), &managedObj);

			ASSERT_SOK(hr);
			Assert::AreEqual((CLRDATA_ADDRESS)BITNESS_CONDITIONAL(0x02ec4148, 0x0000000002ec5cd8), managedObj);
		}

		TEST_METHOD(BeginEnumThreads_BigBlock)
		{
			CComPtr<IEnumClrThreads> t;
			p->BeginEnumThreads(&t);

			EnumClrThreadData data[5];
			ULONG numItems;
			auto hr = t->Next(5, data, &numItems);
			
			Assert::AreEqual(S_FALSE, hr);
			Assert::AreEqual((ULONG)3, numItems);
		}

		TEST_METHOD(BeginEnumThreads_OneAtATime)
		{
			CComPtr<IEnumClrThreads> t;
			p->BeginEnumThreads(&t);

			EnumClrThreadData data;
			ULONG numItems = 0;
			while(t->Next(1, &data, NULL) == S_OK)
			{
				numItems++;
			}
		
			Assert::AreEqual((ULONG)3, numItems);
			Assert::AreEqual((CLRDATA_ADDRESS)0, data.ThreadData.NextThread);
		}

	};
}