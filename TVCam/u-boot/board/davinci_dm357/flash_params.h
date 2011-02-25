/*
 *
 * Copyright (C) 2008 Texas Instruments.
 *
 * ----------------------------------------------------------------------------
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 * ----------------------------------------------------------------------------
 *
 */
#ifndef _FLASH_PARAMSH_
#define _FLASH_PARAMSH_
//
//Structs
//
typedef struct _PageInfo
{
    	ULONG	reserved;
    	BYTE  	BlockReserved;
    	BYTE  	BadBlockFlag;
    	USHORT	reserved2;
}PageInfo, *PPageInfo;

typedef struct
{
	ULONG ReturnValue;
	ULONG ReadAddress;
	ULONG WriteAddress;
	ULONG Size;
} Download_Parms, *PDownload_Parms;

#define NO_ERROR            0
#define CORRECTED_ERROR     1
#define ECC_ERROR           2
#define UNCORRECTED_ERROR   3


#define BIT0    0x00000001
#define BIT1    0x00000002
#define BIT2    0x00000004
#define BIT3    0x00000008
#define BIT4    0x00000010
#define BIT5    0x00000020
#define BIT6    0x00000040
#define BIT7    0x00000080
#define BIT8    0x00000100
#define BIT9    0x00000200
#define BIT10   0x00000400
#define BIT11   0x00000800
#define BIT12   0x00001000
#define BIT13   0x00002000
#define BIT14   0x00004000
#define BIT15   0x00008000
#define BIT16   0x00010000
#define BIT17   0x00020000
#define BIT18   0x00040000
#define BIT19   0x00080000
#define BIT20   0x00100000
#define BIT21   0x00200000
#define BIT22   0x00400000
#define BIT23   0x00800000
#define BIT24   0x01000000
#define BIT25   0x02000000
#define BIT26   0x04000000
#define BIT27   0x08000000
#define BIT28   0x10000000
#define BIT29   0x20000000
#define BIT30   0x40000000
#define BIT31   0x80000000



//  Status bit pattern
#define STATUS_READY                0x40
#define STATUS_ERROR                0x01
//
//NOR SUPPORT
//
// Flash ID Commands INTEL
#define INTEL_ID_CMD       ((Hwd)0x0090)     	// INTEL ID CMD
#define INTEL_MANF_ID      ((Hwd)0x0089)     	// INTEL Manf ID expected
#define INTEL_DEVICE_8T    ((Hwd)0x88F1)     	// INTEL 8Mb top device code
#define INTEL_DEVICE_8B    ((Hwd)0x88F2)     	// INTEL 8Mb bottom device code
#define INTEL_DEVICE_16T   ((Hwd)0x88F3)     	// INTEL 16Mb top device code
#define INTEL_DEVICE_16B   ((Hwd)0x88F4)     	// INTEL 16Mb bottom device code
#define INTELS_J3_DEVICE_32   ((Hwd)0x0016)     // INTEL Strata J3 32Mb device code
#define INTELS_J3_DEVICE_64   ((Hwd)0x0017)     // INTEL Strata J3 64Mb device code
#define INTELS_J3_DEVICE_128  ((Hwd)0x0018)     // INTEL Strata J3 128Mb device code
#define INTELS_K3_DEVICE_64   ((Hwd)0x8801)     // INTEL Strata K3 64Mb device code
#define INTELS_K3_DEVICE_128  ((Hwd)0x8802)    	// INTEL Strata K3 128Mb device code
#define INTELS_K3_DEVICE_256  ((Hwd)0x8803)     // INTEL Strata K3 256Mb device code
#define INTELS_W18_DEVICE_128T  ((Hwd)0x8876)   // INTEL Wirless Flash Top 128 Mb device code
#define INTELS_W18_DEVICE_128B  ((Hwd)0x8867)   // INTEL Wirless Flash Bottom 128 Mb device code
#define INTELS_L18_DEVICE_128T  ((Hwd)0x880C)   // INTEL Wirless Flash Top 128 Mb device code
#define INTELS_L18_DEVICE_128B  ((Hwd)0x880F)   // INTEL Wirless Flash Bottom 128 Mb device code
#define INTELS_L18_DEVICE_256T  ((Hwd)0x880D)   // INTEL Wirless Flash Top 256 Mb device code
#define INTELS_L18_DEVICE_256B  ((Hwd)0x8810)  	// INTEL Wirless Flash Bottom 256 Mb device code
#define INTELS_K18_DEVICE_256B  ((Hwd)0x8807)  	// INTEL Wirless Flash Bottom 256 Mb device code
#define AMD1_DEVICE_ID     ((Hwd)0x2253)   // AMD29DL323CB
#define AMD2_DEVICE_ID     ((Hwd)0x2249)   // AMD29LV160D
#define AMD3_DEVICE_ID1    ((Hwd)0x2212)   // AMD29LV256M
#define AMD3_DEVICE_ID2    ((Hwd)0x2201)   // AMD29LV256M
// Flash ID Commands FUJITSU (Programs like AMD)
#define FUJITSU_MANF_ID    ((Hwd)0x04)        // Fujitsu Manf ID expected
#define FUJITSU1_DEVICE_ID     ((Hwd)0x2253)  // MBM29DL323BD
//Micron Programs Like Intel or Micron
#define MICRON_MANF_ID      ((Hwd)0x002C)     	// MICRON Manf ID expected
#define MICRON_MT28F_DEVICE_128T ((Hwd)0x4492)	// MICRON Flash device Bottom 128 Mb
//Samsung Programs like AMD
#define SAMSUNG_MANF_ID      	((Hwd)0x00EC)     	//SAMSUNG Manf ID expected
#define SAMSUNG_K8S2815E_128T  	((Hwd) 0x22F8)  	//SAMSUNG NOR Flash device TOP 128 Mb
// Flash Erase Commands AMD and FUJITSU
// Flash ID Commands AMD
#define AMD_ID_CMD0        ((Hwd)0xAA)     // AMD ID CMD 0
#define AMD_CMD0_ADDR       0x555          // AMD CMD0 Offset
#define AMD_ID_CMD1        ((Hwd)0x55)     // AMD ID CMD 1
#define AMD_CMD1_ADDR	    0x2AA          // AMD CMD1 Offset
#define AMD_ID_CMD2        ((Hwd)0x90)     // AMD ID CMD 2
#define AMD_CMD2_ADDR	    0x555          // AMD CMD2 Offset
#define AMD_MANF_ID        ((Hwd)0x01)     // AMD Manf ID expected
#define AMD_DEVICE_ID_MULTI   ((Hwd)0x227E)// Indicates Multi-Address Device ID
#define AMD_DEVICE_ID_OFFSET 0x1
#define AMD_DEVICE_ID_OFFSET1 0x0E         // First Addr for Multi-Address ID
#define AMD_DEVICE_ID_OFFSET2 0x0F         // Second Addr for Multi-Address ID
#define AMD_DEVICE_RESET   ((Hwd)0x00F0)   // AMD Device Reset Command
#define AMD_ERASE_CMD0    ((Hwd)0xAA)
#define AMD_ERASE_CMD1    ((Hwd)0x55)
#define AMD_ERASE_CMD2    ((Hwd)0x80)
#define AMD_ERASE_CMD3    ((Hwd)0xAA)     	// AMD29LV017B Erase CMD 3
#define AMD_ERASE_CMD4    ((Hwd)0x55)     	// AMD29LV017B Erase CMD 4
#define AMD_ERASE_CMD5    ((Hwd)0x10)     	// AMD29LV017B Erase CMD 5
#define AMD_ERASE_DONE    ((Hwd)0xFFFF)     // AMD29LV017B Erase Done
#define AMD_ERASE_BLK_CMD0	((Hwd)0xAA)
#define AMD_ERASE_BLK_CMD1	((Hwd)0x55)
#define AMD_ERASE_BLK_CMD2	((Hwd)0x80)
#define AMD_ERASE_BLK_CMD3	((Hwd)0xAA)
#define AMD_ERASE_BLK_CMD4	((Hwd)0x55)
#define AMD_ERASE_BLK_CMD5	((Hwd)0x30)
#define AMD_PROG_CMD0    ((Hwd)0xAA)
#define AMD_PROG_CMD1    ((Hwd)0x55)
#define AMD_PROG_CMD2    ((Hwd)0xA0)
#define AMD2_ERASE_CMD0    ((Hwd)0x00AA)     // AMD29DL800B Erase CMD 0
#define AMD2_ERASE_CMD1    ((Hwd)0x0055)     // AMD29DL800B Erase CMD 1
#define AMD2_ERASE_CMD2    ((Hwd)0x0080)     // AMD29DL800B Erase CMD 2
#define AMD2_ERASE_CMD3    ((Hwd)0x00AA)     // AMD29DL800B Erase CMD 3
#define AMD2_ERASE_CMD4    ((Hwd)0x0055)     // AMD29DL800B Erase CMD 4
#define AMD2_ERASE_CMD5    ((Hwd)0x0030)     // AMD29DL800B Erase CMD 5
#define AMD2_ERASE_DONE    ((Hwd)0x00FF)     // AMD29DL800B Erase Done
#define AMD_WRT_BUF_LOAD_CMD0           ((Hwd)0xAA)
#define AMD_WRT_BUF_LOAD_CMD1           ((Hwd)0x55)
#define AMD_WRT_BUF_LOAD_CMD2           ((Hwd)0x25)
#define AMD_WRT_BUF_CONF_CMD0           ((Hwd)0x29)
#define AMD_WRT_BUF_ABORT_RESET_CMD0    ((Hwd)0xAA)
#define AMD_WRT_BUF_ABORT_RESET_CMD1    ((Hwd)0x55)
#define AMD_WRT_BUF_ABORT_RESET_CMD2    ((Hwd)0xF0)
// Flash Erase Commands INTEL
#define INTEL_ERASE_CMD0   ((Hwd)0x0020)     // INTEL Erase CMD 0
#define INTEL_ERASE_CMD1   ((Hwd)0x00D0)     // INTEL Erase CMD 1
#define INTEL_ERASE_DONE   ((Hwd)0x0080)     // INTEL Erase Done
#define INTEL_READ_MODE    ((Hwd)0x00FF)     // INTEL Read Array Mode
#define STRATA_READ        0x4
#define STRATA_WRITE       0x8
// Flash Block Information
// Intel Burst devices:
//   2MB each (8 8KB [param] and 31 64KB [main] blocks each) for 8MB total
#define NUM_INTEL_BURST_BLOCKS 8
#define PARAM_SET0  0
#define MAIN_SET0   1
#define PARAM_SET1  2
#define MAIN_SET1   3
#define PARAM_SET2  4
#define MAIN_SET2   5
#define PARAM_SET3  6
#define MAIN_SET3   7
// Intel Strata devices:
//   4MB each (32 128KB blocks each) for 8MB total
//   8MB each (64 128KB blocks each) for 16MB total
//  16MB each (128 128KB blocks each) for 32MB total
#define NUM_INTEL_STRATA_BLOCKS 8
#define BLOCK_SET0  0
#define BLOCK_SET1  1
#define BLOCK_SET2  2
#define BLOCK_SET3  3
#define BLOCK_SET4  4
#define BLOCK_SET5  5
#define BLOCK_SET6  6
#define BLOCK_SET7  7
// For AMD Flash
#define NUM_AMD_SECTORS 8  // Only using the first 8 8-KB sections (64 KB Total)
#define AMD_ADDRESS_CS_MASK		0xFE000000	//--AMD-- Set-up as 0xFE000000 per Jon Hunter (Ti)
// Flash Types
enum NORFlashType {
	FLASH_NOT_FOUND,
	FLASH_UNSUPPORTED,
	FLASH_AMD_LV017_2MB,             	// (AMD AM29LV017B-80RFC/RE)
	FLASH_AMD_DL800_1MB_BOTTOM,		  	// (AMD AM29DL800BB-70EC)
	FLASH_AMD_DL800_1MB_TOP,			// (AMD AM29DL800BT-70EC)
	FLASH_AMD_DL323_4MB_BOTTOM,		  	// (AMD AM29DL323CB-70EC)
	FLASH_AMD_DL323_4MB_TOP,			// (AMD AM29DL323BT-70EC)
	FLASH_AMD_LV160_2MB_BOTTOM,
	FLASH_AMD_LV160_2MB_TOP,
	FLASH_AMD_LV256M_32MB,             	// (AMD AM29LV256MH/L)
	FLASH_INTEL_BURST_8MB_BOTTOM,	   	// (Intel DT28F80F3B-95)
	FLASH_INTEL_BURST_8MB_TOP,		   	// (Intel DT28F80F3T-95)
	FLASH_INTEL_BURST_16MB_BOTTOM,	   	// (Intel DT28F160F3B-95)
	FLASH_INTEL_BURST_16MB_TOP,		   	// (Intel DT28F160F3T-95)
	FLASH_INTEL_STRATA_J3_4MB,		   	// (Intel DT28F320J3A)
	FLASH_INTEL_STRATA_J3_8MB,		   	// (Intel DT28F640J3A)
	FLASH_INTEL_STRATA_J3_16MB,		   	// (Intel DT28F128J3A)
	FLASH_FUJITSU_DL323_4MB_BOTTOM,    	// (Fujitsu DL323 Bottom
	FLASH_INTEL_STRATA_K3_8MB,		   	// (Intel 28F64K3C115)
	FLASH_INTEL_STRATA_K3_16MB,        	// (Intel 28F128K3C115)
	FLASH_INTEL_STRATA_K3_32MB,        	// (Intel 28F256K3C115)
	FLASH_INTEL_W18_16MB_TOP,    		// (Intel 28F128W18T) }
	FLASH_INTEL_W18_16MB_BOTTOM,  		// (Intel 28F128W18B) }
	FLASH_INTEL_L18_16MB_TOP,    		// (Intel 28F128L18T) }
	FLASH_INTEL_L18_16MB_BOTTOM,  		// (Intel 28F128L18B) }
	FLASH_INTEL_L18_32MB_TOP,    		// (Intel 28F256L18T) }
	FLASH_INTEL_L18_32MB_BOTTOM,  		// (Intel 28F256L18B) }
	FLASH_INTEL_K18_32MB_BOTTOM,  		// (Intel 28F256K18B) }
	FLASH_MICRON_16MB_TOP,				// (Micron MT28F160C34 )
	FLASH_SAMSUNG_16MB_TOP				// (Samsung K8S281ETA)
};
////NAND SUPPORT
//
enum NANDFlashType {
	NANDFLASH_NOT_FOUND,
	NANDFLASH_SAMSUNG_32x8_Q,             	// (Samsung K9F5608Q0B)
	NANDFLASH_SAMSUNG_32x8_U,             	// (Samsung K9F5608U0B)
	NANDFLASH_SAMSUNG_16x16_Q,             	// (Samsung K9F5616Q0B)
	NANDFLASH_SAMSUNG_16x16_U,             	// (Samsung K9F5616U0B)
	NANDFLASH_SAMSUNG_16x8_U				// (Samsung K9F1G08QOM)
};
// Samsung Manufacture Code
#define SAMSUNG_MANUFACT_ID	0xEC
// Samsung Nand Flash Device ID
#define SAMSUNG_K9F5608Q0B	0x35
#define SAMSUNG_K9F5608U0B	0x75
#define SAMSUNG_K9F5616Q0B	0x45
#define SAMSUNG_K9F5616U0B	0x55
//  MACROS for NAND Flash support
//  Flash Chip Capability
#define NUM_BLOCKS                  0x800       //  32 MB On-board NAND flash.
#define PAGE_SIZE                 	512
#define SPARE_SIZE                  16
#define PAGES_PER_BLOCK             32
#define PAGE_TO_BLOCK(page)     	((page) >> 5 )
#define BLOCK_TO_PAGE(block)      	((block)  << 5 )
#define FILE_TO_PAGE_SIZE(fs) 		((fs / PAGE_SIZE) + ((fs % PAGE_SIZE) ? 1 : 0))
//  For flash chip that is bigger than 32 MB, we need to have 4 step address
#ifdef NAND_SIZE_GT_32MB
#define NEED_EXT_ADDR               1
#else
#define NEED_EXT_ADDR               0
#endif
// Nand flash block status definitions.
#define BLOCK_STATUS_UNKNOWN	0x01
#define BLOCK_STATUS_BAD		0x02
#define BLOCK_STATUS_READONLY	0x04
#define BLOCK_STATUS_RESERVED   0x08
#define BLOCK_RESERVED			0x01
#define BLOCK_READONLY			0x02
#define BADBLOCKMARK            0x00
//  NAND Flash Command. This appears to be generic across all NAND flash chips
#define CMD_READ                0x00        //  Read
#define CMD_READ1               0x01        //  Read1
#define CMD_READ2               0x50        //  Read2
#define CMD_READID              0x90        //  ReadID
#define CMD_WRITE               0x80        //  Write phase 1
#define CMD_WRITE2              0x10        //  Write phase 2
#define CMD_ERASE               0x60        //  Erase phase 1
#define CMD_ERASE2              0xd0        //  Erase phase 2
#define CMD_STATUS              0x70        //  Status read
#define CMD_RESET               0xff        //  Reset
//
//Prototpyes
//
// NOR Flash Dependent Function Pointers
void (*User_Hard_Reset_Flash)(void);
void (*User_Soft_Reset_Flash)(unsigned long addr);
void (*User_Flash_Erase_Block)(unsigned long addr);
void (*User_Flash_Erase_All)(unsigned long addr);
void (*User_Flash_Write_Entry)(void);
int (*User_Flash_Write)(unsigned long *addr, unsigned short data);
int (*User_Flash_Optimized_Write)(unsigned long *addr, unsigned short data[], unsigned long);
void (*User_Flash_Write_Exit)(void);
// Flash AMD Device Dependent Routines
void AMD_Hard_Reset_Flash(void);
void AMD_Soft_Reset_Flash(unsigned long);
void AMD_Flash_Erase_Block(unsigned long);
void AMD_Flash_Erase_All(unsigned long);
int AMD_Flash_Write(unsigned long *, unsigned short);
int AMD_Flash_Optimized_Write(unsigned long *addr, unsigned short data[], unsigned long length);
void AMD_Write_Buf_Abort_Reset_Flash( unsigned long plAddress );
// Flash Intel Device Dependent Routines
void INTEL_Hard_Reset_Flash(void);
void INTEL_Soft_Reset_Flash(unsigned long addr);
void INTEL_Flash_Erase_Block(unsigned long);
int INTEL_Flash_Write(unsigned long *addr, unsigned short data);
int INTEL_Flash_Optimized_Write(unsigned long *addr, unsigned short data[], unsigned long length);

//General Functions
void Flash_Do_Nothing(void);

#endif



