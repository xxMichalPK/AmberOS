#ifndef __TEXT_OUTPUT_H__
#define __TEXT_OUTPUT_H__

#include <stdint.h>
#include <stddef.h>

#define VGA_TEXT_WIDTH 80
#define VGA_TEXT_HEIGHT 25
#define VGA_TEXT_ADDRESS 0xB8000

static uint16_t gTextXPos, gTextYPos;
static uint8_t gTextFG = 0xF;
static uint8_t gTextBG = 0x0;

static void setTextFormat(uint8_t fg, uint8_t bg) {
    gTextFG = fg;
    gTextBG = bg;
}

static void lputc(char c) {
    switch (c) {
        case '\n': gTextYPos++;
        case '\r': gTextXPos = 0;
        case '\0': break;
        default: {
            uint8_t attributes = ((gTextBG & 0x0F) & 0x0F) | gTextFG & 0x0F; 
            *((uint16_t*)(VGA_TEXT_ADDRESS + (gTextYPos * VGA_TEXT_HEIGHT + gTextXPos * 2))) = c | (attributes << 8);
            
            if (gTextXPos == VGA_TEXT_WIDTH * 2) {
                gTextXPos = 0;
                gTextYPos++;
                return;
            }

            gTextXPos++;
        }
    }
}

static void lputs(char const *str) {
    while (*str != '\0') {
        lputc(*str);
        str++;
    }
}

#endif