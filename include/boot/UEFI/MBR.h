#ifndef __MBR_H__
#define __MBR_H__

#include <efi/efi.h>
#include <efi/efiapi.h>

typedef struct MBR_Partition {
    UINT8 driveAttrib[1];
    UINT8 startCHS[3];
    UINT8 partType[1];
    UINT8 lastSectorCHS[3];
    UINT8 startLBA[4];
    UINT8 sectorsCount[4];
} MBR_Partition_t;

typedef struct AmberOS_MBR {
    UINT8 start[2];
    UINT8 os_sig[2];
    UINT8 code[436];
    UINT8 unique_disk_ID[4];
    UINT8 reserved[2];
    MBR_Partition_t partitions[4];
    UINT8 bootSig[2];
} AmberOS_MBR_t;

#endif