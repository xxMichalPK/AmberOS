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
#include <boot/UEFI/configParser.h>
#include <boot/legacy/loader/intop.h>
#include <boot/bootinfo.h>

EFI_STATUS EFIAPI efi_main (EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status;
    EFI_HANDLE *bootDeviceHandle = NULL;
    EFI_BLOCK_IO_PROTOCOL *bootDeviceIO = NULL;

    EFI_GRAPHICS_OUTPUT_PROTOCOL *graphics = NULL;

    InitializeLib(ImageHandle, SystemTable);

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

    UINT8 *configFile;
    UINTN configFileSize;
    status = ReadFile(bootDeviceIO, "/AmberOS/SysConfig/boot.cfg", &configFileSize, (void**)&configFile);
    if (EFI_ERROR(status)) {
        Print(L"Failed to load the config file into memory: %r\n\r", status);
        return status;
    }

    uint8_t *vResString, *hResString;
    uint32_t vRes, hRes;
    parseConfig(configFile, "SCREEN SETTINGS\0", "VERTICAL RESOLUTION\0", &vResString);
    parseConfig(configFile, "SCREEN SETTINGS\0", "HORIZONTAL RESOLUTION\0", &hResString);
    vRes = atoui((char*)vResString);
    hRes = atoui((char*)hResString);

    UINT8 *kernel;
    UINTN kernelSize;
    status = ReadFile(bootDeviceIO, "/AmberOS/System/amberkrn.elf", &kernelSize, (void**)&kernel);
    if (EFI_ERROR(status)) {
        Print(L"Failed to load the kernel into memory: %r\n\r", status);
        return status;
    }

    status = InitializeGraphics(ImageHandle, &graphics);
    if (EFI_ERROR(status)) {
        Print(L"Failed to initialize graphics: %r\n\r", status);
        return status;
    }

    // In UEFI we always use 32 bits per pixel
    status = SetVideoMode(graphics, vRes, hRes, PixelBlueGreenRedReserved8BitPerColor);
    if (EFI_ERROR(status)) return status;

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

    // Prepare the boot information for the kernel
    BootInfo_t *bootInfo = NULL;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, sizeof(BootInfo_t), (void**)&bootInfo);
    if (EFI_ERROR(status) || !bootInfo) {
        Print(L"Failed to allocate memory for the boot info structure: %r\n\r", status);
        return status;
    }

    UINT32 gopBPP = GetGOPBitsPerPixel(graphics);
    if (!gopBPP) {
        uefi_call_wrapper(BS->FreePool, 1, bootInfo);
        return EFI_UNSUPPORTED;
    }

    bootInfo->bootType = UEFI;
    bootInfo->framebuffer.vRes = graphics->Mode->Info->VerticalResolution;
    bootInfo->framebuffer.hRes = graphics->Mode->Info->HorizontalResolution;
    bootInfo->framebuffer.size = graphics->Mode->FrameBufferSize;
    bootInfo->framebuffer.bitsPerPixel = gopBPP;
    bootInfo->framebuffer.base = graphics->Mode->FrameBufferBase;

    void (*StartKernel)(BootInfo_t*) = ((__attribute__((sysv_abi)) void (*)(BootInfo_t*))kernelHeader.e_entry);

    uefi_call_wrapper(BS->ExitBootServices, 1, ImageHandle);

    StartKernel(bootInfo);

    // If the kernel returned something went wrong
    return EFI_ABORTED;
}