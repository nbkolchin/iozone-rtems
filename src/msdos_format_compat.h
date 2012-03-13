#ifndef IOZONE_RTEMS_MSDOS_FORMAT_COMPAT_H
#define IOZONE_RTEMS_MSDOS_FORMAT_COMPAT_H

#include <stdint.h>

#define MS_BYTES_PER_CLUSTER_LIMIT (0x8000)
#define FAT_FAT12_MAX_CLN (4085)
#define FAT_FAT16_MAX_CLN (65525)
#define FAT_DIRENTRY_SIZE (32)
#define FAT_TOTAL_MBR_SIZE (512)
#define FAT_TOTAL_FSINFO_SIZE (512)

#define FAT_BR_OEMNAME_SIZE              (8)
#define FAT_BR_BOOTSIG_VAL               (0x29)
#define FAT_BR_VOLLAB_SIZE               (11)
#define FAT_BR_FILSYSTYPE_SIZE           (8)
#define FAT_BR_FAT32_RESERVED_SIZE       (12)
#define FAT_BR_FAT32_BOOTSIG_VAL         (0x29)
#define FAT_BR_FAT32_VOLLAB_SIZE         (11)
#define FAT_BR_FAT32_FILSYSTYPE_SIZE     (8)
#define FAT_BR_SIGNATURE_VAL             (0xAA55)
#define FAT_BR_EXT_FLAGS_MIRROR          (0x0080)
#define FAT_BR_EXT_FLAGS_FAT_NUM         (0x000F)
#define FAT_BR_MEDIA_FIXED               (0xf8)
#define FAT_FSI_INFO                     (484)

#define FAT_SET_VAL8(x, ofs,val)                    \
                 (*((uint8_t *)(x)+(ofs))=(uint8_t)(val))
 
#define FAT_SET_VAL16(x, ofs,val) do {              \
                 FAT_SET_VAL8((x),(ofs),(val));     \
                 FAT_SET_VAL8((x),(ofs)+1,(val)>>8);\
                 } while (0)

#define FAT_SET_VAL32(x, ofs,val) do {               \
                 uint32_t val1 = val;                \
                 FAT_SET_VAL16((x),(ofs),(val1)&0xffff);\
                 FAT_SET_VAL16((x),(ofs)+2,(val1)>>16);\
                 } while (0)

#define FAT_SET_BR_JMPBOOT(x,val)              FAT_SET_VAL8( x,  0,val)
#define FAT_SET_BR_BYTES_PER_SECTOR(x,val)     FAT_SET_VAL16(x, 11,val)
#define FAT_SET_BR_SECTORS_PER_CLUSTER(x,val)  FAT_SET_VAL8( x, 13,val) 
#define FAT_SET_BR_RESERVED_SECTORS_NUM(x,val) FAT_SET_VAL16(x, 14,val)
#define FAT_SET_BR_FAT_NUM(x,val)              FAT_SET_VAL8( x, 16,val)
#define FAT_SET_BR_FILES_PER_ROOT_DIR(x,val)   FAT_SET_VAL16(x, 17,val)
#define FAT_SET_BR_TOTAL_SECTORS_NUM16(x,val)  FAT_SET_VAL16(x, 19,val)
#define FAT_SET_BR_MEDIA(x,val)                FAT_SET_VAL8( x, 21,val) 
#define FAT_SET_BR_SECTORS_PER_FAT(x,val)      FAT_SET_VAL16(x, 22,val)
#define FAT_SET_BR_SECTORS_PER_TRACK(x,val)    FAT_SET_VAL16(x, 24,val)
#define FAT_SET_BR_NUMBER_OF_HEADS(x,val)      FAT_SET_VAL16(x, 26,val)
#define FAT_SET_BR_HIDDEN_SECTORS(x,val)       FAT_SET_VAL32(x, 28,val)
#define FAT_SET_BR_TOTAL_SECTORS_NUM32(x,val)  FAT_SET_VAL32(x, 32,val)
#define FAT_SET_BR_DRVNUM(x,val)               FAT_SET_VAL8( x, 36,val)
#define FAT_SET_BR_RSVD1(x,val)                FAT_SET_VAL8( x, 37,val)
#define FAT_SET_BR_BOOTSIG(x,val)              FAT_SET_VAL8( x, 38,val)
#define FAT_SET_BR_VOLID(x,val)                FAT_SET_VAL32(x, 39,val)
#define FAT_SET_BR_SECTORS_PER_FAT32(x,val)    FAT_SET_VAL32(x, 36,val)
#define FAT_SET_BR_EXT_FLAGS(x,val)            FAT_SET_VAL16(x, 40,val)
#define FAT_SET_BR_FSVER(x,val)                FAT_SET_VAL16(x, 42,val)
#define FAT_SET_BR_FAT32_ROOT_CLUSTER(x,val)   FAT_SET_VAL32(x, 44,val)
#define FAT_SET_BR_FAT32_FS_INFO_SECTOR(x,val) FAT_SET_VAL16(x, 48,val)
#define FAT_SET_BR_FAT32_BK_BOOT_SECTOR(x,val) FAT_SET_VAL16(x, 50,val)
#define FAT_SET_BR_FAT32_DRVNUM(x,val)         FAT_SET_VAL8( x, 64,val)
#define FAT_SET_BR_FAT32_RSVD1(x,val)          FAT_SET_VAL8( x, 65,val)
#define FAT_SET_BR_FAT32_BOOTSIG(x,val)        FAT_SET_VAL8( x, 66,val)
#define FAT_SET_BR_FAT32_VOLID(x,val)          FAT_SET_VAL32(x, 67,val)
#define FAT_SET_BR_SIGNATURE(x,val)            FAT_SET_VAL16(x,510,val)

#define FAT_GET_ADDR(x, ofs)                 ((uint8_t *)(x) + (ofs))
#define FAT_GET_ADDR_BR_OEMNAME(x)           FAT_GET_ADDR( x,  3)
#define FAT_GET_ADDR_BR_VOLLAB(x)            FAT_GET_ADDR (x, 43)
#define FAT_GET_ADDR_BR_FILSYSTYPE(x)        FAT_GET_ADDR (x, 54)
#define FAT_GET_ADDR_BR_FAT32_RESERVED(x)    FAT_GET_ADDR (x, 52)
#define FAT_GET_ADDR_BR_FAT32_VOLLAB(x)      FAT_GET_ADDR (x, 71)
#define FAT_GET_ADDR_BR_FAT32_FILSYSTYPE(x)  FAT_GET_ADDR (x, 82)

#define FAT_FAT12_EOC          (0x0FF8)
#define FAT_FAT16_EOC          (0xFFF8)
#define FAT_FAT32_EOC          (uint32_t)0x0FFFFFF8

#define FAT_FSINFO_LEAD_SIGNATURE_VALUE      (0x41615252)
#define FAT_FSINFO_STRUC_SIGNATURE_VALUE     (0x61417272)
#define FAT_FSINFO_TRAIL_SIGNATURE_VALUE     (0x000055AA)
#define FAT_FSINFO_STRUCT_OFFSET             488
#define FAT_FSINFO_FREE_CLUSTER_COUNT_OFFSET (FAT_FSINFO_STRUCT_OFFSET+0)
#define FAT_FSINFO_NEXT_FREE_CLUSTER_OFFSET  (FAT_FSINFO_STRUCT_OFFSET+4)

#define FAT_SET_FSINFO_LEAD_SIGNATURE(x,val)      FAT_SET_VAL32(x,  0,val)
#define FAT_SET_FSINFO_STRUC_SIGNATURE(x,val)     FAT_SET_VAL32(x,484,val)
#define FAT_SET_FSINFO_TRAIL_SIGNATURE(x,val)     FAT_SET_VAL32(x,508,val)
#define FAT_SET_FSINFO_FREE_CLUSTER_COUNT(x,val)  FAT_SET_VAL32(x, 4,val)
#define FAT_SET_FSINFO_NEXT_FREE_CLUSTER(x,val)   FAT_SET_VAL32(x, 8,val)

#define MSDOS_DIR_NAME(x)     (char     *)((x) + 0)
#define MSDOS_SHORT_BASE_LEN  8  /* 8 characters */
#define MSDOS_SHORT_EXT_LEN   3  /* 3 characters */
#define MSDOS_SHORT_NAME_LEN  (MSDOS_SHORT_BASE_LEN+MSDOS_SHORT_EXT_LEN)
#define MSDOS_DIR_ATTR(x)     (uint8_t  *)((x) + 11)
#define MSDOS_ATTR_VOLUME_ID  (0x08)

#endif
