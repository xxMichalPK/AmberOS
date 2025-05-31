#ifndef __ISO9660_H__
#define __ISO9660_H__

#include <stdint.h>
#include <stddef.h>
#include <boot/legacy/loader/memory.h>
#include <boot/legacy/loader/disk.h>
#include <ambererr.hpp>

#define ISO_PRIMARY_VOLUME_DESCRIPTOR_LBA 16
#define ISO_SECTOR_SIZE 2048
#define ISO_BYTES_TO_SECTORS(x) (bootDiskType == DISK_TYPE_HD ? (x % 512 == 0 ? x / 512 : x / 512 + 1) : (x % ISO_SECTOR_SIZE == 0 ? x / ISO_SECTOR_SIZE : x / ISO_SECTOR_SIZE + 1))
#define ISO_LBA(x) (bootDiskType == DISK_TYPE_HD ? x * 4 : x)

// The memory allocator works above 1MB but with the functions we are limited to the memory below 1MB
#define ISO_DIRECTORY_MAX_SIZE 4096     // This is the maximum size of the directory entry on stack
#define ISO_MAX_DATA_SIZE 4096

typedef struct ISO_Volume_Descriptor_Header {
    uint8_t type;
    uint8_t identifier[5];
    uint8_t version;
} __attribute__((packed)) ISO_Volume_Descriptor_Header_t;

typedef struct ISO_Primary_Volume_Descriptor {
    ISO_Volume_Descriptor_Header_t header;
    uint8_t unused1;
    uint8_t systemID[32];
    uint8_t volumeID[32];
    uint8_t unused2[8];
    uint32_t volumeSpaceSize[2];
    uint8_t unused3[32];
    uint16_t volumeSetSize[2];
    uint16_t volumeSequenceNumber[2];
    uint16_t logicalBlockSize[2];
    uint32_t pathTableSize[2];
    uint32_t L_PathTableLBA;
    uint32_t L_PathTableLBA_optional;
    uint32_t M_PathTableLBA;
    uint32_t M_PathTableLBA_optional;
    uint8_t rootDirEntry[34];
    uint8_t volumeSetID[128];
    uint8_t publisherID[128];
    uint8_t dataPreparerID[128];
    uint8_t applicationID[128];
    uint8_t copyrightFile[37];
    uint8_t abstractFile[37];
    uint8_t bibliographicFile[37];
    uint8_t creationDateTime[17];
    uint8_t modificationDateTime[17];
    uint8_t expirationDateTime[17];
    uint8_t effectiveDateTime[17];
    uint8_t fileStructureVersion;
    uint8_t unused4;
    uint8_t applicationUsed[512];
    uint8_t reserved[653];
} __attribute__((packed)) ISO_Primary_Volume_Descriptor_t;

typedef struct ISO_PathTableEntry {
    uint8_t nameLength;
    uint8_t extAttribRecordLength;
    uint32_t extentLBA;
    uint16_t parentDirNumber;
    uint8_t *dirName;
} __attribute__((packed)) ISO_PathTableEntry_t;

typedef struct ISO_DirectoryEntry {
    uint8_t recordLength;
    uint8_t extAttribRecordLength;
    uint32_t extentLBA;
    uint32_t fileSize;
    uint8_t recordingDateTime[7];
    uint8_t fileFlags;
    uint8_t fileUnitSize;
    uint8_t interleaveGapSize;
    uint16_t volSequenceNumber;
    uint8_t nameLength;
    uint8_t *fileName;
    uint8_t *systemUse;
} __attribute__((packed)) ISO_DirectoryEntry_t;

static ISO_Primary_Volume_Descriptor_t *pvd = NULL;
static AMBER_STATUS ISO_Initialize(uint8_t diskNum) {
    pvd = (ISO_Primary_Volume_Descriptor_t *)lmalloc(ISO_SECTOR_SIZE);
    if (!pvd) return AMBER_OUT_OF_MEMORY;

    // Use a temporary data buffer to avoid -Waddress-of-packed-member warning
    uint8_t dataBuffer[ISO_SECTOR_SIZE];

    if (ReadSectors(diskNum, ISO_LBA(ISO_PRIMARY_VOLUME_DESCRIPTOR_LBA), ISO_BYTES_TO_SECTORS(ISO_SECTOR_SIZE), (uint32_t*)dataBuffer) != 0) {
        lfree(pvd);
        pvd = NULL;
        return AMBER_READ_ERROR;
    }

    memcpy(pvd, dataBuffer, ISO_SECTOR_SIZE);
    return AMBER_SUCCESS;
}

static AMBER_STATUS ISO_ParseDirectory(uint8_t diskNum, const uint32_t dirLBA, const uint32_t dirSize, uint32_t *entryCount, ISO_DirectoryEntry_t **directory) {
    if (!dirSize) return AMBER_INVALID_ARGUMENT;

    uint8_t *rawDirectoryData = (uint8_t *)lmalloc(ISO_BYTES_TO_SECTORS(dirSize) * DISK_SECTOR_SZIE * sizeof(uint8_t));
    if (!rawDirectoryData) return AMBER_OUT_OF_MEMORY;
    // Read the raw data into the array
    if (ReadSectors(diskNum, ISO_LBA(dirLBA), ISO_BYTES_TO_SECTORS(dirSize), (uint32_t*)rawDirectoryData) != 0) {
        lfree(rawDirectoryData);
        return AMBER_READ_ERROR;
    }

    // Get the number of all entries in directory
    uint32_t numEntries = 0;
    for (uint32_t dataPos = 0; dataPos < dirSize;) {
        if (rawDirectoryData[dataPos] == 0) break;
        dataPos += rawDirectoryData[dataPos];
        numEntries++;
    }

    *entryCount = numEntries;

    // Allocate memory for the directory structure
    *directory = (ISO_DirectoryEntry_t*)lmalloc(numEntries * sizeof(ISO_DirectoryEntry_t));
    if (!directory) {
        lfree(rawDirectoryData);
        return AMBER_OUT_OF_MEMORY;
    }

    // Copy the raw data to the structure
    for (uint32_t i = 0, entryPos = 0; i < numEntries; i++) {
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
        uint32_t fullNameLength = (*directory)[i].nameLength + (rawDirectoryData[entryPos + 33] == 1 ? 1 : 0);
        (*directory)[i].fileName = (uint8_t*)lmalloc((fullNameLength + 1) * sizeof(uint8_t));
        if ((*directory)[i].fileName == NULL) {
            for (uint32_t j = 0; j < i; j++) {
                lfree((*directory)[j].fileName);
            }
            lfree(*directory);
            lfree(rawDirectoryData);
            return AMBER_OUT_OF_MEMORY;
        }

        if (rawDirectoryData[entryPos + 33] != 0 && rawDirectoryData[entryPos + 33] != 1)
            memcpy((*directory)[i].fileName, &rawDirectoryData[entryPos + 33], fullNameLength);

        if (rawDirectoryData[entryPos + 33] == 0) (*directory)[i].fileName[0] = '.';
        if (rawDirectoryData[entryPos + 33] == 1) { (*directory)[i].fileName[0] = '.'; (*directory)[i].fileName[1] = '.'; }
        (*directory)[i].fileName[fullNameLength] = '\0'; // Null-terminate the string

        entryPos += (*directory)[i].recordLength;
    }

    lfree(rawDirectoryData);
    return AMBER_SUCCESS;
}

static AMBER_STATUS ISO_ReadFile(uint8_t diskNum, char *path, uint32_t *fileSize, void** dataBuffer) {
    // We have to specify the absolute file path starting with '/'
    if (path[0] != '/') return AMBER_INVALID_ARGUMENT;

    AMBER_STATUS status;

    uint32_t dirCount = 0;
    uint32_t dirPos = 0;
    while (path[dirPos] != '\0') {
        if (path[dirPos] == '/') dirCount++;
        dirPos++;
    }

    uint32_t *dirNameLenghts = NULL;
    dirNameLenghts = (uint32_t*)lmalloc((dirCount + 1) * sizeof(uint32_t));
    if (!dirNameLenghts) return AMBER_OUT_OF_MEMORY;

    for (uint32_t i = 0; i < dirCount + 1; i++) {
        dirNameLenghts[i] = 0;
    }

    uint32_t dirIdx = 0;
    dirPos = 0;
    while (path[dirPos] != '\0') {
        if (path[dirPos] == '/') { dirIdx++; dirPos++; continue; }
        dirNameLenghts[dirIdx] += 1;
        dirPos++;
    }

    ISO_DirectoryEntry_t **directories = { NULL };
    directories = (ISO_DirectoryEntry_t**)lmalloc(dirCount * sizeof(ISO_DirectoryEntry_t*));
    if (!directories) {
        lfree(dirNameLenghts);
        return AMBER_OUT_OF_MEMORY;
    }

    uint32_t *numFilesInDirectory = NULL;
    numFilesInDirectory = (uint32_t*)lmalloc(dirCount * sizeof(uint32_t));
    if (!numFilesInDirectory) {
        lfree(directories);
        lfree(dirNameLenghts);
        return AMBER_OUT_OF_MEMORY;
    }

    // First directory is always the root directory
    status = ISO_ParseDirectory(diskNum, pvd->rootDirEntry[2] | pvd->rootDirEntry[3] << 8 | pvd->rootDirEntry[4] << 16 | pvd->rootDirEntry[5] << 24,
                               pvd->rootDirEntry[10] | pvd->rootDirEntry[11] << 8 | pvd->rootDirEntry[12] << 16 | pvd->rootDirEntry[13] << 24,
                               &numFilesInDirectory[0], &directories[0]);
    if (AMBER_ERROR(status)) {
        lfree(numFilesInDirectory);
        lfree(directories);
        lfree(dirNameLenghts);
        return status;
    }

    path++; // Skip the first slash '/'
    for (uint32_t d = 0; d < dirCount - 1; d++) {
        uint8_t exists = 0;

        for (uint32_t fIdx = 0; fIdx < numFilesInDirectory[d]; fIdx++) {
            if (memcmp(directories[d][fIdx].fileName, path, dirNameLenghts[d + 1]) == 0) {
                status = ISO_ParseDirectory(diskNum, directories[d][fIdx].extentLBA, directories[d][fIdx].fileSize, &numFilesInDirectory[d + 1], &directories[d + 1]);
                if(AMBER_ERROR(status)) {
                    for (uint32_t fd = 0; fd <= d; fd++) {
                        for (uint32_t ff = 0; ff < numFilesInDirectory[fd]; ff++) {
                            lfree(directories[fd][ff].fileName);
                        }
                        lfree(directories[fd]);
                    }
                    lfree(directories);
                    lfree(dirNameLenghts);
                    lfree(numFilesInDirectory);
                    return status;
                }

                path += dirNameLenghts[d + 1] + 1;
                exists = 1;
            }
        }

        if (!exists) {
            *fileSize = 0;
            for (uint32_t fd = 0; fd <= d; fd++) {
                for (uint32_t ff = 0; ff < numFilesInDirectory[fd]; ff++) {
                    lfree(directories[fd][ff].fileName);
                }
                lfree(directories[fd]);
            }
            lfree(directories);
            lfree(dirNameLenghts);
            lfree(numFilesInDirectory);
            return AMBER_FILE_NOT_FOUND;
        }
    }

    uint8_t tempDataBuffer[ISO_SECTOR_SIZE];
    for (uint32_t fIdx = 0; fIdx < numFilesInDirectory[dirCount - 1]; fIdx++) {
        if (memcmp(directories[dirCount - 1][fIdx].fileName, path, dirNameLenghts[dirCount]) == 0 && directories[dirCount - 1][fIdx].nameLength == dirNameLenghts[dirCount]) {
            uint32_t dataSectorSize = (directories[dirCount - 1][fIdx].fileSize / ISO_SECTOR_SIZE);
            if (directories[dirCount - 1][fIdx].fileSize % ISO_SECTOR_SIZE != 0) dataSectorSize++;

            if (!(*dataBuffer)) {
                (*dataBuffer) = lmalloc(dataSectorSize * ISO_SECTOR_SIZE);
            }
            
            if (!(*dataBuffer)) {
                for (uint32_t fd = 0; fd < dirCount; fd++) {
                    for (uint32_t ff = 0; ff < numFilesInDirectory[fd]; ff++) {
                        lfree(directories[fd][ff].fileName);
                    }
                    lfree(directories[fd]);
                }
                lfree(directories);
                lfree(dirNameLenghts);
                lfree(numFilesInDirectory);
                return AMBER_OUT_OF_MEMORY;
            }

            for (uint32_t currentLBA = directories[dirCount - 1][fIdx].extentLBA, off = 0; currentLBA < directories[dirCount - 1][fIdx].extentLBA + dataSectorSize; currentLBA++, off += ISO_SECTOR_SIZE) {
                if (ReadSectors(diskNum, ISO_LBA(currentLBA), ISO_BYTES_TO_SECTORS(ISO_SECTOR_SIZE), (uint32_t*)tempDataBuffer) != 0) {
                    for (uint32_t fd = 0; fd < dirCount; fd++) {
                        for (uint32_t ff = 0; ff < numFilesInDirectory[fd]; ff++) {
                            lfree(directories[fd][ff].fileName);
                        }
                        lfree(directories[fd]);
                    }
                    lfree(directories);
                    lfree(dirNameLenghts);
                    lfree(numFilesInDirectory);
                    return AMBER_READ_ERROR;
                }
                memcpy((void*)(((uintptr_t)(*dataBuffer)) + off), (void*)tempDataBuffer, ISO_SECTOR_SIZE);
            }

            *fileSize = directories[dirCount - 1][fIdx].fileSize;
            for (uint32_t fd = 0; fd < dirCount; fd++) {
                for (uint32_t ff = 0; ff < numFilesInDirectory[fd]; ff++) {
                    lfree(directories[fd][ff].fileName);
                }
                lfree(directories[fd]);
            }
            lfree(directories);
            lfree(dirNameLenghts);
            lfree(numFilesInDirectory);
            return AMBER_SUCCESS;
        }
    }

    *fileSize = 0;
    for (uint32_t fd = 0; fd < dirCount; fd++) {
        for (uint32_t ff = 0; ff < numFilesInDirectory[fd]; ff++) {
            lfree(directories[fd][ff].fileName);
        }
        lfree(directories[fd]);
    }
    lfree(directories);
    lfree(dirNameLenghts);
    lfree(numFilesInDirectory);
    return AMBER_FILE_NOT_FOUND;
}

static size_t ISO_GetFileSize(uint8_t diskNum, char *path) {
    // We have to specify the absolute file path starting with '/'
    if (path[0] != '/') return 0;

    uint32_t dirCount = 0;
    uint32_t dirPos = 0;
    while (path[dirPos] != '\0') {
        if (path[dirPos] == '/') dirCount++;
        dirPos++;
    }

    uint32_t *dirNameLenghts = NULL;
    dirNameLenghts = (uint32_t*)lmalloc((dirCount + 1) * sizeof(uint32_t));
    if (!dirNameLenghts) return 0;

    for (uint32_t i = 0; i < dirCount + 1; i++) {
        dirNameLenghts[i] = 0;
    }

    uint32_t dirIdx = 0;
    dirPos = 0;
    while (path[dirPos] != '\0') {
        if (path[dirPos] == '/') { dirIdx++; dirPos++; continue; }
        dirNameLenghts[dirIdx] += 1;
        dirPos++;
    }

    ISO_DirectoryEntry_t **directories = { NULL };
    directories = (ISO_DirectoryEntry_t**)lmalloc(dirCount * sizeof(ISO_DirectoryEntry_t*));
    if (!directories) {
        lfree(dirNameLenghts);
        return 0;
    }

    uint32_t *numFilesInDirectory = NULL;
    numFilesInDirectory = (uint32_t*)lmalloc(dirCount * sizeof(uint32_t));
    if (!numFilesInDirectory) {
        lfree(directories);
        lfree(dirNameLenghts);
        return 0;
    }

    // First directory is always the root directory
    if (ISO_ParseDirectory(diskNum, pvd->rootDirEntry[2] | pvd->rootDirEntry[3] << 8 | pvd->rootDirEntry[4] << 16 | pvd->rootDirEntry[5] << 24,
                               pvd->rootDirEntry[10] | pvd->rootDirEntry[11] << 8 | pvd->rootDirEntry[12] << 16 | pvd->rootDirEntry[13] << 24,
                               &numFilesInDirectory[0], &directories[0]) != 0
    ) {
        lfree(numFilesInDirectory);
        lfree(directories);
        lfree(dirNameLenghts);
        return 0;
    }

    path++; // Skip the first slash '/'
    for (uint32_t d = 0; d < dirCount - 1; d++) {
        uint8_t exists = 0;

        for (uint32_t fIdx = 0; fIdx < numFilesInDirectory[d]; fIdx++) {
            if (memcmp(directories[d][fIdx].fileName, path, dirNameLenghts[d + 1]) == 0) {
                if (ISO_ParseDirectory(diskNum, directories[d][fIdx].extentLBA, directories[d][fIdx].fileSize, &numFilesInDirectory[d + 1], &directories[d + 1]) != 0) {
                    for (uint32_t fd = 0; fd <= d; fd++) {
                        for (uint32_t ff = 0; ff < numFilesInDirectory[fd]; ff++) {
                            lfree(directories[fd][ff].fileName);
                        }
                        lfree(directories[fd]);
                    }
                    lfree(directories);
                    lfree(dirNameLenghts);
                    lfree(numFilesInDirectory);
                    return 0;
                }

                path += dirNameLenghts[d + 1] + 1;
                exists = 1;
            }
        }

        if (!exists) {
            for (uint32_t fd = 0; fd <= d; fd++) {
                for (uint32_t ff = 0; ff < numFilesInDirectory[fd]; ff++) {
                    lfree(directories[fd][ff].fileName);
                }
                lfree(directories[fd]);
            }
            lfree(directories);
            lfree(dirNameLenghts);
            lfree(numFilesInDirectory);
            return 0;
        }
    }

    for (uint32_t fIdx = 0; fIdx < numFilesInDirectory[dirCount - 1]; fIdx++) {
        if (memcmp(directories[dirCount - 1][fIdx].fileName, path, dirNameLenghts[dirCount]) == 0 && directories[dirCount - 1][fIdx].nameLength == dirNameLenghts[dirCount]) {
            size_t fileSize = directories[dirCount - 1][fIdx].fileSize;
            for (uint32_t fd = 0; fd < dirCount; fd++) {
                for (uint32_t ff = 0; ff < numFilesInDirectory[fd]; ff++) {
                    lfree(directories[fd][ff].fileName);
                }
                lfree(directories[fd]);
            }
            lfree(directories);
            lfree(dirNameLenghts);
            lfree(numFilesInDirectory);
            return fileSize;
        }
    }

    return 0;
}

#endif