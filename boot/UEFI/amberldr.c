/* Copyright (C) 2023 Michał Pazurek - All Rights Reserved
 * This file is part of the "AmberOS" project
 * Unauthorized copying of this file, via any medium is strictly prohibited
 * Proprietary and confidential
 * Written by Michał Pazurek <michal10.05.pk@gmail.com>
*/

#include <efi/efi.h>
#include <efi/efilib.h>
#include <elf.h>

#include <boot/UEFI/BootDevice.h>
#include <boot/UEFI/ISO9660.h>
#include <boot/UEFI/GraphicsOutputProtocol.h>
// #include <boot/UEFI/BMP.h>
// #include <boot/UEFI/GIF.h>
// #include <boot/UEFI/timer.h>

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

    UINT8 *kernel;
    UINTN kernelSize;
    status = ReadFile(bootDeviceIO, "/AmberOS/System/amberkrn.elf", &kernelSize, (void**)&kernel);
    if (EFI_ERROR(status)) {
        Print(L"Failed to load the kernel into memory: %r\n\r", status);
        return status;
    }

    Elf64_Ehdr kernelHeader;
    memcpy(&kernelHeader, kernel, sizeof(kernelHeader));
    if (memcmp(&kernelHeader.e_ident[EI_MAG0], ELFMAG, SELFMAG) != 0 ||
		kernelHeader.e_ident[EI_CLASS] != ELFCLASS64 ||
		kernelHeader.e_ident[EI_DATA] != ELFDATA2LSB ||
		kernelHeader.e_type != ET_EXEC ||
		kernelHeader.e_machine != EM_X86_64 ||
		kernelHeader.e_version != EV_CURRENT)
	{
		Print(L"Kernel has wrong format\r\n");
        return EFI_UNSUPPORTED;
	}

    Elf64_Phdr* phdrs;
	{
		UINTN size = kernelHeader.e_phnum * kernelHeader.e_phentsize;
		uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, size, (void**)&phdrs);
		memcpy(phdrs, kernel + kernelHeader.e_phoff, size);
	}

    for (Elf64_Phdr* phdr = phdrs; (char*)phdr < (char*)phdrs + kernelHeader.e_phnum * kernelHeader.e_phentsize; phdr = (Elf64_Phdr*)((char*)phdr + kernelHeader.e_phentsize)) {
		switch (phdr->p_type) {
			case PT_LOAD: {
				int pages = (phdr->p_memsz + 0x1000 - 1) / 0x1000;
				Elf64_Addr segment = phdr->p_paddr;
				uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, pages, &segment);

				UINTN size = phdr->p_filesz;
                memcpy((void*)segment, kernel + phdr->p_offset, size);
				break;
			}
		}
	}

    void (*StartKernel)(UINT32*) = ((__attribute__((sysv_abi)) void (*)(UINT32*))kernelHeader.e_entry);

    uefi_call_wrapper(BS->ExitBootServices, 1, ImageHandle);

    StartKernel((UINT32*)graphics->Mode->FrameBufferBase);

    // If the kernel returned something went wrong
    return EFI_ABORTED;
}