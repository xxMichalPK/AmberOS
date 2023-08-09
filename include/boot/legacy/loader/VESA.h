#ifndef __VESA_H__
#define __VESA_H__

#include <boot/legacy/loader/BIOS.h>
#include <boot/legacy/loader/memory.h>
#include <boot/legacy/loader/math.h>

typedef struct {
   uint8_t signature[4];        // == "VESA"
   uint16_t version;            // == 0x0300 for VBE 3.0
   uint32_t OEM_string_ptr;     // isa vbeFarPtr
   uint8_t capabilities[4];
   uint32_t vModePtr;           // isa vbeFarPtr
   uint16_t totalMemory;        // as # of 64KB blocks
   uint8_t reserved[492];
} __attribute__((packed)) VBE_INFO_t;

// VBE Mode info block - holds current graphics mode values
typedef struct {
    // Mandatory info for all VBE revisions
	uint16_t mode_attributes;
	uint8_t window_a_attributes;
	uint8_t window_b_attributes;
	uint16_t window_granularity;
	uint16_t window_size;
	uint16_t window_a_segment;
	uint16_t window_b_segment;
	uint32_t window_function_pointer;
	uint16_t bytes_per_scanline;

    // Mandatory info for VBE 1.2 and above
	uint16_t x_resolution;
	uint16_t y_resolution;
	uint8_t x_charsize;
	uint8_t y_charsize;
	uint8_t number_of_planes;
	uint8_t bits_per_pixel;
	uint8_t number_of_banks;
	uint8_t memory_model;
	uint8_t bank_size;
	uint8_t number_of_image_pages;
	uint8_t reserved1;

    // Direct color fields (required for direct/6 and YUV/7 memory models)
	uint8_t red_mask_size;
	uint8_t red_field_position;
	uint8_t green_mask_size;
	uint8_t green_field_position;
	uint8_t blue_mask_size;
	uint8_t blue_field_position;
	uint8_t reserved_mask_size;
	uint8_t reserved_field_position;
	uint8_t direct_color_mode_info;

    // Mandatory info for VBE 2.0 and above
	uint32_t physical_base_pointer;         // Physical address for flat memory frame buffer
	uint32_t offscreen_memory_offset;
	uint16_t offscreen_memory_size;

    // Mandatory info for VBE 3.0 and above
	uint16_t linear_bytes_per_scanline;
    uint8_t bank_number_of_image_pages;
    uint8_t linear_number_of_image_pages;
    uint8_t linear_red_mask_size;
    uint8_t linear_red_field_position;
    uint8_t linear_green_mask_size;
    uint8_t linear_green_field_position;
    uint8_t linear_blue_mask_size;
    uint8_t linear_blue_field_position;
    uint8_t linear_reserved_mask_size;
    uint8_t linear_reserved_field_position;
    uint32_t max_pixel_clock;

    uint8_t reserved4[190];              // Remainder of mode info block

} __attribute__ ((packed)) VBE_MODE_INFO_t;

static VBE_INFO_t gVBEInformation __attribute__((aligned(0x100)));
static VBE_MODE_INFO_t gVModeInformation __attribute__((aligned(0x100)));
static int SetVideoMode(uint32_t hRes, uint32_t vRes, uint8_t bpp) {
    rmode_regs_t regs;
    // Initialize all the segments with the segment of current address
    regs.ds = RMODE_SEGMENT(&_loadAddr);
    regs.es = RMODE_SEGMENT(&_loadAddr);
    regs.fs = RMODE_SEGMENT(&_loadAddr);
    regs.gs = RMODE_SEGMENT(&_loadAddr);

    // Get VBE information
    regs.eax = 0x4F00;
    regs.edi = RMODE_OFFSET(&gVBEInformation);
    bios_call_wrapper(0x10, &regs);
    if (memcmp(gVBEInformation.signature, "VESA", 4) != 0) return -1;
    if (gVBEInformation.version < 0x0200) return -1;

    // If we are here the VBE is supported!
    // Find the requested video mode or the closest fit...
    uint32_t modesAddr = (uint32_t)&gVBEInformation.vModePtr;
    uint16_t *modes = (uint16_t*)modesAddr;
    int32_t bestModeIdx = 0;
    uint32_t bestModeScore = 0xFFFFFFFF;
    int32_t modeIdx = -1;
    for (int32_t i = 0; modes[i] != 0xFFFF; i++) {
        // Read the mode information
        regs.eax = 0x4F01;
        regs.ecx = modes[i];
        regs.edi = RMODE_OFFSET(&gVModeInformation);
        bios_call_wrapper(0x10, &regs);

        // Check if it supports linear framebuffer. If not go to next one
        if ((gVModeInformation.mode_attributes & 0x90) != 0x90) continue;

        // Check the requested dimensions
        if (gVModeInformation.x_resolution == hRes && gVModeInformation.y_resolution == vRes && gVModeInformation.bits_per_pixel == bpp) {
            modeIdx = i;
            break;
        }

        // Otherwise, calculate the score which indicates how close the current mode is to the desired one
        uint32_t currentScore = ABS((int32_t)gVModeInformation.x_resolution - (int32_t)hRes) + 
                                ABS((int32_t)gVModeInformation.y_resolution - (int32_t)vRes) + 
                                ABS((int32_t)gVModeInformation.bits_per_pixel - (int32_t)bpp);
        if (currentScore < bestModeScore) {
            bestModeIdx = i;
            bestModeScore = currentScore;
        }
    }

    // If the desired mode wasn't found get the best mode found information
    if (modeIdx == -1) {
        modeIdx = bestModeIdx;
        regs.eax = 0x4F01;
        regs.ecx = modes[modeIdx];
        regs.edi = RMODE_OFFSET(&gVModeInformation);
        bios_call_wrapper(0x10, &regs);
    }

    regs.eax = 0x4F02;
    regs.ebx = modes[modeIdx] | 0x4000;
    regs.ecx = 0;
    regs.edi = 0;
    bios_call_wrapper(0x10, &regs);
    return 0;
}

#endif