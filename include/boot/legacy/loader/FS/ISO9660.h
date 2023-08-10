#ifndef __ISO9660_H__
#define __ISO9660_H__

#include <stdint.h>
#include <stddef.h>
#include <boot/legacy/loader/memory.h>
#include <boot/legacy/loader/disk.h>

#define ISO_PRIMARY_VOLUME_DESCRIPTOR_LBA 16
#define ISO_SECTOR_SIZE 2048
#define ISO_LBA(x) (x)      // In the legacy version the interrupt uses the actual sector size (2048B)

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
static int InitializeISO_FS(uint8_t diskNum) {
    pvd = (ISO_Primary_Volume_Descriptor_t *)lmalloc(ISO_SECTOR_SIZE);
    if (!pvd) return -1;

    // Since we execute the ReadSectors function in real mode, we can't read to address above 1MB
    // so create a temporary data buffer for storing the data
    uint8_t dataBuffer[ISO_SECTOR_SIZE];

    ISO_Primary_Volume_Descriptor_t *pvdPtr = pvd;
    if (ReadSectors(diskNum, ISO_PRIMARY_VOLUME_DESCRIPTOR_LBA, 1, (uint32_t*)dataBuffer) != 0) return -1;

    memcpy(pvd, dataBuffer, ISO_SECTOR_SIZE);
    return 0;
}

static int ParseISODirectory(uint8_t diskNum, const uint32_t dirLBA, const uint32_t dirSize, uint32_t *entryCount, ISO_DirectoryEntry_t **directory) {
    uint8_t rawDirectoryData[ISO_DIRECTORY_MAX_SIZE];

    uint32_t dirSectorSize = dirSize / ISO_SECTOR_SIZE;
    if (dirSize % ISO_SECTOR_SIZE != 0) dirSectorSize++;

    if (dirSectorSize * ISO_SECTOR_SIZE > ISO_DIRECTORY_MAX_SIZE) return -1;

    // Read the raw data into the array
    if (ReadSectors(diskNum, dirLBA, dirSectorSize, (uint32_t*)rawDirectoryData) != 0) return -1;

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
    if (!directory) return -1;

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
        if ((*directory)[i].fileName == NULL) return -1;

        if (rawDirectoryData[entryPos + 33] != 0 && rawDirectoryData[entryPos + 33] != 1)
            memcpy((*directory)[i].fileName, &rawDirectoryData[entryPos + 33], fullNameLength);

        if (rawDirectoryData[entryPos + 33] == 0) (*directory)[i].fileName[0] = '.';
        if (rawDirectoryData[entryPos + 33] == 1) { (*directory)[i].fileName[0] = '.'; (*directory)[i].fileName[1] = '.'; }
        (*directory)[i].fileName[fullNameLength] = '\0'; // Null-terminate the string

        entryPos += (*directory)[i].recordLength;
    }

    return 0;
}

static int ReadISO_FSFile(uint8_t diskNum, char *path, uint32_t *fileSize, void** dataBuffer) {
    // We have to specify the absolute file path starting with '/'
    if (path[0] != '/') return -1;

    uint32_t dirCount = 0;
    uint32_t dirPos = 0;
    while (path[dirPos] != '\0') {
        if (path[dirPos] == '/') dirCount++;
        dirPos++;
    }

    uint32_t *dirNameLenghts = NULL;
    dirNameLenghts = (uint32_t*)lmalloc((dirCount + 1) * sizeof(uint32_t));
    if (!dirNameLenghts) return -1;

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
    if (!directories) return -1;

    uint32_t *numFilesInDirectory = NULL;
    numFilesInDirectory = (uint32_t*)lmalloc(dirCount * sizeof(uint32_t));
    if (!numFilesInDirectory) return -1;

    // First directory is always the root directory
    if (ParseISODirectory(diskNum, pvd->rootDirEntry[2] | pvd->rootDirEntry[3] << 8 | pvd->rootDirEntry[4] << 16 | pvd->rootDirEntry[5] << 24,
                               pvd->rootDirEntry[10] | pvd->rootDirEntry[11] << 8 | pvd->rootDirEntry[12] << 16 | pvd->rootDirEntry[13] << 24,
                               &numFilesInDirectory[0], &directories[0]) != 0) return -1;

    path++; // Skip the first slash '/'
    for (uint32_t d = 0; d < dirCount - 1; d++) {
        uint8_t exists = 0;

        for (uint32_t fIdx = 0; fIdx < numFilesInDirectory[d]; fIdx++) {
            if (memcmp(directories[d][fIdx].fileName, path, dirNameLenghts[d + 1]) == 0) {
                if (ParseISODirectory(diskNum, directories[d][fIdx].extentLBA, directories[d][fIdx].fileSize, &numFilesInDirectory[d + 1], &directories[d + 1]) != 0) return -1;

                path += dirNameLenghts[d + 1] + 1;
                exists = 1;
            }
        }

        if (!exists) return -1;
    }

    uint8_t tempDataBuffer[ISO_SECTOR_SIZE];
    for (uint32_t fIdx = 0; fIdx < numFilesInDirectory[dirCount - 1]; fIdx++) {
        if (memcmp(directories[dirCount - 1][fIdx].fileName, path, dirNameLenghts[dirCount]) == 0 && directories[dirCount - 1][fIdx].nameLength == dirNameLenghts[dirCount]) {
            uint32_t dataSectorSize = (directories[dirCount - 1][fIdx].fileSize / ISO_SECTOR_SIZE);
            if (directories[dirCount - 1][fIdx].fileSize % ISO_SECTOR_SIZE != 0) dataSectorSize++;

            (*dataBuffer) = lmalloc(dataSectorSize * ISO_SECTOR_SIZE);
            if (!(*dataBuffer)) return -1;

            // Up to this point everything works fine. The second iteration crashes!
            for (uint32_t currentLBA = directories[dirCount - 1][fIdx].extentLBA, off = 0; currentLBA < directories[dirCount - 1][fIdx].extentLBA + dataSectorSize; currentLBA++, off += ISO_SECTOR_SIZE) {
                if (ReadSectors(diskNum, currentLBA, 1, (uint32_t*)tempDataBuffer) != 0) return -1;
                memcpy((void*)((*dataBuffer) + off), (void*)tempDataBuffer, ISO_SECTOR_SIZE);
            }

            *fileSize = directories[dirCount - 1][fIdx].fileSize;
            return 0;
        }
    }

    *fileSize = 0;
    return -1;
}

#endif