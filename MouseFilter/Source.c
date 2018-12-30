#include <ntddk.h>

NTSTATUS ObReferenceObjectByName(
	IN PUNICODE_STRING ObjectName,
	IN ULONG Attributes,
	IN PACCESS_STATE AccessState,
	IN ACCESS_MASK  DesiredAccess,
	IN POBJECT_TYPE ObjectType,
	IN KPROCESSOR_MODE AccessMode,
	IN PVOID ParseContext,
	OUT PVOID *Object
);

typedef struct {
	PDEVICE_OBJECT LowerKbdDevice;
} DEVICE_EXTENSION, *PDEVICE_EXTENSION;

extern POBJECT_TYPE *IoDriverObjectType;

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

	while (DeviceObject) {

		IoDetachDevice(((PDEVICE_EXTENSION)DeviceObject->DeviceExtension)->LowerKbdDevice);
		DeviceObject = DeviceObject->NextDevice;
	}

	while (pendingkey) {
		KeDelayExecutionThread(KernelMode, FALSE, &interval);
	}

	DeviceObject = DriverObject->DeviceObject;
	while (DeviceObject) {
		IoDeleteDevice(myKbdDevice);
		DeviceObject = DeviceObject->NextDevice;
	}
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
	UNICODE_STRING MCName = RTL_CONSTANT_STRING(L"\\Driver\\mouclass");
	PDRIVER_OBJECT targetDriverObject = NULL;
	PDEVICE_OBJECT currentDeviceObject = NULL;
	PDEVICE_OBJECT myDeviceObject = NULL;
	status = ObReferenceObjectByName(&MCName, OBJ_CASE_INSENSITIVE, NULL, 0, *IoDriverObjectType, KernelMode, NULL, (PVOID*)&targetDriverObject);
	
	if (!NT_SUCCESS(status)) {
		KdPrint(("ObReference failed \r\n"));
		return status;
	}

	ObDereferenceObject(targetDriverObject);

	currentDeviceObject = targetDriverObject->DeviceObject;

	while (currentDeviceObject) {

		status = IoCreateDevice(DriverObject, sizeof(DEVICE_EXTENSION), NULL, FILE_DEVICE_MOUSE, 0, FALSE, &myDeviceObject);
		if (!NT_SUCCESS(status)) {
			//do your work
			return status;
		}
		RtlZeroMemory(myKbdDevice->DeviceExtension, sizeof(DEVICE_EXTENSION));
		status = IoAttachDeviceToDeviceStackSafe(myDeviceObject, currentDeviceObject, &((PDEVICE_EXTENSION)myKbdDevice->DeviceExtension)->LowerKbdDevice);
		
		if (!NT_SUCCESS(status)) {
			//IoDeleteDevice(myKbdDevice);
			//do your work
			return status;
		}


		myKbdDevice->Flags |= DO_BUFFERED_IO;
		myKbdDevice->Flags &= ~DO_DEVICE_INITIALIZING;

		currentDeviceObject = currentDeviceObject->NextDevice;
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