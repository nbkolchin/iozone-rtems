/*! @file
 *  @brief this file contains msdos_format function. This function
 *  formats a disk partition conforming to MS-DOS conventions.
 *
 *  Copyright (c) 2004 IMD Ingenieurbuero fuer Microcomputertechnik
 *  Author: Th. Doerfler <Thomas.Doerfler@imd-systems.de>
 *
 *  The license and distribution terms for this file may be       
 *  found in the file LICENSE in this distribution or at           
 *  http://www.rtems.com/license/LICENSE.                          
 *
 *  $Id: msdos_format.c, 2004/10/29 Exp $
 */
#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include <rtems/dosfs.h>

#ifndef MSDOS_FMT_FATANY

#include <rtems/libio_.h>
#include <rtems/seterr.h>
#include <rtems/diskdevs.h>

#include "msdos_format.h"
#include "msdos_format_compat.h"

#ifndef ENOTBLK
#define ENOTBLK EINVAL
#endif

/** @brief MSDOS filesystem format parameters
*/
typedef struct {
  uint32_t bytes_per_sector;               /**< Bytes per sector (512 as a rule) */
  uint32_t totl_sector_cnt;                /**< Total sector count */
  uint32_t rsvd_sector_cnt;                /**< Reserved sector count.
                                                It is recomended to set this value
                                                to 1 for FAT12/16 and to 32 for FAT32 */
  uint32_t sectors_per_cluster;            /**< Sectors per cluster */
  uint32_t sectors_per_fat;                /**< Sectors needed for one FAT */

  uint32_t fat_start_sec;                  /**< Unused */
  uint32_t files_per_root_dir;             /**< Files per root directory.
                                                Should be 0 for FAT32 */
  uint32_t root_dir_sectors;               /**< Root directory sectors */
  uint32_t root_dir_start_sec;             /**< Location of root directory */
  uint32_t root_dir_fmt_sec_cnt;           /**< Roor directory sectors count */
  uint32_t mbr_copy_sec;                   /**< Location of copy of mbr or 0 */
  uint32_t fsinfo_sec;                     /**< Location of fsinfo sector or 0 */
  uint8_t  fat_num;                        /**< Number of FAT copies */
  uint8_t  media_code;                     /**< Media code */
  uint8_t  fattype;                        /**< FAT type */
  char     OEMName[FAT_BR_OEMNAME_SIZE+1]; /**< OEM Name string */
  char     VolLabel[FAT_BR_VOLLAB_SIZE+1]; /**< Volume label */
  bool     VolLabel_present;               /**< true if volume label is present */
  uint32_t vol_id;                         /**< Volume ID */
}  msdos_format_param_t;

/** @brief Write to sector.
 *
 * @param[in]  fd           File descriptor index.
 * @param[in]  start_sector Sector number to write to.
 * @param[in]  sector_size  Size of sector.
 * @param[in]  buffer       Buffer with  write data.
 *
 * @return 0: OK.
 * @return -1: Error occured. Errno is set to real error value.
 */
static int msdos_format_write_sec
(
 int         fd,                       /* IN */
 uint32_t    start_sector,             /* IN */
 uint32_t    sector_size,              /* IN */
 const char *buffer                    /* IN */
 )
{
  int ret_val = 0;

  if (0 > lseek(fd,((off_t)start_sector)*sector_size,SEEK_SET)) {
    ret_val = -1;
  }
  if (ret_val == 0) {
    if (0 > write(fd,buffer,sector_size)) {
      ret_val = -1;
    }
  }

  return ret_val;
}

/** @brief Fill sectors with byte.
 *
 * @param[in] fd             File descriptor index.
 * @param[in] start_secror   Sector number to fill to.
 * @param[in] sector_cnt     Number of sectors to fill to.
 * @param[in[ sector_size    Size of sector.
 * @param[in] fill_byte      Byte to fill into sectors.
 *
 * @return 0: OK.
 * @return -1: Error occured. Errno is set to real error value.
 */
static int msdos_format_fill_sectors
(
 int         fd,                       /* IN */
 uint32_t    start_sector,             /* IN */
 uint32_t    sector_cnt,               /* IN */
 uint32_t    sector_size,              /* IN */
 const char  fill_byte                 /* IN */
 )
{
  int ret_val = 0;
  char *fill_buffer = NULL;

  /*
   * allocate and fill buffer
   */
  if (ret_val == 0) {
    fill_buffer = malloc(sector_size);
    if (fill_buffer == NULL) {
      errno = ENOMEM;
      ret_val = -1;
    }
    else {
      memset(fill_buffer,fill_byte,sector_size);
    }
  }
  /*
   * write to consecutive sectors
   */
  while ((ret_val == 0) && 
	 (sector_cnt > 0)) {
    ret_val = msdos_format_write_sec(fd,start_sector,sector_size,fill_buffer);
    start_sector++;
    sector_cnt--;
  }
  /*
   * cleanup
   */
  if (fill_buffer != NULL) {
    free(fill_buffer);
    fill_buffer = NULL;
  }
  return ret_val;
}


/** @brief Generate pseudo-random volume id.
 *
 * @param[out] volid_prt Pointer to volume ID.
 *
 * @return 0: OK.
 * @return -1: Error occured. Errno is set to real error value.
 */
static int msdos_format_gen_volid
(
 uint32_t *volid_ptr                   /* OUT */
 )
{
  int ret_val = 0;
  int rc;
  rtems_clock_time_value time_value;

  rc = rtems_clock_get(RTEMS_CLOCK_GET_TIME_VALUE,&time_value);
  if (rc == RTEMS_SUCCESSFUL) {
    *volid_ptr = time_value.seconds + time_value.microseconds;
  }
  else {
    *volid_ptr = rand();
  }

  return ret_val;
}


/** @brief Check/adjust sectors_per_cluster to legal values.
 *
 * @param[in]  fattype                 FAT type (FAT_FAT12, FAT_FAT16, ...).
 * @param[in]  bytes_per_sector        Bytes count per sector (512 as a rule).
 * @param[in]  fatdata_sec_cnt         Sectors available for FAT and data.
 * @param[in]  fat_num                 Number of FAT copies.
 * @param[in]  sectors_per_cluster     Sectors per cluster (requested).
 * @param[out] sectors_per_cluster_adj Pointer to sectors per cluster (granted).
 * @param[out] sectors_per_fat_ptr     Sectors needed for one FAT.
 *
 * @return 0: OK.
 * @return -1: Error occured. Errno is set to real error value.
 */
static int msdos_format_eval_sectors_per_cluster
(
 int       fattype,                  /* IN */
 uint32_t  bytes_per_sector,         /* IN */
 uint32_t  fatdata_sec_cnt,          /* IN */
 uint8_t   fat_num,                  /* IN */
 uint32_t  sectors_per_cluster,      /* IN  */
 uint32_t *sectors_per_cluster_adj,  /* OUT */
 uint32_t *sectors_per_fat_ptr       /* OUT */
 )
{

  bool     finished = false;
  int      ret_val = 0;
  uint32_t fatdata_cluster_cnt;
  uint32_t fat_capacity;
  uint32_t sectors_per_fat;
  uint32_t data_cluster_cnt;
  /*
   * ensure, that maximum cluster size (32KByte) is not exceeded
   */
  while (MS_BYTES_PER_CLUSTER_LIMIT / bytes_per_sector < sectors_per_cluster) {
    sectors_per_cluster /= 2;
  }
  
  do {
    /*
     * compute number of data clusters for current data:
     * - compute cluster count for data AND fat
     * - compute storage size for FAT
     * - subtract from total cluster count
     */
    fatdata_cluster_cnt = fatdata_sec_cnt/sectors_per_cluster;
    if (fattype == FAT_FAT12) {
      fat_capacity = fatdata_cluster_cnt * 3 / 2;
    }
    else if (fattype == FAT_FAT16) {
      fat_capacity = fatdata_cluster_cnt * 2;
    }
    else { /* FAT32 */
      fat_capacity = fatdata_cluster_cnt * 4;
    }

    sectors_per_fat = ((fat_capacity 
			+ (bytes_per_sector - 1))
		       / bytes_per_sector);

    data_cluster_cnt = (fatdata_cluster_cnt -
			(((sectors_per_fat * fat_num)
			  + (sectors_per_cluster - 1))
			 / sectors_per_cluster));
    /*
     * data cluster count too big? then make sectors bigger
     */
    if (((fattype == FAT_FAT12) && (data_cluster_cnt > FAT_FAT12_MAX_CLN)) ||
        ((fattype == FAT_FAT16) && (data_cluster_cnt > FAT_FAT16_MAX_CLN))) {
      sectors_per_cluster *= 2;
    }
    else {
      finished = true;
    }
    /*
     * when maximum cluster size is exceeded, we have invalid data, abort...
     */
    if ((sectors_per_cluster * bytes_per_sector) 
	> MS_BYTES_PER_CLUSTER_LIMIT) {
      ret_val = EINVAL;
      finished = true;
    }
  } while (!finished);

  if (ret_val != 0) {
    rtems_set_errno_and_return_minus_one(ret_val);
  }
  else {
    *sectors_per_cluster_adj = sectors_per_cluster;
    *sectors_per_fat_ptr     = sectors_per_fat;
    return 0;
  }
}


/** @brief Determine parameters for formatting.
 *
 * If requested format parameters is specified then try to use it.
 *
 * @param[in]  dd         Pointer to disc device structure.
 * @param[in]  rqdata     Pointer to requested format parameters.
 *                        Set to NULL for fully-automatic determination.
 * @param[out] fmt_params Pointer to computed format parameters.
 *
 * @return 0: OK.
 * @return -1: Error occured. Errno is set to real error value. 
 */
static int msdos_format_determine_fmt_params
(
 const rtems_disk_device            *dd,       /* IN */
 const msdos_format_request_param_t *rqdata,   /* IN */
 msdos_format_param_t               *fmt_params/* OUT */
 )
{
  int ret_val = 0;
  uint32_t fatdata_sect_cnt;
  uint32_t onebit;
  uint32_t sectors_per_cluster_adj = 0;

  memset(fmt_params,0,sizeof(*fmt_params));
  /* 
   * this one is fixed in this implementation.
   * At least one thing we don't have to magically guess...
   */
  if (ret_val == 0) {
    fmt_params->bytes_per_sector = dd->block_size;
    fmt_params->totl_sector_cnt  = dd->size;
  }
  /*
   * determine number of FATs
   */
  if (ret_val == 0) {
    if ((rqdata == NULL) || 
	(rqdata->fat_num == 0)) {
      fmt_params->fat_num = 2;
    }
    else if (rqdata->fat_num <= 6) {
      fmt_params->fat_num = rqdata->fat_num;
    }
    else {
      ret_val = EINVAL;
    }
  }
  /*
   * Now we get sort of a loop when determining things:
   * The FAT type (FAT12/16/32) is determined ONLY from the 
   * data cluster count:
   * Disks with data cluster count <  4085 are FAT12.
   * Disks with data cluster count < 65525 are FAT16.
   * The rest is FAT32 (no FAT128 available yet :-) 
   *
   * The number of data clusters is the 
   * total capacity 
   * minus reserved sectors
   * minus root directory ares
   * minus storage needed for the FAT (and its copy/copies).
   * 
   * The last item once again depends on the FAT type and the cluster count.
   * 
   * So here is what we do in this formatter:
   * - If a FAT type is requested from the caller, we try to modify
   * the cluster size, until the data cluster count is in range
   * - If no FAT type is given, we estimate a useful FAT type from 
   * the disk capacity and then adapt the cluster size
   */  

  /*
   * determine characteristic values:
   * - number of sectors
   * - number of reserved sectors
   * - number of used sectors
   * - sectors per cluster
   */
  /*
   * determine FAT type and sectors per cluster
   * depends on 
   */
  if (ret_val == 0) {
    fmt_params->sectors_per_cluster = 1;
    if ((rqdata != NULL) && 
	(rqdata->fattype == MSDOS_FMT_FAT12)) {
      fmt_params->fattype = FAT_FAT12;
    }
    else if ((rqdata != NULL) && 
	     (rqdata->fattype == MSDOS_FMT_FAT16)) {
      fmt_params->fattype = FAT_FAT16;
    }
    else if ((rqdata != NULL) && 
	     (rqdata->fattype == MSDOS_FMT_FAT32)) {
      fmt_params->fattype = FAT_FAT32;
    }
    else if ((rqdata != NULL) &&
	     (rqdata->fattype != MSDOS_FMT_FATANY)) {
      ret_val = -1;
      errno = EINVAL;
    }
    else {
      /*
       * limiting values for disk size, fat type, sectors per cluster
       * NOTE: maximum sect_per_clust is arbitrarily choosen with values that
       * are a compromise concerning capacity and efficency
       */
      if (fmt_params->totl_sector_cnt 
	  < ((uint32_t)FAT_FAT12_MAX_CLN)*8) {
	fmt_params->fattype = FAT_FAT12;
	/* start trying with small clusters */
	fmt_params->sectors_per_cluster = 2; 
      }
      else if (fmt_params->totl_sector_cnt 
	       < ((uint32_t)FAT_FAT16_MAX_CLN)*32) {
	fmt_params->fattype = FAT_FAT16;
	/* start trying with small clusters */
	fmt_params->sectors_per_cluster = 2; 
      }
      else {
	fmt_params->fattype = FAT_FAT32;
	/* start trying with small clusters... */
	fmt_params->sectors_per_cluster = 1; 
      }
    }
    /*
     * try to use user requested cluster size
     */
    if ((rqdata != NULL) && 
	(rqdata->sectors_per_cluster > 0)) {
      fmt_params->sectors_per_cluster = 
	rqdata->sectors_per_cluster;    
    }
    /*
     * check sectors per cluster.
     * must be power of 2
     * must be smaller than or equal to 128
     * sectors_per_cluster*bytes_per_sector must not be bigger than 32K
     */
    for (onebit = 128;onebit >= 1;onebit = onebit>>1) {
      if (fmt_params->sectors_per_cluster > onebit) {
	fmt_params->sectors_per_cluster = onebit;
	if (fmt_params->sectors_per_cluster 
	    <= 32768L/fmt_params->bytes_per_sector) {
	  /* value is small enough so this value is ok */
	  onebit = 1;
	}
      }
    }
  }

  if (ret_val == 0) {
    if (fmt_params->fattype == FAT_FAT32) {
      /* recommended: for FAT32, always set reserved sector count to 32 */
      fmt_params->rsvd_sector_cnt = 32;
      /* for FAT32, always set files per root directory 0 */
      fmt_params->files_per_root_dir = 0;
      /* location of copy of MBR */
      fmt_params->mbr_copy_sec = 6;
      /* location of fsinfo sector */
      fmt_params->fsinfo_sec = 1;

    }
    else {
      /* recommended: for FAT12/FAT16, always set reserved sector count to 1 */
      fmt_params->rsvd_sector_cnt = 1;
      /* recommended: for FAT16, set files per root directory to 512 */
      /* for FAT12/FAT16, set files per root directory */
      /* must fill up an even count of sectors         */
      if ((rqdata != NULL) && 
	  (rqdata->files_per_root_dir > 0)) {
	fmt_params->files_per_root_dir = rqdata->files_per_root_dir;
      }
      else {
	if (fmt_params->fattype == FAT_FAT16) {
	  fmt_params->files_per_root_dir = 512;
	}
	else {
	  fmt_params->files_per_root_dir = 64;
	}
      }
      fmt_params->files_per_root_dir = (fmt_params->files_per_root_dir + 
			    (2*fmt_params->bytes_per_sector/
			     FAT_DIRENTRY_SIZE-1));
      fmt_params->files_per_root_dir -= (fmt_params->files_per_root_dir % 
			     (2*fmt_params->bytes_per_sector
			      /FAT_DIRENTRY_SIZE));
    }
    fmt_params->root_dir_sectors = 
      (((fmt_params->files_per_root_dir * FAT_DIRENTRY_SIZE)
	+ fmt_params->bytes_per_sector - 1) 
       / fmt_params->bytes_per_sector);
  }
  if (ret_val == 0) {
    fatdata_sect_cnt = (fmt_params->totl_sector_cnt - 
			fmt_params->rsvd_sector_cnt - 
			fmt_params->root_dir_sectors);
			
    /*
     * check values to get legal arrangement of FAT type and cluster count
     */

    ret_val = msdos_format_eval_sectors_per_cluster
      (fmt_params->fattype,
       fmt_params->bytes_per_sector,
       fatdata_sect_cnt,
       fmt_params->fat_num,
       fmt_params->sectors_per_cluster,
       &sectors_per_cluster_adj,
       &(fmt_params->sectors_per_fat));
    fmt_params->sectors_per_cluster = sectors_per_cluster_adj;
  }

  /*
   * determine media code
   */
  if (ret_val == 0) {
    if ((rqdata != NULL) && 
	(rqdata->media != 0)) {
      const char valid_media_codes[] = 
	{0xF0,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF};
      if (NULL==memchr(valid_media_codes,
		       rqdata->media,
		       sizeof(valid_media_codes))) {
	ret_val = -1;
	errno = EINVAL;
      }
      else {
	fmt_params->media_code = rqdata->media;
      }
    }
    else {
      fmt_params->media_code = FAT_BR_MEDIA_FIXED;
    }
  }
  /*
   * determine location and size of root directory
   * for formatting
   */
  if (fmt_params->root_dir_sectors > 0) {
    fmt_params->root_dir_start_sec = 
      fmt_params->rsvd_sector_cnt
      + (fmt_params-> fat_num*fmt_params->sectors_per_fat); 
    fmt_params->root_dir_fmt_sec_cnt = fmt_params->root_dir_sectors;
  }
  else {
    /*
     * for FAT32: root directory is in cluster 2
     */
    fmt_params->root_dir_start_sec = 
      fmt_params->rsvd_sector_cnt
      + (fmt_params-> fat_num*fmt_params->sectors_per_fat); 
    fmt_params->root_dir_fmt_sec_cnt = fmt_params->sectors_per_cluster;
  }
  /*
   * determine usable OEMName
   */
  if (ret_val == 0) {
      const char *from;
      char        *to = fmt_params->OEMName;
      int          cnt;
      from = "RTEMS"; /* default: make "from" point to OS Name */
    if ((rqdata != NULL) &&
	(rqdata->OEMName != NULL)) {
      from = rqdata->OEMName;
    }
    for (cnt = 0;
	 cnt < (sizeof(fmt_params->OEMName)-1);
	 cnt++) {
      if (isprint(*from)) {
	*to++ = *from++;
      }
      else {
	/*
	 * non-printable character in given name, so keep stuck 
	 * at that character and replace all following characters 
	 * with a ' '
	 */
	*to++=' ';
      }
      *to = '\0';
    }
  }

  /*
   * determine usable Volume Label
   */
  if (ret_val == 0) {
      const char *from;
      char        *to = fmt_params->VolLabel;
      int          cnt;
      from = ""; /* default: make "from" point to empty string */
    if ((rqdata != NULL) &&
	(rqdata->VolLabel != NULL)) {
      from = rqdata->VolLabel;
      fmt_params->VolLabel_present = true;
    }
    for (cnt = 0;
	 cnt < (sizeof(fmt_params->VolLabel)-1);
	 cnt++) {
      if (isprint(*from)) {
	*to++ = *from++;
      }
      else {
	/*
	 * non-printable character in given name, so keep stuck 
	 * at that character and replace all following characters 
	 * with a ' '
	 */
	*to++=' ';
      }
      *to = '\0';
    }
  }
      
  /*
   * determine usable Volume ID
   */
  if (ret_val == 0) {
    msdos_format_gen_volid(&(fmt_params->vol_id));
  }
  /*
   * Phuuu.... That's it.
   */
  if (ret_val != 0) {
    rtems_set_errno_and_return_minus_one(ret_val);
  }
  else {
    return 0;
  }
}

/** @brief Generate Master Boot Record.
 *
 * Create master boot record content from parameter set.
 * 
 * @param[out] mbr        Sector buffer.
 * @param[in]  fmt_params Pointer to format parameters.
 *
 * @return 0: OK.
 * @return -1: Error occured. Errno is set to real error value.
 */
static int msdos_format_gen_mbr
(
 char mbr[],                            /* OUT */
 const msdos_format_param_t *fmt_params /* IN */
 )
{
  uint32_t  total_sectors_num16 = 0;
  uint32_t  total_sectors_num32 = 0;

  /* store total sector count in either 16 or 32 bit field in mbr */
  if (fmt_params->totl_sector_cnt < 0x10000) {
    total_sectors_num16 = fmt_params->totl_sector_cnt;
  }
  else {
    total_sectors_num32 = fmt_params->totl_sector_cnt;
  }
  /*
   * finally we are there: let's fill in the values into the MBR 
   */
  memset(mbr,0,FAT_TOTAL_MBR_SIZE);
  /*
   * FIXME: fill jmpBoot and Boot code...
   * with 0xEB,....
   */
  /*
   * fill OEMName 
   */
  memcpy(FAT_GET_ADDR_BR_OEMNAME(mbr),
	 fmt_params->OEMName,
	 FAT_BR_OEMNAME_SIZE);
  FAT_SET_BR_BYTES_PER_SECTOR(mbr    , fmt_params->bytes_per_sector);
  FAT_SET_BR_SECTORS_PER_CLUSTER(mbr , fmt_params->sectors_per_cluster);
  FAT_SET_BR_RESERVED_SECTORS_NUM(mbr, fmt_params->rsvd_sector_cnt);
  
  /* number of FATs on medium */
  FAT_SET_BR_FAT_NUM(mbr             , 2); /* standard/recommended value */
  FAT_SET_BR_FILES_PER_ROOT_DIR(mbr  , fmt_params->files_per_root_dir);
  FAT_SET_BR_TOTAL_SECTORS_NUM16(mbr , total_sectors_num16);
  FAT_SET_BR_MEDIA(mbr               , fmt_params->media_code);

  FAT_SET_BR_SECTORS_PER_TRACK(mbr   , 0); /* only needed for INT13... */
  FAT_SET_BR_NUMBER_OF_HEADS(mbr     , 0); /* only needed for INT13... */
  FAT_SET_BR_HIDDEN_SECTORS(mbr      , 0); /* only needed for INT13... */

  FAT_SET_BR_TOTAL_SECTORS_NUM32(mbr , total_sectors_num32);
  if (fmt_params->fattype != FAT_FAT32) {
    FAT_SET_BR_SECTORS_PER_FAT(mbr   ,fmt_params->sectors_per_fat);
    FAT_SET_BR_DRVNUM(mbr            , 0); /* only needed for INT13... */
    FAT_SET_BR_RSVD1(mbr             , 0); /* fill with zero */
    FAT_SET_BR_BOOTSIG(mbr           , FAT_BR_BOOTSIG_VAL); 
    FAT_SET_BR_VOLID(mbr             , fmt_params->vol_id); /* volume id */
  memcpy(FAT_GET_ADDR_BR_VOLLAB(mbr),
	 fmt_params->VolLabel,
	 FAT_BR_VOLLAB_SIZE);
    memcpy(FAT_GET_ADDR_BR_FILSYSTYPE(mbr),
	   (fmt_params->fattype == FAT_FAT12) 
	   ? "FAT12   "
	   : "FAT16   ",
	   FAT_BR_FILSYSTYPE_SIZE);
  }
  else {
    FAT_SET_BR_SECTORS_PER_FAT32(mbr   ,fmt_params->sectors_per_fat);
    FAT_SET_BR_EXT_FLAGS(mbr           , 0);
    FAT_SET_BR_FSVER(mbr               , 0); /* FAT32 Version:0.0 */
    FAT_SET_BR_FAT32_ROOT_CLUSTER(mbr  , 2); /* put root dir to cluster 2 */
    FAT_SET_BR_FAT32_FS_INFO_SECTOR(mbr, 1); /* Put fsinfo  to rsrvd sec 1*/
    FAT_SET_BR_FAT32_BK_BOOT_SECTOR(mbr, fmt_params->mbr_copy_sec ); /* Put MBR copy to rsrvd sec */
    memset(FAT_GET_ADDR_BR_FAT32_RESERVED(mbr),0,FAT_BR_FAT32_RESERVED_SIZE);

    FAT_SET_BR_FAT32_DRVNUM(mbr      , 0); /* only needed for INT13... */
    FAT_SET_BR_FAT32_RSVD1(mbr       , 0); /* fill with zero */
    FAT_SET_BR_FAT32_BOOTSIG(mbr     ,FAT_BR_FAT32_BOOTSIG_VAL); 
    FAT_SET_BR_FAT32_VOLID(mbr       , 0); /* not set */
    memset(FAT_GET_ADDR_BR_FAT32_VOLLAB(mbr)   ,0,FAT_BR_VOLLAB_SIZE);
    memcpy(FAT_GET_ADDR_BR_FAT32_FILSYSTYPE(mbr),
	   "FAT32   ",
	   FAT_BR_FILSYSTYPE_SIZE);
  }
  /*
   * add boot record signature
   */
  FAT_SET_BR_SIGNATURE(mbr,      FAT_BR_SIGNATURE_VAL);

  /*
   * add jump to boot loader at start of sector
   */
  FAT_SET_VAL8(mbr,0,0xeb);
  FAT_SET_VAL8(mbr,1,0x3c);
  FAT_SET_VAL8(mbr,2,0x90);
  /*
   * FIXME: a nice little PC boot loader would be nice here.
   * but where can I get one for free? 
   */
  /*
   * Phuuu.... That's it.
   */
  return 0;
}

/** @brief Generate FS Information Sector.
 *
 * Function to create FS Information Sector (FAT32 specific).
 *
 * @param[out] fsinfo Sector buffer.
 *
 * @return 0: OK.
 * @return -1: Error is occured. Errno is set to real error value.
 */
static int msdos_format_gen_fsinfo
(
 char fsinfo[]                         /* OUT */
 )
{

  /*
   * clear fsinfo sector data 
   */
  memset(fsinfo,0,FAT_TOTAL_FSINFO_SIZE);
  /*
   * write LEADSIG, STRUCTSIG, TRAILSIG
   */
  FAT_SET_FSINFO_LEAD_SIGNATURE (fsinfo,FAT_FSINFO_LEAD_SIGNATURE_VALUE );
  FAT_SET_FSINFO_STRUC_SIGNATURE(fsinfo,FAT_FSINFO_STRUC_SIGNATURE_VALUE);
  FAT_SET_FSINFO_TRAIL_SIGNATURE(fsinfo,FAT_FSINFO_TRAIL_SIGNATURE_VALUE);
/* 
 * write "empty" values for free cluster count and next cluster number 
 */
  FAT_SET_FSINFO_FREE_CLUSTER_COUNT(fsinfo+FAT_FSI_INFO,
				    0xffffffff);
  FAT_SET_FSINFO_NEXT_FREE_CLUSTER (fsinfo+FAT_FSI_INFO,
				    0xffffffff);
  return 0;
}

int msdos_format
(
 const char *devname,                        /* IN */
 const msdos_format_request_param_t *rqdata  /* IN */
 )
{
  char                 tmp_sec[FAT_TOTAL_MBR_SIZE];
  int                  rc;
  rtems_disk_device   *dd        = NULL; 
  struct stat          stat_buf;
  int                  ret_val   = 0;
  int                  fd        = -1;
  int                  i;
  msdos_format_param_t fmt_params;

  /*
   * sanity check on device
   */
  printk("..%s(). sanity check on device\n", __FUNCTION__);
  if (ret_val == 0) {
    rc = stat(devname, &stat_buf);
    ret_val = rc;
  }
  
  /* rtems feature: no block devices, all are character devices */ 
  printk("..%s(). rtems feature: no block devices, "
         "all are character devices\n", __FUNCTION__);
  if ((ret_val == 0) &&
      (!S_ISCHR(stat_buf.st_mode))) {
    errno = ENOTBLK;
    ret_val = -1;
  }
  
  /* check that  device is registered as block device and lock it */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if (ret_val == 0) {
    dd = rtems_disk_obtain(stat_buf.st_dev);
    if (dd == NULL) {
      errno = ENOTBLK;
      ret_val = -1;
    }
  }

  /*
   * open device for writing
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if (ret_val == 0) {
    fd = open(devname, O_WRONLY);
    if (fd == -1)
    {
      ret_val= -1;
    }    
  }  

  /*
   * compute formatting parameters
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if (ret_val == 0) {
    ret_val = msdos_format_determine_fmt_params(dd,rqdata,&fmt_params);
  }
  /*
   * if requested, write whole disk/partition with 0xe5
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if ((ret_val == 0) &&
      (rqdata != NULL) &&
      !(rqdata->quick_format)) {
    ret_val = msdos_format_fill_sectors
      (fd,
       0,                            /* start sector */
       fmt_params.totl_sector_cnt,   /* sector count */
       fmt_params.bytes_per_sector,
       0xe5);
  }
  /*
   * create master boot record 
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if (ret_val == 0) {
    ret_val = msdos_format_gen_mbr(tmp_sec,&fmt_params);
  }
  /*
   * write master boot record to disk
   * also write copy of MBR to disk
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if (ret_val == 0) {
    ret_val = msdos_format_write_sec(fd, 
				     0, 
				     fmt_params.bytes_per_sector,
				     tmp_sec);
  }
  if ((ret_val == 0) && 
      (fmt_params.mbr_copy_sec != 0)) {
    /*
     * write copy of MBR
     */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
    ret_val = msdos_format_write_sec(fd, 
				     fmt_params.mbr_copy_sec , 
				     fmt_params.bytes_per_sector,
				     tmp_sec);
  }
  /*
   * for FAT32: initialize info sector on disk
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if ((ret_val == 0) && 
      (fmt_params.fsinfo_sec != 0)) {
      ret_val = msdos_format_gen_fsinfo(tmp_sec);
  }
  /*
   * write fsinfo sector
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if ((ret_val == 0) &&
      (fmt_params.fsinfo_sec != 0)) {
    ret_val = msdos_format_write_sec(fd, 
				     fmt_params.fsinfo_sec,
				     fmt_params.bytes_per_sector,
				     tmp_sec);
  }
  /*
   * write FAT as all empty
   * -> write all FAT sectors as zero
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if (ret_val == 0) {
    ret_val = msdos_format_fill_sectors
      (fd,
       fmt_params.rsvd_sector_cnt,                   /* start sector */
       fmt_params.fat_num*fmt_params.sectors_per_fat,/* sector count */
       fmt_params.bytes_per_sector,
       0x00);
  }
  /*
   * clear/init root directory
   * -> write all directory sectors as 0x00
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if (ret_val == 0) {
    ret_val = msdos_format_fill_sectors
      (fd,
       fmt_params.root_dir_start_sec,        /* start sector */
       fmt_params.root_dir_fmt_sec_cnt,      /* sector count */
       fmt_params.bytes_per_sector,
       0x00);
  }
  /*
   * write volume label to first entry of directory
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if ((ret_val == 0) && fmt_params.VolLabel_present) {
    memset(tmp_sec,0,sizeof(tmp_sec));
    memcpy(MSDOS_DIR_NAME(tmp_sec),fmt_params.VolLabel,MSDOS_SHORT_NAME_LEN);
    *MSDOS_DIR_ATTR(tmp_sec) = MSDOS_ATTR_VOLUME_ID;
    ret_val = msdos_format_write_sec
      (fd,
       fmt_params.root_dir_start_sec,
       fmt_params.bytes_per_sector,
       tmp_sec);
  }
  /*
   * write FAT entry 0 as (0xffffff00|Media_type)EOC, 
   * write FAT entry 1 as EOC
   * allocate directory in a FAT32 FS
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if ((ret_val == 0) && fmt_params.VolLabel_present){
    /*
     * empty sector: all clusters are free/do not link further on
     */
    memset(tmp_sec,0,sizeof(tmp_sec));

    switch(fmt_params.fattype) {
    case FAT_FAT12:
      /* LSBits of FAT entry 0: media_type */
      FAT_SET_VAL8(tmp_sec,0,(fmt_params.media_code));
      /* MSBits of FAT entry 0:0xf, LSBits of FAT entry 1: LSB of EOC */
      FAT_SET_VAL8(tmp_sec,1,(0x0f | (FAT_FAT12_EOC << 4)));
      /* MSBits of FAT entry 1: MSBits of EOC */
      FAT_SET_VAL8(tmp_sec,2,(FAT_FAT12_EOC >> 4));
      break;

    case FAT_FAT16:
      /* FAT entry 0: 0xff00|media_type */
      FAT_SET_VAL16(tmp_sec,0,0xff00|fmt_params.media_code);
      /* FAT entry 1: EOC */
      FAT_SET_VAL16(tmp_sec,2,FAT_FAT16_EOC);
      break;

    case FAT_FAT32:
      /* FAT entry 0: 0xffffff00|media_type */
      FAT_SET_VAL32(tmp_sec,0,0xffffff00|fmt_params.media_code);
      /* FAT entry 1: EOC */
      FAT_SET_VAL32(tmp_sec,4,FAT_FAT32_EOC);
      break;

    default:
      ret_val = -1;
      errno = EINVAL;
    }
    if (fmt_params.fattype == FAT_FAT32) {
      /*
       * only first valid cluster (cluster number 2) belongs
       * to root directory, and is end of chain
       * mark this in every copy of the FAT
       */
      FAT_SET_VAL32(tmp_sec,8,FAT_FAT32_EOC);
    }
    for (i = 0;
	 (i < fmt_params.fat_num) && (ret_val == 0);
	 i++) {
      ret_val = msdos_format_write_sec
	(fd,
	 fmt_params.rsvd_sector_cnt
	 + (i * fmt_params.sectors_per_fat),
	 fmt_params.bytes_per_sector,
	 tmp_sec);
    }
  }
  /*
   * cleanup:
   * sync and unlock disk
   * free any data structures (not needed now)
   */
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if (fd != -1) {
    close(fd);
  }
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  if (dd != NULL) {
    rtems_disk_release(dd);
  printk("..%s(line:%d). \n", __FUNCTION__, __LINE__);
  }

  printk("..%s(). return %d \n", __FUNCTION__, ret_val);
  return ret_val;
}

#endif
