#ifndef __ISO9660_H__
#define __ISO9660_H__

#include <efi/efi.h>
#include <efi/efilib.h>
#include <boot/UEFI/memory.h>

#define ISO_SECTOR_SIZE 2048
#define ISO_LBA(x) (x * (ISO_SECTOR_SIZE / 512))

typedef struct ISO_Volume_Descriptor_Header {
    UINT8 type;
    UINT8 identifier[5];
    UINT8 version;
} ISO_Volume_Descriptor_Header_t;

typedef struct ISO_Primary_Volume_Descriptor {
    ISO_Volume_Descriptor_Header_t header;
    UINT8 unused1;
    UINT8 systemID[32];
    UINT8 volumeID[32];
    UINT8 unused2[8];
    UINT32 volumeSpaceSize[2];
    UINT8 unused3[32];
    UINT16 volumeSetSize[2];
    UINT16 volumeSequenceNumber[2];
    UINT16 logicalBlockSize[2];
    UINT32 pathTableSize[2];
    UINT32 L_PathTableLBA;
    UINT32 L_PathTableLBA_optional;
    UINT32 M_PathTableLBA;
    UINT32 M_PathTableLBA_optional;
    UINT8 rootDirEntry[34];
    UINT8 volumeSetID[128];
    UINT8 publisherID[128];
    UINT8 dataPreparerID[128];
    UINT8 applicationID[128];
    UINT8 copyrightFile[37];
    UINT8 abstractFile[37];
    UINT8 bibliographicFile[37];
    UINT8 creationDateTime[17];
    UINT8 modificationDateTime[17];
    UINT8 expirationDateTime[17];
    UINT8 effectiveDateTime[17];
    UINT8 fileStructureVersion;
    UINT8 unused4;
    UINT8 applicationUsed[512];
    UINT8 reserved[653];
} ISO_Primary_Volume_Descriptor_t;

typedef struct ISO_PathTableEntry {
    UINT8 nameLength;
    UINT8 extAttribRecordLength;
    UINT32 extentLBA;
    UINT16 parentDirNumber;
    UINT8 *dirName;    // We'll include the padding in the name
} ISO_PathTableEntry_t;

static EFI_STATUS ParsePathTable(UINT8 *pathTableData, UINTN pathTableSize, UINTN *entryCount, ISO_PathTableEntry_t **pathTableEntries) {
    EFI_STATUS status;
    
    UINTN numEntries = pathTableSize / 10; // The minimum length of each entry is 10 bytes
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, numEntries * sizeof(ISO_PathTableEntry_t), (void**)&(*pathTableEntries));
    if (EFI_ERROR(status) || *pathTableEntries == NULL) {
        Print(L"Failed to allocate memory for the Path Table Structure: %r\n\r", status);
        return status;
    }

    for (UINTN i = 0, entryPos = 0; i < numEntries; i++) {
        if (pathTableData[entryPos] == 0) {
            *entryCount = i;
            return EFI_SUCCESS;
        }

        (*pathTableEntries)[i].nameLength = pathTableData[entryPos];
        (*pathTableEntries)[i].extAttribRecordLength = pathTableData[entryPos + 1];
        // We use the Little Endian Path Table!
        (*pathTableEntries)[i].extentLBA = 
            (pathTableData[entryPos + 5] << 24) |
            (pathTableData[entryPos + 4] << 16) |
            (pathTableData[entryPos + 3] << 8) |
            pathTableData[entryPos + 2];
        (*pathTableEntries)[i].parentDirNumber = 
            (pathTableData[entryPos + 7] << 8) |
            pathTableData[entryPos + 6];

        // Directory Name is variable-length, so let's copy it properly
        status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, ((*pathTableEntries)[i].nameLength + 1) * sizeof(UINT8), (void**)&((*pathTableEntries)[i].dirName));
        if (EFI_ERROR(status) || (*pathTableEntries)[i].dirName == NULL) {
            Print(L"Failed to allocate memory for the Path Table Entry name: %r\n\r", status);
            // Failed to allocate memory for directory name, clean up and return
            for (UINTN j = 0; j < i; j++) {
                uefi_call_wrapper(BS->FreePool, 1, (*pathTableEntries)[j].dirName);
            }
            uefi_call_wrapper(BS->FreePool, 1, *pathTableEntries);
            return status;
        }

        memcpy((*pathTableEntries)[i].dirName, &pathTableData[entryPos + 8], (*pathTableEntries)[i].nameLength);
        (*pathTableEntries)[i].dirName[(*pathTableEntries)[i].nameLength] = '\0'; // Null-terminate the string

        entryPos += 8 + (pathTableData[entryPos] % 2 ? pathTableData[entryPos] + 1 : pathTableData[entryPos]);
    }

    *entryCount = numEntries;
    return EFI_SUCCESS;
}

#endif