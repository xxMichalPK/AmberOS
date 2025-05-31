#ifndef __CONFIG_PARSER_H__
#define __CONFIG_PARSER_H__

#include <stdint.h>
#include <stddef.h>

#include <boot/legacy/loader/memory.h>

static int getSectionNameLen(char* str) {
    int len = 0;
    while (*str != '\0' && *str != ']') {
        len++;
        str++;
    }

    return len;
}

static int getSettingNameLen(char* str) {
    int len = 0;
    while (*str != '\0' && *str != ':') {
        len++;
        str++;
    }

    return len;
}

static int getSettingLen(char* str) {
    int len = 0;
    while (*str != '\0' && *str != ';' && *str != 0x0A) {
        len++;
        str++;
    }

    return len;
}

static int parseConfig(uint8_t* data, char* section, char* setting, uint8_t **outValue) {
    int sectionNameLen = getSectionNameLen(section);
    int settingNameLen = getSettingNameLen(setting);

    while (*data != '\0') {
        // Search for the section
        if (*data != '[') {
            data++;
            continue;
        }

        // We found a section of configuration. Compare the section's name length with the desired length
        if (getSectionNameLen((char*)(data + 1)) != sectionNameLen) {
            data++;
            continue;
        }

        // If the name length is the same compare the section name
        if (memcmp(section, (void*)(data + 1), sectionNameLen) != 0) {
            data++;
            continue;
        }

        // The section names match for now. Check if the section symbol is closed with ']' character
        data += sectionNameLen + 1;
        if (*data != ']') {
            data++;
            continue;
        }

        // If yes the desired section was found. Search for the setting
        data++;
        while (*data != '\0' && *data != '[') {
            // Search for the setting symbol '>'
            if (*data != '>') {
                data++;
                continue;
            }

            // We found a setting. Skip all the spaces
            data++;
            while (*data == ' ') data++;

            // Compare the setting's name length with the desired length
            if (getSettingNameLen((char*)data) != settingNameLen) {
                data++;
                continue;
            }

            // They are the same length. Check if it is the desired setting
            if (memcmp(setting, (void*)data, settingNameLen) != 0) {
                data++;
                continue;
            }

            // Check if it's the desired setting by checking the 'set setting' character ':'
            data += settingNameLen;
            if (*data != ':') {
                data++;
                continue;
            }

            // This is the setting we are looking for. Skip all the spaces, check its length and copy to the output buffer
            data++;
            while (*data == ' ') data++;
            int settingLen = getSettingLen((char*)data);
            (*outValue) = (uint8_t*)lmalloc(settingLen + 1);
            memcpy(*outValue, data, settingLen);
            (*outValue)[settingLen] = '\0';
            return settingLen;
        }
    }

    // We reached the end of the file so the setting wasn't found
    return -1;
}

#endif