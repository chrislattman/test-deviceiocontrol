#define _AMD64_
#include <wdm.h>

#include "testdriver.h"

static UNICODE_STRING name = RTL_CONSTANT_STRING(L"\\Device\\testdevice");
static UNICODE_STRING symlink = RTL_CONSTANT_STRING(L"\\??\\testdriver");
static int answer = 0;
static struct mystruct test = {0, ""};
static char *string;
static KSPIN_LOCK myspinlock;
static KIRQL irqlevel;

static NTSTATUS CreateClose(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return STATUS_SUCCESS;
}

// the return value of this function is in GetLastError() if unsuccessful
static NTSTATUS DeviceIoctl(_In_ PDEVICE_OBJECT DeviceObject, _Inout_ PIRP Irp)
{
    UNREFERENCED_PARAMETER(DeviceObject);
    PVOID inputBuffer, outputBuffer;

    NTSTATUS status = STATUS_SUCCESS;
    PIO_STACK_LOCATION stack = IoGetCurrentIrpStackLocation(Irp);

    KeAcquireSpinLock(&myspinlock, &irqlevel); // ProbeForRead and memcpy should be fast
    switch (stack->Parameters.DeviceIoControl.IoControlCode) {
    case RD_VALUE:
        outputBuffer = Irp->UserBuffer;
        __try {
            ProbeForRead(outputBuffer,
                stack->Parameters.DeviceIoControl.OutputBufferLength, sizeof(unsigned char));
            memcpy(outputBuffer, &answer, sizeof(answer));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrint("Received invalid output buffer.\n");
            status = STATUS_ACCESS_VIOLATION;
        }
        break;
    case WR_VALUE:
        inputBuffer = stack->Parameters.DeviceIoControl.Type3InputBuffer;
        __try {
            ProbeForRead(inputBuffer,
                stack->Parameters.DeviceIoControl.InputBufferLength, sizeof(unsigned char));
            memcpy(&answer, inputBuffer, sizeof(answer));
            DbgPrint("Received answer = %d\n", answer);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrint("Received invalid input buffer.\n");
            status = STATUS_ACCESS_VIOLATION;
        }
        break;
    case RD_STRUCT:
        outputBuffer = Irp->UserBuffer;
        __try {
            ProbeForRead(outputBuffer,
                stack->Parameters.DeviceIoControl.OutputBufferLength, sizeof(unsigned char));
            memcpy(outputBuffer, &test, sizeof(test));
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrint("Received invalid output buffer.\n");
            status = STATUS_ACCESS_VIOLATION;
        }
        break;
    case WR_STRUCT:
        inputBuffer = stack->Parameters.DeviceIoControl.Type3InputBuffer;
        __try {
            ProbeForRead(inputBuffer,
                stack->Parameters.DeviceIoControl.InputBufferLength, sizeof(unsigned char));
            memcpy(&test, inputBuffer, sizeof(test));
            DbgPrint("Received num = %d, name = \"%s\"\n", test.num, test.name);
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            DbgPrint("Received invalid input buffer.\n");
            status = STATUS_ACCESS_VIOLATION;
        }
        break;
    default:
        DbgPrint("Invalid request.\n");
        status = STATUS_INVALID_DEVICE_REQUEST;
    }
    KeReleaseSpinLock(&myspinlock, irqlevel);

    Irp->IoStatus.Status = status;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
    return status;
}

static void UnloadFunc(_In_ PDRIVER_OBJECT DriverObject)
{
    IoDeleteSymbolicLink(&symlink);
    IoDeleteDevice(DriverObject->DeviceObject);
    DbgPrint("Driver unloaded.\n");
}

NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);
    NTSTATUS status;
    PDEVICE_OBJECT DeviceObject;

    DriverObject->DriverUnload = UnloadFunc;
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateClose;
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = DeviceIoctl;
    KeInitializeSpinLock(&myspinlock);

    status = IoCreateDevice(DriverObject, 0, &name, FILE_DEVICE_UNKNOWN, 0, FALSE, &DeviceObject);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Error creating device.\n");
        return status;
    }

    status = IoCreateSymbolicLink(&symlink, &name);
    if (!NT_SUCCESS(status)) {
        DbgPrint("Error creating symbolic link.\n");
        IoDeleteDevice(DeviceObject);
        return status;
    }

    // Bitwise OR POOL_FLAG_UNINITIALIZED with POOL_FLAG_PAGED if zeroing the memory isn't necessary
    // Older functions: ExAllocatePoolWithTag(), ExAllocatePoolZero()
    string = ExAllocatePool2(POOL_FLAG_PAGED, 30, 'dcba');
    if (!string) {
        DbgPrint("Error allocating memory for string.\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }
    strncpy(string, "Hello from testdriver!", 30);
    DbgPrint("%s\n", string);
    ExFreePool(string);
    DbgPrint("Driver loaded.\n");
    return STATUS_SUCCESS;
}
