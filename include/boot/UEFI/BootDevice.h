#ifndef __BOOT_DEVICE_H__
#define __BOOT_DEVICE_H__

#include <efi/efi.h>
#include <efi/efiapi.h>

#include <boot/UEFI/MBR.h>

// This function gets the DeviceHandle for the ISO image used to boot this app
static EFI_STATUS GetBootDeviceHandle(EFI_HANDLE** bootDevHandle) {
    EFI_STATUS status;
    EFI_HANDLE* handles = NULL;
    UINTN handleCount = 0;

    status = uefi_call_wrapper(BS->LocateHandleBuffer, 5, ByProtocol, &gEfiBlockIoProtocolGuid, NULL, &handleCount, (void**)&handles);
    if (EFI_ERROR(status) || handleCount == 0) {
        Print(L"Failed to get the BlockIO handles: %r\n\r", status);
        return status;
    }

    for (UINTN handleIndex = 0; handleIndex < handleCount; handleIndex++) {
        EFI_BLOCK_IO_PROTOCOL* blockIo;
        status = uefi_call_wrapper(BS->HandleProtocol, 3, handles[handleIndex], &gEfiBlockIoProtocolGuid, (void**)&blockIo);
        if (!EFI_ERROR(status)) {
            AmberOS_MBR_t mbr;
            status = uefi_call_wrapper(blockIo->ReadBlocks, 5, blockIo, blockIo->Media->MediaId, 0, 512, &mbr);
            if (EFI_ERROR(status)) {
                Print(L"Couldn't validate the IO device: %r\n\r", status);
                continue;
            }

            if (mbr.os_sig[0] == 'M' && mbr.os_sig[1] == 'P') {
                *bootDevHandle = handles[handleIndex];
                return EFI_SUCCESS;
            }
        }
    }

    // If we are here, we couldn't find the boot device handle
    uefi_call_wrapper(BS->FreePool, 1, handles);
    *bootDevHandle = NULL;
    return EFI_NOT_FOUND;
}

#endif