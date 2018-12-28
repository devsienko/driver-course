#include <ntddk.h>
//https://www.youtube.com/watch?v=UlzctiY2NSg&index=8&list=PLZ4EgN7ZCzJyUT-FmgHsW4e9BxfP-VMuo
//16:11

typedef struct {
	PDEVICE_OBJECT LowerKbdDevice;
} DEVICE_EXTENSION, * PDEVICE_EXTENSION;

typedef struct _KEYBOARD_INPUT_DATA {
	USHORT UnitId;
	USHORT MakeCode;
	USHORT Flags;
	USHORT Reserved;
	ULONG  ExtraInformation;
} KEYBOARD_INPUT_DATA, *PKEYBOARD_INPUT_DATA;

PDEVICE_OBJECT myKbdDevice = NULL;


VOID DriverUnload(PDRIVER_OBJECT DriverObject) 
{
	PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject;
	IoDetachDevice(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerKbdDevice);
	IoDeleteDevice(myKbdDevice);
	KdPrint(("Unload Our Driver  \r\n"));
}

NTSTATUS DispatchPass(PDEVICE_OBJECT DeviceObject, PIRP Irp) 
{
	IoCopyCurrentIrpStackLocationToNext(Irp);
	return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerKbdDevice, Irp);
}

NTSTATUS ReadComplete(PDEVICE_OBJECT DeviceObject, PIRP Irp, PVOID Context)
{
	CHAR* keyflag[4] = { "KeyDown", "KeyUp", "E0", "E1" };
	PKEYBOARD_INPUT_DATA Keys = (PKEYBOARD_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;

	if (Irp->IoStatus.Status == STATUS_SUCCESS) {
		KdPrint(("the scan code is %x (%s)\n", Keys->MakeCode, keyflag[Keys->Flags]));
	}
}

NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp) 
{
	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(Irp, ReadComplete, NULL, TRUE, TRUE, TRUE);

	return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerKbdDevice, Irp);
}

NTSTATUS MyAttachDevice(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;
	UNICODE_STRING TargetDevice = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
	status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_KEYBOARD, 0, FALSE, &myKbdDevice);
	if (!NT_SUCCESS(status)) {
		return status;
	}

	myKbdDevice->Flags |= DO_BUFFERED_IO;
	myKbdDevice->Flags &= ~DO_DEVICE_INITIALIZING;

	RtlZeroMemory(myKbdDevice->DeviceExtension, sizeof(DEVICE_EXTENSION));

	status = IoAttachDevice(myKbdDevice, &TargetDevice, &((PDEVICE_EXTENSION)myKbdDevice->DeviceExtension)->LowerKbdDevice);
	if (!NT_SUCCESS(status)) {
		IoDeleteDevice(myKbdDevice);
		return status;
	}
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;
	int i;
	DriverObject->DriverUnload = DriverUnload;

	for (i = 0; i < IRP_MJ_MAXIMUM_FUNCTION; i++) {
		DriverObject->MajorFunction[i] = DispatchPass;
	}

	DriverObject->MajorFunction[IRP_MJ_READ] = DispatchRead;

	status = MyAttachDevice(DriverObject);
	if (!NT_SUCCESS(status)) {
		KdPrint(("attaching is failing \r\n"));
	}
	else {
		KdPrint(("attaching succeeds \r\n"));
	}
	return status;
}