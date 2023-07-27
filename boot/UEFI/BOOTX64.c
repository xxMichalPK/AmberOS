/* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
 * This file is part of the "AmberOS" project
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Michał Pazurek <michal10.05.pk@gmail.com>
*/

#include <efi/efi.h>
#include <efi/efilib.h>

#include <boot/UEFI/BootDevice.h>
#include <boot/UEFI/ISO9660.h>
#include <boot/UEFI/GraphicsOutputProtocol.h>
#include <boot/UEFI/BMP.h>
#include <boot/UEFI/timer.h>

EFI_STATUS EFIAPI efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;
    EFI_HANDLE *bootDeviceHandle = NULL;
    EFI_BLOCK_IO_PROTOCOL *bootDeviceIO = NULL;

    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics = NULL;

    InitializeLib(ImageHandle, SystemTable);

    status = InitializeGraphics(ImageHandle, &graphics);
    if (EFI_ERROR(status)) {
        Print(L"Failed to initialize graphics: %r\n\r", status);
        return status;
    }

    status = SetVideoMode(graphics, 1920, 1080, PixelBlueGreenRedReserved8BitPerColor);
    if (EFI_ERROR(status)) return status;

    status = GetBootDeviceHandle(&bootDeviceHandle);
    if (EFI_ERROR(status) || !bootDeviceHandle) {
        Print(L"Failed to get the boot device handle: %r\n\r", status);
        return status;
    }

    status = uefi_call_wrapper(BS->HandleProtocol, 3, bootDeviceHandle, &gEfiBlockIoProtocolGuid, &bootDeviceIO);
    if (EFI_ERROR(status) || !bootDeviceIO) {
        Print(L"Failed to get the boot device IO protocol: %r\n\r", status);
        return status;
    }

    status = InitializeISO_FS(bootDeviceIO);
    if (EFI_ERROR(status) || !bootDeviceIO) {
        Print(L"Failed to initialize the ISO File System: %r\n\r", status);
        return status;
    }

    UINT8 *fileBuffer;
    UINTN fileSize;
    status = ReadFile(bootDeviceIO, "/AmberOS/LoadingLogo.bmp", &fileSize, (void**)&fileBuffer);
    if (EFI_ERROR(status)) {
        Print(L"Failed to read file: %r\n\r", status);
        return status;
    }

    UINT32 logoWidth, logoHeight;
    status = GetBMPSize(fileBuffer, &logoWidth, &logoHeight);
    if (EFI_ERROR(status)) {
        Print(L"Failed to get logo size: %r\n\r", status);
        return status;
    }

    status = DrawBMP(graphics, fileBuffer, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
    if (EFI_ERROR(status)) {
        Print(L"Failed to draw logo: %r\n\r", status);
        return status;
    }

    Print(L"Waiting...\n\r");
    status = sleep_s(5);
    if (EFI_ERROR(status)) {
        Print(L"Sleep failed to execute: %r\n\r", status);
        return status;
    }

    Print(L"Continuing...");

    for (;;);

    return EFI_SUCCESS;
}