#ifndef IOZONE_RTEMS_MSDOS_FORMAT_H
#define IOZONE_RTEMS_MSDOS_FORMAT_H

#include <rtems/dosfs.h>

#ifndef MSDOS_FMT_FATANY

#define MSDOS_FMT_FATANY 0
#define MSDOS_FMT_FAT12  1
#define MSDOS_FMT_FAT16  2
#define MSDOS_FMT_FAT32  3

/** @brief MSDOS filesystem format parameters.
 *
 * Data to be filled out for formatter: parameters for format call
 * any parameter set to 0 or NULL will be automatically detected/computed.
 */
typedef struct {
  const char *OEMName;            /**< OEM Name string or NULL               */
  const char *VolLabel;           /**< Volume Label string or NULL           */
  uint32_t  sectors_per_cluster;  /**< Sectors per cluster    */
  uint32_t  fat_num;              /**< Mumber of FATs on disk */
  uint32_t  files_per_root_dir;   /**< File entries in root   */
  uint8_t   fattype;              /**< Fat type (MSDOS_FMT_FAT12/16/32) */
  uint8_t   media;                /**< Media code (default: 0xF8)             */
  bool      quick_format;         /**< If true then do not clear out data sectors   */
  uint32_t  cluster_align;        /**< Cluster alignment                    
                                       make sector number of first sector  
                                       of first cluster divisible by this  
                                       value. This can optimize clusters   
                                       to be located at start of track     
                                       or start of flash block */
} msdos_format_request_param_t;

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Format device with MSDOS filesystem.
 *
 * @param[in]  devname Device name.
 * @param[in]  rqdata  Pointer to requested format parameters,
 *                     could be NULL for automatic determination.
 * @return 0: OK.
 * @return -1: Error occurred. Errno is set to real error value.
 */
int msdos_format
(
 const char *devname,                        /* IN */
 const msdos_format_request_param_t *rqdata  /* IN */
);

#ifdef __cplusplus
}
#endif

#endif /* !defined(MSDOS_FMT_FATANY) */

#endif
