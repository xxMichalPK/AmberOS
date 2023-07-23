/* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
 * This file is part of the "AmberOS" project
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Michał Pazurek <michal10.05.pk@gmail.com>
*/

#include <efi/efi.h>
#include <efi/efilib.h>

EFI_STATUS efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    Print(L"Booting in UEFI mode comming soon...\n\r");

    for (;;);

    return EFI_SUCCESS;
}