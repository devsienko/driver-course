//18-18 https://www.youtube.com/watch?v=5bTAYQeKY6k&list=PLZ4EgN7ZCzJyUT-FmgHsW4e9BxfP-VMuo&index=5

#include "ntddk.h"
#include "intrin.h"
#define DELAY_ONE_MICROSECOND (-10)
#define DELAY_ONE_MILLISECOND (DELAY_ONE_MICROSECOND*1000)
#define THREAD_NUM 64

#define DEVICE_SEND CTL_CODE(FILE_DEVICE_UNKNOWN, 0x801, METHOD_BUFFERED, FILE_WRITE_DATA)
#define DEVICE_REC CTL_CODE(FILE_DEVICE_UNKNOWN, 0x802, METHOD_BUFFERED, FILE_READ_DATA)

UNICODE_STRING DeviceName = RTL_CONSTANT_STRING(L"\\Device\\mydevice123");
UNICODE_STRING SymLinkName = RTL_CONSTANT_STRING(L"\\??\\mydevicelink123");
PDEVICE_OBJECT DeviceObject = NULL;
KSEMAPHORE Se;
ULONG datanum = 0;
ULONG cpunum = 0;
HANDLE thread[THREAD_NUM] = { 0 };
PVOID threadobj[THREAD_NUM] = { 0 };
BOOLEAN signal = FALSE;
PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Info = NULL;
UINT64 structsize = 0;
KSPIN_LOCK datalock;

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

	IoDeleteSymbolicLink(&SymLinkName);
	IoDeleteDevice(DeviceObject);
	DbgPrint("driver unload \r\n");
}

NTSTATUS DispatchPassThru(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;

	switch (irpsp->MajorFunction) {
	case IRP_MJ_CREATE:
		KdPrint(("create request \r\n"));
		break;
	case IRP_MJ_CLOSE:
		KdPrint(("close request \r\n"));
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
		break;
	}

	Irp->IoStatus.Information = 0;
	Irp->IoStatus.Status = status;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

NTSTATUS DispatchDevCTL(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	PIO_STACK_LOCATION irpsp = IoGetCurrentIrpStackLocation(Irp);
	NTSTATUS status = STATUS_SUCCESS;
	ULONG returnLength = 0;
	PVOID buffer = Irp->AssociatedIrp.SystemBuffer;
	ULONG inLength = irpsp->Parameters.DeviceIoControl.InputBufferLength;
	ULONG outLength = irpsp->Parameters.DeviceIoControl.OutputBufferLength;
	WCHAR* demo = L"sample returned from driver";
	LARGE_INTEGER interval = { 0 };

	interval.QuadPart = DELAY_ONE_MILLISECOND * 2;
	switch (irpsp->Parameters.DeviceIoControl.IoControlCode)
	{
	case DEVICE_SEND:
		if (!strncmp(buffer, "start", 6)) {
			KdPrint(("start \r\n"));
			create_systhread();
		}
		break;
	case DEVICE_REC:
		KeReleaseSemaphore(&Se, 0, cpunum, FALSE);
		while (1) {
			KeDelayExecutionThread(KernelMode, FALSE, &interval);
			if (datanum == cpunum) {
				break;
			}
		}
		break;
	default:
		status = STATUS_INVALID_PARAMETER;
	}

	Irp->IoStatus.Status = status;
	Irp->IoStatus.Information = returnLength;
	IoCompleteRequest(Irp, IO_NO_INCREMENT);
	return status;
}

VOID MyProc(PVOID Context)
{
	GROUP_AFFINITY groupaff = { 0 };
	ULONG index = 0x19c;
	UINT64 readout = 0;
	ULONG delta = 0;
	ULONG cputemp = 0;
	ULONG cpuid = ((UINT64)Context - (UINT64)Info) / structsize;
	KIRQL oirql;
	groupaff = ((PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)Context)->Processor.GroupMask[0];
	KeSetSystemGroupAffinityThread(&groupaff, NULL);

	while (1) {

		KeWaitForSingleObject(&Se, Executive, KernelMode, 0, NULL);
		if (signal == TRUE) {
			break;
		}

		readout = __readmsr(index);

		if ((readout & 0x80000000)) {
			delta = (readout >> 16) & 0x7F;
			cputemp = 100 - delta;
			DbgPrint("cpu %d : temp is %d \r\n", cpuid, cputemp);

			KeAcquireSpinLock(&datalock, &oirql);
			datanum++;
			KeReleaseSpinLock(&datalock, &oirql);
		}
	}

	PsTerminateSystemThread(STATUS_SUCCESS);
}

VOID create_systhread()
{
	NTSTATUS status;

	int i;
	for (i = 0; i < cpunum; i++) {
		status = PsCreateSystemThread(&thread[i], 0, NULL, NULL, NULL, MyProc, (PVOID)((UINT64)Info + structsize * i));

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
	int i;

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

	KeInitializeSemaphore(&Se, 0, cpunum);
	KeInitializeSpinLock(&datalock);
	status = IoCreateDevice(DriverObject, 0, &DeviceName, FILE_DEVICE_UNKNOWN, FILE_DEVICE_SECURE_OPEN, FALSE, &DeviceObject);

	if (!NT_SUCCESS(status)) {
		KdPrint(("creating device failed \r\n"));
		return status;
	}

	status = IoCreateSymbolicLink(&SymLinkName, &DeviceName);

	if (!NT_SUCCESS(status)) {
		KdPrint(("creating symbolic link failed \r\n"));
		IoDeleteDevice(DriverObject);
		return status;
	}

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchPassThru;
	}

	DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DispatchDevCTL;

	return STATUS_SUCCESS;
}