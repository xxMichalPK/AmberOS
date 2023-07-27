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

    UINT8 *anim0, *anim12, *anim25, *anim37, *anim50, *anim62, *anim75, *anim87, *anim100, *anim112, *anim125, *anim137, *anim150, *anim162, *anim175, *anim187;
    UINTN fileSize;
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/0.bmp", &fileSize, (void**)&anim0);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/12.bmp", &fileSize, (void**)&anim12);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/25.bmp", &fileSize, (void**)&anim25);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/37.bmp", &fileSize, (void**)&anim37);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/50.bmp", &fileSize, (void**)&anim50);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/62.bmp", &fileSize, (void**)&anim62);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/75.bmp", &fileSize, (void**)&anim75);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/87.bmp", &fileSize, (void**)&anim87);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/100.bmp", &fileSize, (void**)&anim100);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/112.bmp", &fileSize, (void**)&anim112);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/125.bmp", &fileSize, (void**)&anim125);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/137.bmp", &fileSize, (void**)&anim137);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/150.bmp", &fileSize, (void**)&anim150);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/162.bmp", &fileSize, (void**)&anim162);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/175.bmp", &fileSize, (void**)&anim175);
    status = ReadFile(bootDeviceIO, "/AmberOS/Loading Animation/187.bmp", &fileSize, (void**)&anim187);
    if (EFI_ERROR(status)) {
        Print(L"Failed to read file: %r\n\r", status);
        return status;
    }

    UINT32 logoWidth, logoHeight;
    status = GetBMPSize(anim0, &logoWidth, &logoHeight);
    if (EFI_ERROR(status)) {
        Print(L"Failed to get logo size: %r\n\r", status);
        return status;
    }

    while (status == EFI_SUCCESS) {
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim100, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(5*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim112, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim125, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim137, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim150, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim162, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim175, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim187, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim0, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(5*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim12, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim25, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim37, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim50, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim62, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim75, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
        status = DrawRect(graphics, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2), logoWidth, logoHeight, 0x00000000);
        status = DrawBMP(graphics, anim87, (graphics->Mode->Info->HorizontalResolution / 2) - (logoWidth / 2), (graphics->Mode->Info->VerticalResolution / 2) - (logoHeight / 2));
        status = sleep(1*1000*1000);
    }

    for (;;);

    return EFI_SUCCESS;
}