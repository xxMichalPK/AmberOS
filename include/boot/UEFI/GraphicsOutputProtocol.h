#ifndef __GRAPHICS_OUTPUT_PROTOCOL_H__
#define __GRAPHICS_OUTPUT_PROTOCOL_H__

#include <efi/efi.h>
#include <efi/efilib.h>

EFI_STATUS InitializeGraphics(EFI_HANDLE ImageHandle, EFI_GRAPHICS_OUTPUT_PROTOCOL **gop) {
    EFI_STATUS status;

    UINTN gopCount;
    EFI_HANDLE *gopHandles = NULL;

    status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiGraphicsOutputProtocolGuid, NULL, &gopCount, &gopHandles);
    if (EFI_ERROR(status)) {
        Print(L"Failed to locate the GOP Handle: %r\n\r", status);
        return status;
    }

    status = uefi_call_wrapper(BS->OpenProtocol, 6, gopHandles[0], &gEfiGraphicsOutputProtocolGuid, (void**)&(*gop), ImageHandle, NULL, EFI_OPEN_PROTOCOL_BY_HANDLE_PROTOCOL);
    if (EFI_ERROR(status)) {
        Print(L"Failed to open the Graphics Output Protocol: %r\n\r", status);
        uefi_call_wrapper(BS->FreePool, 1, gopHandles);
        *gop = NULL;
        return status;
    }

    return EFI_SUCCESS;
}

EFI_STATUS SetVideoMode(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, UINTN hRes, UINTN vRes, UINTN pFormat) {
    EFI_STATUS status;
    
    UINTN modeNum;
    UINTN sizeOfInfo;
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION *gopModeInfo = NULL;
    for (modeNum = 0; (status = uefi_call_wrapper(gop->QueryMode, 4, gop, modeNum, &sizeOfInfo, &gopModeInfo)) == EFI_SUCCESS; modeNum++) {
        if (gopModeInfo->HorizontalResolution == hRes && gopModeInfo->VerticalResolution == vRes && gopModeInfo->PixelFormat == pFormat) {
            break;
        }
    }

    status = uefi_call_wrapper(gop->SetMode, 2, gop, modeNum);
    if (EFI_ERROR(status)) {
        Print(L"Failed to set the desired video mode: %r\n\r", status);
        uefi_call_wrapper(BS->FreePool, 1, gopModeInfo);
        return status;
    }

    return EFI_SUCCESS;
}

#endif