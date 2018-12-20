#include "ntddk.h"
#include "intrin.h"

VOID Unload(IN PDRIVER_OBJECT DriverObject)
{
	DbgPrint("driver unload \r\n");
}

NTSTATUS DriverEntry(IN PDRIVER_OBJECT DriverObject, IN PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;
	int cpuinfo[4] = { 0 };

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

	return STATUS_SUCCESS;
}