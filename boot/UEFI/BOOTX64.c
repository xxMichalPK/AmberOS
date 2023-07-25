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

    ISO_Primary_Volume_Descriptor_t *pvd;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, ISO_SECTOR_SIZE, (void**)&pvd);
    if (EFI_ERROR(status)) {
        Print(L"Failed to allocate memory for the Primary Volume Descriptor: %r\n\r", status);
        return status;
    }

    status = uefi_call_wrapper(bootDeviceIO->ReadBlocks, 5, bootDeviceIO, bootDeviceIO->Media->MediaId, ISO_LBA(16), ISO_SECTOR_SIZE, &(*pvd));
    if (EFI_ERROR(status)) {
        Print(L"Failed to read the boot device: %r\n\r", status);
        return status;
    }

    UINT8 *rawPathTable = NULL;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, ((pvd->pathTableSize[0] / 512) + 1) * 512, (void**)&rawPathTable);
    if (EFI_ERROR(status)) {
        Print(L"Failed to allocate memory for the Path Table: %r\n\r", status);
        return status;
    }

    status = uefi_call_wrapper(bootDeviceIO->ReadBlocks, 5, bootDeviceIO, bootDeviceIO->Media->MediaId, ISO_LBA(pvd->L_PathTableLBA), ((pvd->pathTableSize[0] / 512) + 1) * 512, &(*rawPathTable));
    if (EFI_ERROR(status)) {
        Print(L"Failed to read the Path Table: %r\n\r", status);
        return status;
    }

    ISO_PathTableEntry_t *pathTable = NULL;
    UINTN pathTableEntryCount;
    status = ParsePathTable(rawPathTable, pvd->pathTableSize[0], &pathTableEntryCount, &pathTable);
    if (EFI_ERROR(status)) {
        Print(L"Failed to parse the Path Table: %r\n\r", status);
        return status;
    }

    // Now when we have the path table in a nice structure we can free the memory of rawPathTable
    uefi_call_wrapper(BS->FreePool, 1, rawPathTable);

    for (int i = 0; i < pathTableEntryCount; i++) {
        //Print(L"Found directory: %a\n\r", pathTable[i].dirName);
        if (memcmp(pathTable[i].dirName, "boot\0", 5) == 0) {
            Print(L"Found boot directory!\n\r");
            UINT8 *bootDirData = NULL;

            status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, ISO_SECTOR_SIZE, (void**)&bootDirData);
            if (EFI_ERROR(status)) {
                Print(L"Failed to allocate memory for the directory: %r\n\r", status);
                return status;
            }
            
            status = uefi_call_wrapper(bootDeviceIO->ReadBlocks, 5, bootDeviceIO, bootDeviceIO->Media->MediaId, ISO_LBA(pathTable[i].extentLBA), ISO_SECTOR_SIZE, &(*bootDirData));
            if (EFI_ERROR(status)) {
                Print(L"Failed to read the directory: %r\n\r", status);
                return status;
            }

            Print(L"Data:\n\r");
            for (int p = 0; p < ISO_SECTOR_SIZE; p++) {
                Print(L"%c", bootDirData[p]);
            }

            Print(L"\n\rEnd of data\n\r");
        }
    }

    for (;;);

    return EFI_SUCCESS;
}