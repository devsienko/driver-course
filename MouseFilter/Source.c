#include <ntddk.h>

typedef struct {
	PDEVICE_OBJECT LowerKbdDevice;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

typedef struct _MOUSE_INPUT_DATA {
	USHORT UnitId;
	USHORT Flags;
	union {
		ULONG Buttons;
		struct {
			USHORT ButtonFlags;
			USHORT ButtonData;
		};
	};
	ULONG  RawButtons;
	LONG   LastX;
	LONG   LastY;
	ULONG  ExtraInformation;
} MOUSE_INPUT_DATA, *PMOUSE_INPUT_DATA;


PDEVICE_OBJECT myKbdDevice = NULL;
ULONG pendingkey = 0;

VOID DriverUnload(PDRIVER_OBJECT DriverObject)
{
	LARGE_INTEGER interval = { 0 };
	PDEVICE_OBJECT DeviceObject = DriverObject->DeviceObject;
	interval.QuadPart = -10 * 1000 * 1000;
	IoDetachDevice(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerKbdDevice);
	while (pendingkey) {
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
	}
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
	PMOUSE_INPUT_DATA Keys = (PMOUSE_INPUT_DATA)Irp->AssociatedIrp.SystemBuffer;
	int structnum = Irp->IoStatus.Information / sizeof(MOUSE_INPUT_DATA);
	int i;
	if (Irp->IoStatus.Status == STATUS_SUCCESS) {
		for (i = 0; i < structnum; i++) {
			KdPrint(("the button state is %x \n", Keys->ButtonFlags));
		}
	}

	if (Irp->PendingReturned) {
		IoMarkIrpPending(Irp);
	}
	pendingkey--;
	return Irp->IoStatus.Status;
}

NTSTATUS DispatchRead(PDEVICE_OBJECT DeviceObject, PIRP Irp)
{
	IoCopyCurrentIrpStackLocationToNext(Irp);

	IoSetCompletionRoutine(Irp, ReadComplete, NULL, TRUE, TRUE, TRUE);

	pendingkey++;

	return IoCallDriver(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerKbdDevice, Irp);
}

NTSTATUS MyAttachDevice(PDRIVER_OBJECT DriverObject)
{
	NTSTATUS status;
	UNICODE_STRING TargetDevice = RTL_CONSTANT_STRING(L"\\Device\\PointerClass0");
	status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_MOUSE, 0, FALSE, &myKbdDevice);
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