#include <ntddk.h>
//https://www.youtube.com/watch?v=JVWtFsmOkA0&index=7&list=PLZ4EgN7ZCzJyUT-FmgHsW4e9BxfP-VMuo
//8:33
PDEVICE_OBJECT myKbdDevice = NULL;

VOID DriverUnload(PDRIVER_OBJECT DriverObject) 
{
	KdPrint(("Unload Our Driver  \r\n"));
}

NTSTATUS MyAttachDevice(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;

	IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_KEYBOARD, 0, FALSE, &myKbdDevice);
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	int i;
	DriverObject->DriverUnload = DriverUnload;

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchPass;
	}

	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	KdPrint(("Unload Our Driver  \r\n"));
	MyAttachDevice(DriverObject);
	return STATUS_SUCCESS;
}