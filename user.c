#include <Windows.h>
#include <stdio.h>
#include <string.h>

#include "testdriver.h"

int main(void)
{
    HANDLE hDevice;
    int answer;
    struct mystruct test;
    DWORD numBytes;

    hDevice = CreateFileW(L"\\\\.\\testdriver", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    if (hDevice == INVALID_HANDLE_VALUE) {
        printf("Failed to open device");
        return 1;
    }

    DeviceIoControl(hDevice, RD_VALUE, &answer, sizeof(answer), NULL, 0, &numBytes, NULL);
    printf("answer = %d\n", answer);

    answer = 123;
    DeviceIoControl(hDevice, WR_VALUE, NULL, 0, &answer, sizeof(answer), &numBytes, NULL);

    DeviceIoControl(hDevice, RD_VALUE, &answer, sizeof(answer), NULL, 0, &numBytes, NULL);
    printf("New answer = %d\n", answer);

    DeviceIoControl(hDevice, RD_STRUCT, &test, sizeof(test), NULL, 0, &numBytes, NULL);
    printf("test.num = %d, test.name = \"%s\"\n", test.num, test.name);

    test.num = 5;
    strncpy(test.name, "Chris", sizeof(test.name));
    DeviceIoControl(hDevice, WR_STRUCT, NULL, 0, &test, sizeof(test), &numBytes, NULL);

    DeviceIoControl(hDevice, RD_STRUCT, &test, sizeof(test), NULL, 0, &numBytes, NULL);
    printf("New test.num = %d, test.name = \"%s\"\n", test.num, test.name);

    CloseHandle(hDevice);
    return 0;
}
