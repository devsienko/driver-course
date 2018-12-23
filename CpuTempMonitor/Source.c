#include "ntddk.h"
#include "intrin.h"
#define DELAY_ONE_MICROSECOND (-10)
#define DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define THREAD_NUM 64

ULONG cpunum = 0;
HANDLE thread[THREAD_NUM] = { 0 };
PVOID threadobj[THREAD_NUM] = { 0 };
BOOLEAN signal = FALSE;
PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Info = NULL;
UINT64 structsize = 0;

VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	signal = TRUE;

	if (NULL != Info) {
		ExFreePool(Info);
	}

	for (int i = 0; i < cpunum && threadobj[i]; i++) {
		KeWaitForSingleObject(threadobj[i], Executive, KernelMode, FALSE, NULL);
		ObDereferenceObject(threadobj[i]);
	}

	DbgPrint("driver unload \r\n");
}

VOID MyProc(PVOID Context) 
{
	GROUP_AFFINITY groupaff = { 0 };
	ULONG index = 0x19c;
	UINT64 readout = 0;
	ULONG delta = 0;
	ULONG cputemp = 0;
	LARGE_INTEGER interval = { 0 };
	ULONG cpuid = ((UINT64)Context - (UINT64)Info) / structsize;

	interval.QuadPart = DELAY_ONE_MILLISECOND * 3000;
	groupaff = ((PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)Context)->Processor.GroupMask[0];
	//groupaff.Group = 0;
	//groupaff.Mask = (1 << (UINT64)Context);
	KeSetSystemGroupAffinityThread(&groupaff, NULL);

	while (1) {
		
		if (signal == TRUE) {
			break;
		}

		KeDelayExecutionThread(KernelMode, FALSE, &interval);

		readout = __readmsr(index);

		if ((readout & 0x80000000)) {
			delta = (readout >> 16) & 0x7F;
			cputemp = 100 - delta;
			DbgPrint("cpu %d : temp is %d \r\n", cpuid, cputemp);
		}
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID create_systhread()
{
	NTSTATUS status;

	int i;
	for (i = 0; i < cpunum; i++) {
		status = PsCreateSystemThread(&thread[i], 0, NULL, NULL, NULL, MyProc, (PVOID)((UINT64)Info+structsize*i));

		if (!NT_SUCCESS(status)) {
			DbgPrint("creating thread failed \r\n");
			break;
		}

		status = ObReferenceObjectByHandle(thread[i], THREAD_ALL_ACCESS, NULL, KernelMode, &threadobj[i],
			NULL);
		ZwClose(thread[i]);

		if (!NT_SUCCESS(status)) {
			break;
		}
	}
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;
	int cpuinfo[4] = { 0 };
	ULONG size = 0;

	DriverObject->DriverUnload = Unload;
	__cpuid(cpuinfo, 0);
	if (cpuinfo[1] != 0x756e6547 || cpuinfo[2] != 0x6c65746e || cpuinfo[3] != 0x49656e69) {
		DbgPrint("not intel cpu \r\n");
		return STATUS_UNSUCCESSFUL;
	}

	__cpuid(cpuinfo, 1);
	if ((cpuinfo[3] & 0x20) == 0) {
		DbgPrint("not support rdmsr \r\n");
		return STATUS_UNSUCCESSFUL;
	}

	__cpuid(cpuinfo, 6);

	if ((cpuinfo[0] & 1) == 0) {
		DbgPrint("not support digital thermal sensor \r\n");
		return STATUS_UNSUCCESSFUL;
	}

	status = KeQueryLogicalProcessorRelationship(NULL, RelationProcessorCore, NULL, &size);

	if (status == STATUS_INFO_LENGTH_MISMATCH) {
		Info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)ExAllocatePoolWithTag(PagedPool, size, 'abcd');

		if (Info == NULL) {
			return STATUS_UNSUCCESSFUL;
		}
		RtlZeroMemory(Info, size);
		status = KeQueryLogicalProcessorRelationship(NULL, RelationProcessorCore, Info, &size);

		if (!NT_SUCCESS(status)) {
			ExFreePool(Info);
			return STATUS_UNSUCCESSFUL;
		}

		structsize = (UINT64)(&((PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)0)->Processor) + sizeof(PROCESSOR_RELATIONSHIP);
		cpunum = size / structsize;
	}
	else {
		return STATUS_UNSUCCESSFUL;
	}

	create_systhread();

	return STATUS_SUCCESS;
}