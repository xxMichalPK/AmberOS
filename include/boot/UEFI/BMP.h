#ifndef __BMP_H__
#define __BMP_H__

#include <efi/efi.h>
#include <efi/efilib.h>
#include <boot/UEFI/memory.h>

// BMP DIB Header sizes
#define BITMAPCOREHEADER 12
#define OS22XBITMAPHEADER 64
#define BITMAPINFOHEADER 40
#define BITMAPV2INFOHEADER 52
#define BITMAPV3INFOHEADER 56
#define BITMAPV4HEADER 108
#define BITMAPV5HEADER 124

// BMP Compressions
#define BI_RGB 0
#define BI_RLE8 1
#define BI_RLE4 2
#define BI_BITFIELDS 3
#define BI_JPEG 4
#define BI_PNG 5
#define BI_ALPHABITFIELDS 6
#define BI_CMYK 11
#define BI_CMYKRLE8 12
#define BI_CMYKRLE4 13

typedef struct {
    UINT8 signature[2];
    UINT32 fileSize;
    UINT32 reserved;
    UINT32 dataOffset;
} __attribute__((packed)) BMP_Header_t;

typedef struct {
    UINT32 headerSize;
    UINT16 width;
    UINT16 height;
    UINT16 colorPlanes;
    UINT16 bpp;
} __attribute__((packed)) BITMAPCOREHEADER_t;

typedef struct {
    UINT32 headerSize;
    UINT32 width;
    UINT32 height;
    UINT16 colorPlanes;
    UINT16 bpp;
    UINT32 compression;
    UINT32 imageSize;
    UINT32 hResolution;
    UINT32 vResolution;
    UINT32 colorNum;
    UINT32 importantColorNum;
} __attribute__((packed)) BITMAPINFOHEADER_t;

static EFI_STATUS GetBMPSize(UINT8 *bmpData, UINT32 *width, UINT32 *height) {
    BMP_Header_t *header = (BMP_Header_t*)bmpData;
    if (header->signature[0] != 'B' || header->signature[1] != 'M') {
        return EFI_UNSUPPORTED;
    }

    UINT32 dibLength = bmpData[14] | (bmpData[15] << 8) | (bmpData[16] << 16) | (bmpData[17] << 24);
    switch (dibLength) {
        case BITMAPCOREHEADER: {
            BITMAPCOREHEADER_t *dibHeader = (BITMAPCOREHEADER_t*)(bmpData + 14);
            *width = dibHeader->width;
            *height = dibHeader->height;
            return EFI_SUCCESS;
        }
        case BITMAPINFOHEADER:
        case BITMAPV5HEADER: {
            BITMAPINFOHEADER_t *dibHeader = (BITMAPINFOHEADER_t*)(bmpData + 14);
            *width = dibHeader->width;
            *height = dibHeader->height;
            return EFI_SUCCESS;
        }
        default: {
            return EFI_UNSUPPORTED;
        }
    }

    return EFI_UNSUPPORTED;
}

static EFI_STATUS DrawBMP(EFI_GRAPHICS_OUTPUT_PROTOCOL *gop, UINT8 *bmpData, UINT32 _x, UINT32 _y) {
    BMP_Header_t *header = (BMP_Header_t*)bmpData;
    if (header->signature[0] != 'B' || header->signature[1] != 'M') {
        return EFI_UNSUPPORTED;
    }

    UINT32 imageWidth = 0;
    UINT32 imageHeight = 0;
    UINT16 imageBPP = 0;
    UINT32 dibLength = bmpData[14] | (bmpData[15] << 8) | (bmpData[16] << 16) | (bmpData[17] << 24);
    switch (dibLength) {
        case BITMAPCOREHEADER: {
            BITMAPCOREHEADER_t *dibHeader = (BITMAPCOREHEADER_t*)(bmpData + 14);
            imageWidth = dibHeader->width;
            imageHeight = dibHeader->height;
            imageBPP = dibHeader->bpp;
            break;
        }
        case BITMAPINFOHEADER:
        case BITMAPV5HEADER: {
            BITMAPINFOHEADER_t *dibHeader = (BITMAPINFOHEADER_t*)(bmpData + 14);
            imageWidth = dibHeader->width;
            imageHeight = dibHeader->height;
            imageBPP = dibHeader->bpp;
            if (dibHeader->compression != BI_RGB) return EFI_UNSUPPORTED;
            break;
        }
        default: {
            return EFI_UNSUPPORTED;
        }
    }

    UINT16 imageBytesPerPixel = imageBPP / 8;
    UINT32 padding = imageWidth % 4;
    for (UINT32 y = _y + imageHeight - 1, pixPos = header->dataOffset; y >= _y; y--) {
        for (UINT32 x = _x; x < _x + imageWidth; x++) {
            *((UINT32*)(gop->Mode->FrameBufferBase + 4 * gop->Mode->Info->PixelsPerScanLine * y + 4 * x)) = (bmpData[pixPos]) | (bmpData[pixPos + 1] << 8)| (bmpData[pixPos + 2] << 16);
            pixPos += imageBytesPerPixel;
        }

        pixPos += padding;
    }

    return EFI_SUCCESS;
}

#endif