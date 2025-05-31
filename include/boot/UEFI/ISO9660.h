#ifndef __ISO9660_H__
#define __ISO9660_H__

#include <efi/efi.h>
#include <efi/efilib.h>
#include <boot/UEFI/memory.h>

#define ISO_PRIMARY_VOLUME_DESCRIPTOR_LBA 16
#define ISO_SECTOR_SIZE 2048
#define ISO_LBA(x) (x * (ISO_SECTOR_SIZE / 512))

typedef struct ISO_Volume_Descriptor_Header {
    UINT8 type;
    UINT8 identifier[5];
    UINT8 version;
} __attribute__((packed)) ISO_Volume_Descriptor_Header_t;

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
} __attribute__((packed)) ISO_Primary_Volume_Descriptor_t;

typedef struct ISO_PathTableEntry {
    UINT8 nameLength;
    UINT8 extAttribRecordLength;
    UINT32 extentLBA;
    UINT16 parentDirNumber;
    UINT8 *dirName;
} __attribute__((packed)) ISO_PathTableEntry_t;

typedef struct ISO_DirectoryEntry {
    UINT8 recordLength;
    UINT8 extAttribRecordLength;
    UINT32 extentLBA;
    UINT32 fileSize;
    UINT8 recordingDateTime[7];
    UINT8 fileFlags;
    UINT8 fileUnitSize;
    UINT8 interleaveGapSize;
    UINT16 volSequenceNumber;
    UINT8 nameLength;
    UINT8 *fileName;
    UINT8 *systemUse;
} __attribute__((packed)) ISO_DirectoryEntry_t;

static ISO_Primary_Volume_Descriptor_t *pvd = NULL;

static EFI_STATUS InitializeISO_FS(EFI_BLOCK_IO_PROTOCOL *diskIO) {
    EFI_STATUS status;

    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, ISO_SECTOR_SIZE, (void**)&pvd);
    if (EFI_ERROR(status)) return status;

    status = uefi_call_wrapper(diskIO->ReadBlocks, 5, diskIO, diskIO->Media->MediaId, ISO_LBA(ISO_PRIMARY_VOLUME_DESCRIPTOR_LBA), ISO_SECTOR_SIZE, &(*pvd));
    if (EFI_ERROR(status)) return status;

    return EFI_SUCCESS;
}

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

static EFI_STATUS ParseDirectory(EFI_BLOCK_IO_PROTOCOL *diskIO, const UINT32 dirLBA, const UINT32 dirSize, UINTN *entryCount, ISO_DirectoryEntry_t **directory) {
    EFI_STATUS status;

    UINT8 *rawDirectoryData = NULL;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, (((dirSize / 512) + 1) * 512) * sizeof(UINT8), (void**)&rawDirectoryData);
    if (EFI_ERROR(status) || rawDirectoryData == NULL) {
        Print(L"Failed to allocate memory for the Raw Directory Data: %r\n\r", status);
        return status;
    }

    status = uefi_call_wrapper(diskIO->ReadBlocks, 5, diskIO, diskIO->Media->MediaId, ISO_LBA(dirLBA), ((dirSize / 512) + 1) * 512, &(*rawDirectoryData));
    if (EFI_ERROR(status)) {
        Print(L"Failed to read the directory: %r\n\r", status);
        return status;
    }

    // Get the number of all entries in directory
    UINTN numEntries = 0;
    for (UINTN dataPos = 0; dataPos < dirSize;) {
        if (rawDirectoryData[dataPos] == 0) break;
        dataPos += rawDirectoryData[dataPos];
        numEntries++;
    }

    *entryCount = numEntries;

    // Allocate memory for the directory structure
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, numEntries * sizeof(ISO_DirectoryEntry_t), (void**)&(*directory));
    if (EFI_ERROR(status) || *directory == NULL) {
        Print(L"Failed to allocate memory for the Directory Structure: %r\n\r", status);
        return status;
    }

    // Copy the raw data to the structure
    for (UINTN i = 0, entryPos = 0; i < numEntries; i++) {
        (*directory)[i].recordLength = rawDirectoryData[entryPos];
        (*directory)[i].extAttribRecordLength = rawDirectoryData[entryPos + 1];
        (*directory)[i].extentLBA = (rawDirectoryData[entryPos + 5] << 24) | (rawDirectoryData[entryPos + 4] << 16) | (rawDirectoryData[entryPos + 3] << 8) | rawDirectoryData[entryPos + 2];
        (*directory)[i].fileSize = (rawDirectoryData[entryPos + 13] << 24) | (rawDirectoryData[entryPos + 12] << 16) | (rawDirectoryData[entryPos + 11] << 8) | rawDirectoryData[entryPos + 10];
        (*directory)[i].recordingDateTime[0] = rawDirectoryData[entryPos + 18];
        (*directory)[i].recordingDateTime[1] = rawDirectoryData[entryPos + 19];
        (*directory)[i].recordingDateTime[2] = rawDirectoryData[entryPos + 20];
        (*directory)[i].recordingDateTime[3] = rawDirectoryData[entryPos + 21];
        (*directory)[i].recordingDateTime[4] = rawDirectoryData[entryPos + 22];
        (*directory)[i].recordingDateTime[5] = rawDirectoryData[entryPos + 23];
        (*directory)[i].recordingDateTime[6] = rawDirectoryData[entryPos + 24];
        (*directory)[i].fileFlags = rawDirectoryData[entryPos + 25];
        (*directory)[i].fileUnitSize = rawDirectoryData[entryPos + 26];
        (*directory)[i].interleaveGapSize = rawDirectoryData[entryPos + 27];
        (*directory)[i].volSequenceNumber = rawDirectoryData[entryPos + 28] | (rawDirectoryData[entryPos + 29] << 8);
        (*directory)[i].nameLength = rawDirectoryData[entryPos + 32];
        (*directory)[i].systemUse = NULL;

        // File Name is variable-length, so let's copy it properly
        UINTN fullNameLength = (*directory)[i].nameLength + (rawDirectoryData[entryPos + 33] == 1 ? 1 : 0);
        status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, (fullNameLength + 1) * sizeof(UINT8), (void**)&((*directory)[i].fileName));
        if (EFI_ERROR(status) || (*directory)[i].fileName == NULL) {
            Print(L"Failed to allocate memory for the File name: %r\n\r", status);
            // Failed to allocate memory for directory name, clean up and return
            for (UINTN j = 0; j < i; j++) {
                uefi_call_wrapper(BS->FreePool, 1, (*directory)[j].fileName);
            }
            uefi_call_wrapper(BS->FreePool, 1, *directory);
            uefi_call_wrapper(BS->FreePool, 1, rawDirectoryData);
            return status;
        }

        if (rawDirectoryData[entryPos + 33] != 0 && rawDirectoryData[entryPos + 33] != 1)
            memcpy((*directory)[i].fileName, &rawDirectoryData[entryPos + 33], fullNameLength);

        if (rawDirectoryData[entryPos + 33] == 0) (*directory)[i].fileName[0] = '.';
        if (rawDirectoryData[entryPos + 33] == 1) { (*directory)[i].fileName[0] = '.'; (*directory)[i].fileName[1] = '.'; }
        (*directory)[i].fileName[fullNameLength] = '\0'; // Null-terminate the string

        entryPos += (*directory)[i].recordLength;
    }

    uefi_call_wrapper(BS->FreePool, 1, rawDirectoryData);
    return EFI_SUCCESS;
}

static EFI_STATUS ReadFile(EFI_BLOCK_IO_PROTOCOL *diskIO, char *path, UINTN *fileSize, void** dataBuffer) {
    EFI_STATUS status;

    // We have to specify the absolute file path starting with '/'
    if (path[0] != '/') {
        return EFI_NOT_FOUND;
    }

    UINTN dirCount = 0;
    UINTN dirPos = 0;
    while (path[dirPos] != '\0') {
        if (path[dirPos] == '/') dirCount++;
        dirPos++;
    }

    UINTN *dirNameLenghts = NULL;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, (dirCount + 1) * sizeof(UINTN), (void**)&dirNameLenghts);
    if (EFI_ERROR(status) || dirNameLenghts == NULL) {
        return status;
    }

    for (UINTN i = 0; i < dirCount + 1; i++) {
        dirNameLenghts[i] = 0;
    }

    UINTN dirIdx = 0;
    dirPos = 0;
    while (path[dirPos] != '\0') {
        if (path[dirPos] == '/') { dirIdx++; dirPos++; continue; }
        dirNameLenghts[dirIdx] += 1;
        dirPos++;
    }

    ISO_DirectoryEntry_t **directories = { NULL };
    UINTN *numFilesInDirectory = NULL;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, dirCount * sizeof(ISO_DirectoryEntry_t*), (void**)&directories);
    if (EFI_ERROR(status)) {
        Print(L"Failed to allocate memory for the directories: %r\n\r", status);
        uefi_call_wrapper(BS->FreePool, 1, dirNameLenghts);
        return status;
    }

    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, dirCount * sizeof(UINTN), (void**)&numFilesInDirectory);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(BS->FreePool, 1, dirNameLenghts);
        uefi_call_wrapper(BS->FreePool, 1, *directories);
        return status;
    }

    // First directory is always the root directory
    status = ParseDirectory(diskIO, pvd->rootDirEntry[2] | pvd->rootDirEntry[3] << 8 | pvd->rootDirEntry[4] << 16 | pvd->rootDirEntry[5] << 24,
                                          pvd->rootDirEntry[10] | pvd->rootDirEntry[11] << 8 | pvd->rootDirEntry[12] << 16 | pvd->rootDirEntry[13] << 24, &numFilesInDirectory[0], &directories[0]);
    if (EFI_ERROR(status)) {
        Print(L"Failed to parse root directory: %r\n\r", status);
        uefi_call_wrapper(BS->FreePool, 1, dirNameLenghts);
        uefi_call_wrapper(BS->FreePool, 1, numFilesInDirectory);
        uefi_call_wrapper(BS->FreePool, 1, *directories);
        return status;
    }

    path++; // Skip the first slash '/'
    for (UINTN d = 0; d < dirCount - 1; d++) {
        BOOLEAN exists = FALSE;

        for (UINTN fIdx = 0; fIdx < numFilesInDirectory[d]; fIdx++) {
            if (memcmp(directories[d][fIdx].fileName, path, dirNameLenghts[d + 1]) == 0) {
                status = ParseDirectory(diskIO, directories[d][fIdx].extentLBA, directories[d][fIdx].fileSize, &numFilesInDirectory[d + 1], &directories[d + 1]);
                if (EFI_ERROR(status)) {
                    Print(L"Failed to parse directory: %r\n\r", status);
                    uefi_call_wrapper(BS->FreePool, 1, dirNameLenghts);
                    uefi_call_wrapper(BS->FreePool, 1, numFilesInDirectory);
                    uefi_call_wrapper(BS->FreePool, 1, *directories);
                    return status;
                }

                path += dirNameLenghts[d + 1] + 1;
                exists = TRUE;
            }
        }

        if (!exists) {
            uefi_call_wrapper(BS->FreePool, 1, dirNameLenghts);
            uefi_call_wrapper(BS->FreePool, 1, numFilesInDirectory);
            uefi_call_wrapper(BS->FreePool, 1, *directories);
            return EFI_NOT_FOUND;
        }
    }

    for (UINTN fIdx = 0; fIdx < numFilesInDirectory[dirCount - 1]; fIdx++) {
        if (memcmp(directories[dirCount - 1][fIdx].fileName, path, dirNameLenghts[dirCount]) == 0 &&
            directories[dirCount - 1][fIdx].nameLength == dirNameLenghts[dirCount]) {
            status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, ((directories[dirCount - 1][fIdx].fileSize / 512) + 1) * 512, dataBuffer);
            if (EFI_ERROR(status)) {
                uefi_call_wrapper(BS->FreePool, 1, dirNameLenghts);
                uefi_call_wrapper(BS->FreePool, 1, numFilesInDirectory);
                uefi_call_wrapper(BS->FreePool, 1, *directories);
                return status;
            }

            status = uefi_call_wrapper(diskIO->ReadBlocks, 5, diskIO, diskIO->Media->MediaId, ISO_LBA(directories[dirCount - 1][fIdx].extentLBA), ((directories[dirCount - 1][fIdx].fileSize / 512) + 1) * 512, *dataBuffer);
            if (EFI_ERROR(status)) {
                Print(L"Failed to read the file: %r\n\r", status);
                uefi_call_wrapper(BS->FreePool, 1, dirNameLenghts);
                uefi_call_wrapper(BS->FreePool, 1, numFilesInDirectory);
                uefi_call_wrapper(BS->FreePool, 1, *directories);
                return status;
            }

            *fileSize = directories[dirCount - 1][fIdx].fileSize;

            uefi_call_wrapper(BS->FreePool, 1, dirNameLenghts);
            uefi_call_wrapper(BS->FreePool, 1, numFilesInDirectory);
            uefi_call_wrapper(BS->FreePool, 1, *directories);
            return EFI_SUCCESS;
        }
    }

    *fileSize = 0;
    uefi_call_wrapper(BS->FreePool, 1, dirNameLenghts);
    uefi_call_wrapper(BS->FreePool, 1, numFilesInDirectory);
    uefi_call_wrapper(BS->FreePool, 1, *directories);
    return EFI_NOT_FOUND;
}

#endif