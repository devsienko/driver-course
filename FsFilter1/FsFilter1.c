
#include <fltKernel.h>
#include <dontuse.h>
#include <suppress.h>

PFLT_PORT port = NULL;
PFLT_PORT ClientPort = NULL;
PFLT_FILTER FilterHandle = NULL;
NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags);
FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags);
FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);
FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext);

const FLT_OPERATION_REGISTRATION Callbacks[] = {
	{IRP_MJ_CREATE, 0, MiniPreCreate, MiniPostCreate},
	{IRP_MJ_WRITE, 0, MiniPreWrite, NULL},
	{IRP_MJ_OPERATION_END}
};

const FLT_REGISTRATION FilterRegistration = {
sizeof(FLT_REGISTRATION),
FLT_REGISTRATION_VERSION,
0,
NULL,
Callbacks,
MiniUnload,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL,
NULL
};

NTSTATUS MiniUnload(FLT_FILTER_UNLOAD_FLAGS Flags)
{
	KdPrint(("driver unload \r\n"));
	FltCloseCommunicationPort(port);
	FltUnregisterFilter(FilterHandle);

	return STATUS_SUCCESS;
}

FLT_POSTOP_CALLBACK_STATUS MiniPostCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext, FLT_POST_OPERATION_FLAGS Flags)
{
	KdPrint(("post create is running \r\n"));
	return FLT_POSTOP_FINISHED_PROCESSING;
}

FLT_PREOP_CALLBACK_STATUS MiniPreCreate(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	PFLT_FILE_NAME_INFORMATION FileNameInfo;
	NTSTATUS status;
	WCHAR Name[200] = { 0 };

	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

	if (NT_SUCCESS(status)) {

		status = FltParseFileNameInformation(FileNameInfo);

		if (NT_SUCCESS(status)) {

			if (FileNameInfo->Name.MaximumLength < 260) {

				RtlCopyMemory(Name, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength);
				KdPrint(("create file: %ws \r\n", Name));
			}
		}

		FltReleaseFileNameInformation(FileNameInfo);
	}

	return FLT_PREOP_SUCCESS_WITH_CALLBACK;
}

FLT_PREOP_CALLBACK_STATUS MiniPreWrite(PFLT_CALLBACK_DATA Data, PCFLT_RELATED_OBJECTS FltObjects, PVOID* CompletionContext)
{
	PFLT_FILE_NAME_INFORMATION FileNameInfo;
	NTSTATUS status;
	WCHAR Name[200] = { 0 };

	status = FltGetFileNameInformation(Data, FLT_FILE_NAME_NORMALIZED | FLT_FILE_NAME_QUERY_DEFAULT, &FileNameInfo);

	if (NT_SUCCESS(status)) {

		status = FltParseFileNameInformation(FileNameInfo);

		if (NT_SUCCESS(status)) {

			if (FileNameInfo->Name.MaximumLength < 260) {

				RtlCopyMemory(Name, FileNameInfo->Name.Buffer, FileNameInfo->Name.MaximumLength);
				_wcsupr(Name);
				if (wcsstr(Name, L"OPENME.TXT") != NULL) {
					KdPrint(("write file: %ws blocked \r\n", Name));
					Data->IoStatus.Status = STATUS_INVALID_PARAMETER;
					Data->IoStatus.Information = 0;
					FltReleaseFileNameInformation(FileNameInfo);
					return FLT_PREOP_COMPLETE;
				}
				KdPrint(("create file: %ws \r\n", Name));
			}
		}

		FltReleaseFileNameInformation(FileNameInfo);
	}

	return FLT_PREOP_SUCCESS_NO_CALLBACK;
}

VOID MiniDisconnect(PVOID connectioncookie) 
{
	KdPrint(("disconect \r\n"));
	FltCloseClientPort(FilterHandle, &ClientPort);
}

NTSTATUS MiniConnect(PFLT_PORT clientport, PVOID serverportcookie, PVOID Context, ULONG size, PVOID Connectioncookie)
{
	ClientPort = clientport;
	KdPrint(("connect \r\n"));
	return STATUS_SUCCESS;
}

NTSTATUS MiniSendRec(PVOID portcookie, PVOID InputBuffer, ULONG InputBufferLength, PVOID OutputBuffer, ULONG OutputBufferLength, PULONG RetLength)
{
	PCHAR msg = "kernel msg";
	KdPrint(("user msg is : %s \r\n", (PCHAR)InputBuffer));

	strcpy((PCHAR)OutputBuffer, msg);
	return STATUS_SUCCESS;
}

NTSTATUS DriverEntry(PDRIVER_OBJECT DriverObject, PUNICODE_STRING RegistryPath)
{
	NTSTATUS status;
	PSECURITY_DESCRIPTOR sd;
	OBJECT_ATTRIBUTES oa = { 0 };
	UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\mf");

	status = FltRegisterFilter(DriverObject, &FilterRegistration, &FilterHandle);

	if (!NT_SUCCESS(status)) {
		return status;
	}

	status = FltBuildDefaultSecurityDescriptor(&sd, FLT_PORT_ALL_ACCESS);

	if (NT_SUCCESS(status)) {

		InitializeObjectAttributes(&oa, &name, OBJ_KERNEL_HANDLE|OBJ_CASE_INSENSITIVE, NULL, sd);
		status = FltCreateCommunicationPort(FilterHandle, &port, &oa, NULL, MiniConnect, MiniDisconnect, MiniSendRec, 1);

		FltFreeSecurityDescriptor(sd);

		if (NT_SUCCESS(status)) {

			status = FltStartFiltering(FilterHandle);

			if (!NT_SUCCESS(status)) {

				return status;

			}
			FltCloseCommunicationPort(port);
		}

		FltUnregisterFilter(FilterHandle);
	}	

	return status;
}