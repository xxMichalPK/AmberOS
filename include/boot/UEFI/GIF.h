#ifndef __GIF_H__
#define __GIF_H__

#include <efi/efi.h>
#include <efi/efilib.h>
#include <stdint.h>
#include <boot/UEFI/memory.h>
#include <boot/UEFI/timer.h>

#define GIF_APPLICATION_EXTENSION 0xFF
#define GIF_COMMENT_EXTENSION 0xFE
#define GIF_GRAPHIC_CONTROL_EXTENSION 0xF9
#define GIF_PLAIN_TEXT_EXTENSION 0x01

#define GIF_EXTENSION_BLOCK 0x21
#define GIF_IMAGE_DESCRIPTOR 0x2C
#define GIF_EOF 0x3B
#define GIF_TERMINATOR 0x00

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} __attribute__((packed)) GIF_ColorTableEntry_t;

typedef struct {
    uint8_t signature[6];
    uint16_t width;
    uint16_t height;
    uint8_t flags;
    uint8_t bgColor;
    uint8_t pixelAspectRatio;
    GIF_ColorTableEntry_t *globalColorTable;
} __attribute__((packed)) GIF_Header_t;

typedef struct {
    int32_t numberOfRepetitions;
} __attribute__((packed)) GIF_ApplicationExtension_t;

typedef struct {
    uint8_t flags;
    uint16_t frameDelay;
    uint8_t transparentColorIdx;
} __attribute__((packed)) GIF_GraphicControlExtension_t;

typedef struct {
    uint16_t frameX;
    uint16_t frameY;
    uint16_t width;
    uint16_t height;
    uint8_t flags;
    uint8_t codeSize;
} __attribute__((packed)) GIF_ImageDescriptorHeader_t;

#define MAX_LZW_TABLE_SIZE 4096
typedef struct {
    uint8_t byte;
    int previous;
    int length;
} __attribute__((packed)) LZW_TableEntry_t;

static EFI_STATUS HandleApplicationExtension(uint8_t **gifData, GIF_ApplicationExtension_t *extData) {
    extData->numberOfRepetitions = -1;
    // For now we support only NETSCAPE
    *gifData += 1;
    if (memcmp(*gifData + 1, "NETSCAPE2.0", **gifData) != 0) return EFI_UNSUPPORTED;
    *gifData += (**gifData) + 3;
    extData->numberOfRepetitions = (**gifData) | ((*((*gifData) + 1)) << 8);
    *gifData += 2;
    return EFI_SUCCESS;
}

static EFI_STATUS HandleCommentExtension(uint8_t **gifData) {
    // Skip the comments
    *gifData += 1;
    *gifData += (**gifData) + 1;
    return EFI_SUCCESS;
}

static EFI_STATUS HandleGraphicControlExtension(uint8_t **gifData, GIF_GraphicControlExtension_t *extData) {
    *gifData += 2;
    extData->flags = **gifData;
    *gifData += 1;
    extData->frameDelay = (**gifData) | ((*((*gifData) + 1)) << 8);
    *gifData += 2;
    extData->transparentColorIdx = **gifData;
    *gifData += 1;
    return EFI_SUCCESS;
}

static EFI_STATUS HandlePlainTextExtension(uint8_t **gifData) {
    return EFI_UNSUPPORTED;
}

static EFI_STATUS ReadImageDescriptorHeader(uint8_t **gifData, GIF_ImageDescriptorHeader_t *outHeader) {
    *gifData += 1;
    outHeader->frameX = (**gifData) | ((*((*gifData) + 1)) << 8);
    *gifData += 2;
    outHeader->frameY = (**gifData) | ((*((*gifData) + 1)) << 8);
    *gifData += 2;
    outHeader->width = (**gifData) | ((*((*gifData) + 1)) << 8);
    *gifData += 2;
    outHeader->height = (**gifData) | ((*((*gifData) + 1)) << 8);
    *gifData += 2;
    outHeader->flags = **gifData;
    *gifData += 1;
    outHeader->codeSize = **gifData;
    *gifData += 1;
    return EFI_SUCCESS;
}

static void InitializeLZWTable(LZW_TableEntry_t *table, uint8_t codeSize) {
    for (uint16_t i = 0; i < (1 << codeSize); i++) {
        table[i].byte = i;
        table[i].previous = -1;
        table[i].length = 1;
    }
}

static EFI_STATUS DecodeImageDescriptorData(uint8_t **gifData, uint8_t codeSize, uint8_t **uncompressedData) {
    EFI_STATUS status;

    uint8_t *output = *uncompressedData;
    uint8_t *tmpData = *gifData;
    uint32_t dataSize;
    uint8_t currentDataSize = *tmpData;
    while (*tmpData != GIF_TERMINATOR) {
        dataSize += *tmpData;
        tmpData += (*tmpData) + 1;
    }

    uint8_t *encodedData = NULL;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, dataSize * sizeof(uint8_t), (void**)&encodedData);
    if (EFI_ERROR(status)) return status;

    // Copy the image encoded data into one buffer
    *gifData += 1;  // Go to the start of compressed data
    uint8_t *encodedDataPtr = encodedData;
    while (currentDataSize) {
        memcpy(encodedDataPtr, *gifData, currentDataSize);
        *gifData += currentDataSize;
        encodedDataPtr += currentDataSize;
        currentDataSize = **gifData;
        *gifData += 1;
    }
    encodedDataPtr = encodedData;

    uint8_t currentBit;
    uint16_t tableIdx;
    int16_t currentCode, previousCode = -1;
    uint16_t mask = 0x01;
    uint8_t defaultCodeSize = codeSize;
    uint16_t clearCode = 1 << codeSize;
    uint16_t endCode = clearCode + 1;
    LZW_TableEntry_t lzwTable[MAX_LZW_TABLE_SIZE];
    for (tableIdx = 0; tableIdx < clearCode; tableIdx++) {
        lzwTable[tableIdx].byte = tableIdx;
        lzwTable[tableIdx].previous = -1;
        lzwTable[tableIdx].length = 1;
    }
    tableIdx++; tableIdx++;

    while (dataSize) {
        currentCode = 0;

        // Get the current code
        for (uint8_t i = 0; i < (codeSize + 1); i++) {
            currentBit = ((*encodedData) & mask) ? 1 : 0;
            mask <<= 1;

            if (mask == 0x100) {
                mask = 0x01;
                encodedData += 1;
                dataSize--;
            }

            currentCode = currentCode | (currentBit << i);
        }

        // Handle the clear and end codes
        if (currentCode == clearCode) {
            codeSize = defaultCodeSize;
            for (tableIdx = 0; tableIdx < (1 << codeSize); tableIdx++) {
                lzwTable[tableIdx].byte = tableIdx;
                lzwTable[tableIdx].previous = -1;
                lzwTable[tableIdx].length = 1;
            }
            tableIdx++; tableIdx++;
            previousCode = -1;
            continue;
        } else if (currentCode == endCode) {
            if (dataSize > 1) {
                uefi_call_wrapper(BS->FreePool, 1, encodedDataPtr);
                return EFI_ABORTED;
            }
            break;
        }

        // Handle other codes representing actual data
        if ((previousCode > -1) && (codeSize < 12)) {
            if (currentCode > tableIdx) { 
                uefi_call_wrapper(BS->FreePool, 1, encodedDataPtr);
                return EFI_OUT_OF_RESOURCES;
            }

            if (currentCode == tableIdx) {
                int16_t ptr = previousCode;
                while (lzwTable[ptr].previous != -1) {
                    ptr = lzwTable[ptr].previous;
                }

                lzwTable[tableIdx].byte = lzwTable[ptr].byte;
            } else {
                int16_t ptr = currentCode;
                while (lzwTable[ptr].previous != -1) {
                    ptr = lzwTable[ptr].previous;
                }

                lzwTable[tableIdx].byte = lzwTable[ptr].byte;
            }

            lzwTable[tableIdx].previous = previousCode;
            lzwTable[tableIdx].length = lzwTable[previousCode].length + 1;

            tableIdx++;
            // GIF89a mandates that this stops at 12 bits
            if ((tableIdx == (1 << (codeSize + 1))) && (codeSize < 11)) {
                codeSize++;
            }
        }

        previousCode = currentCode;
        // Now copy the table entry backwards into output buffer
        int32_t matchLength = lzwTable[currentCode].length;
        while (currentCode != -1) {
            output[lzwTable[currentCode].length - 1] = lzwTable[currentCode].byte;
            if (lzwTable[currentCode].previous == currentCode) {
                uefi_call_wrapper(BS->FreePool, 1, encodedDataPtr);
                return EFI_ABORTED;
            }

            currentCode = lzwTable[currentCode].previous;
        }

        output += matchLength;
    }
    
    uefi_call_wrapper(BS->FreePool, 1, encodedDataPtr);
    return EFI_SUCCESS;
}

static EFI_STATUS GetGIFDimensions(uint8_t *gifData, uint32_t *width, uint32_t *height) {
    if (memcmp(gifData, "GIF87a", 6) != 0 && memcmp(gifData, "GIF89a", 6) != 0) return EFI_UNSUPPORTED;

    GIF_Header_t *header = (GIF_Header_t *)gifData;
    *width = header->width;
    *height = header->height;
    return EFI_SUCCESS;
}

static EFI_STATUS DrawGIF(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, uint8_t *gifData, uint32_t _x, uint32_t _y) {
    EFI_STATUS status;
    if (memcmp(gifData, "GIF87a", 6) != 0 && memcmp(gifData, "GIF89a", 6) != 0) return EFI_UNSUPPORTED;

    GIF_Header_t *header = NULL;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, sizeof(GIF_Header_t), (void**)&header);
    if (EFI_ERROR(status)) return status;

    memcpy(header, gifData, 13);
    UINT32 colorTableSize = (1 << ((header->flags & 0x07) + 1)) * 3;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, colorTableSize, (void**)&header->globalColorTable);
    if (EFI_ERROR(status)) {
        uefi_call_wrapper(BS->FreePool, 1, header);
        return status;
    }
    
    gifData += 13;
    memcpy(header->globalColorTable, gifData, colorTableSize);
    // Increment the dataPointer to the start of the first data block
    gifData += colorTableSize;

    status = EFI_SUCCESS;
    uint8_t *repeatedDataPointer = NULL;
    int32_t numRepetitions = 0;
    uint16_t frameDelay = 0;
    while (numRepetitions > -1) {
        if (repeatedDataPointer != NULL) gifData = repeatedDataPointer;

        while (status == EFI_SUCCESS) {
            switch (*gifData) {
                case GIF_EXTENSION_BLOCK: {
                    gifData++;
                    switch (*gifData) {
                        case GIF_APPLICATION_EXTENSION: {
                            GIF_ApplicationExtension_t extensionData;
                            status = HandleApplicationExtension(&gifData, &extensionData);
                            if (EFI_ERROR(status)) return EFI_ABORTED;
                            if (extensionData.numberOfRepetitions > -1) {
                                repeatedDataPointer = gifData;
                            }
                            numRepetitions = extensionData.numberOfRepetitions;
                            continue;
                        }
                        case GIF_COMMENT_EXTENSION:
                            status = HandleCommentExtension(&gifData);
                            continue;
                        case GIF_GRAPHIC_CONTROL_EXTENSION: {
                            GIF_GraphicControlExtension_t extensionData;
                            status = HandleGraphicControlExtension(&gifData, &extensionData);
                            if (EFI_ERROR(status)) return EFI_ABORTED;
                            if (extensionData.frameDelay) frameDelay = extensionData.frameDelay;
                            continue;
                        }
                        case GIF_PLAIN_TEXT_EXTENSION:
                            status = HandlePlainTextExtension(&gifData);
                            continue;
                    }
                    break;
                }
                case GIF_IMAGE_DESCRIPTOR: {
                    GIF_ImageDescriptorHeader_t imageDescHeader;
                    ReadImageDescriptorHeader(&gifData, &imageDescHeader);
                    uint8_t *uncompressedData = NULL;
                    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, imageDescHeader.width * imageDescHeader.height * sizeof(uint8_t), (void**)&uncompressedData);
                    if (EFI_ERROR(status)) break;
                    //status = DecodeImageDescriptorData(&gifData, imageDescHeader.codeSize, &uncompressedData);
                    status = DecodeImageDescriptorData(&gifData, imageDescHeader.codeSize, &uncompressedData);
                    if (EFI_ERROR(status)) break;
                    uint32_t pixPos = 0;
                    for (int y = _y; y < _y + imageDescHeader.height; y++) {
                        for (int x = _x; x < _x + imageDescHeader.width; x++) {
                            *((UINT32*)(gop->Mode->FrameBufferBase + 4 * gop->Mode->Info->PixelsPerScanLine * y + 4 * x)) = 
                                (header->globalColorTable[uncompressedData[pixPos]].b) | 
                                (header->globalColorTable[uncompressedData[pixPos]].g << 8) | 
                                (header->globalColorTable[uncompressedData[pixPos]].r << 16);
                            
                            pixPos++;
                        }
                    }
                    uefi_call_wrapper(BS->FreePool, 1, uncompressedData);
                    if (frameDelay) status = sleep(frameDelay * 100 * 1000);
                    break;
                }
                case GIF_TERMINATOR:
                    gifData++;
                    break;
                case GIF_EOF:
                    status = EFI_END_OF_FILE;
                    break;
                default:
                    status = EFI_NOT_FOUND;
                    break;
            }
        }

        if (numRepetitions == 1) numRepetitions = -1;
        else if (numRepetitions != 0) numRepetitions--;

        if (status == EFI_SUCCESS || status == EFI_END_OF_FILE) status = EFI_SUCCESS;
        else break;
    }
    
    uefi_call_wrapper(BS->FreePool, 1, header->globalColorTable);
    uefi_call_wrapper(BS->FreePool, 1, header);
    return status;
}

#endif