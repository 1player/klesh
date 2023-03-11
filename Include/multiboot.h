/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_MULTIBOOT_H
#define KERNEL_MULTIBOOT_H

#include <types.h>
#include <bios.h>

struct multiboot_module {
	uint32_t	module_start_addr;
	uint32_t	module_end_addr;

	unsigned char	*string;	/* ? */

	uint32_t	reserved;
}__attribute__((packed));

struct multiboot_drive {
	uint32_t	struct_size;
	
	uint8_t		drive_number;

	uint8_t		drive_mode;

	uint16_t	cylinders;
	uint8_t		heads;
	uint8_t		sectors_per_track;

	uint16_t	ports[];
}__attribute__((packed));

struct multiboot_apm {
	uint16_t	version;
	uint16_t	code_seg_32bit;
	uint16_t	code_off;
	uint16_t	code_seg_16bit;
	uint16_t	data_seg_16bit;
	uint16_t	flags;
	uint16_t	code_seg_32bit_len;
	uint16_t	code_seg_16bit_len;
	uint16_t	data_seg_16bit_len;
}__attribute__((packed));
	

struct multiboot_info {
	uint32_t		flags;

	/* present if bit 0 of flags is set */
	uint32_t		lower_memory_size;	/* in KB */
	uint32_t		upper_memory_size;	/* in KB */

	/* present if bit 1 of flags is set */
	uint8_t			boot_part3;
	uint8_t			boot_part2;
	uint8_t			boot_part1;
	uint8_t			boot_drive;

	/* present if bit 2 of flags is set */
	unsigned char		*command_line;

	/* present if bit 3 of flags is set */
	uint32_t		modules_count;
	struct multiboot_module	*module_entry;

	union {
		/* present if bit 4 of flags is set */
		struct {
			uint32_t	table_size;
			uint32_t	string_size;
			void		*addr;
			uint32_t	reserved;
		} aout;
	
		/* present if bit 5 of flags is set */
		struct {
			uint32_t	shdr_num;
			uint32_t	shdr_size;
			uint32_t	shdr_addr;
			uint32_t	shdr_shndx;
		} elf;			
	} symbols;

	/* present if bit 6 of flags is set */
	uint32_t		mmap_length;
	struct e820_map_entry	*mmap_entry;

	/* present if bit 7 of flags is set */
	uint32_t		drives_length;
	struct multiboot_drive	*drive_entry;

	/* present if bit 8 of flags is set */
	void			*rom_config_table;

	/* present if bit 9 of flags is set */
	unsigned char		*boot_loader_name;

	/* present if bit 10 of flags is set */
	struct multiboot_apm	*apm_table;

	/* present if bit 11 of flags is set */
	uint32_t		vbe_control_info;
	uint32_t		vbe_mode_info;
	uint16_t		vbe_mode;
	uint16_t		vbe_interface_seg;
	uint16_t		vbe_interface_off;
	uint32_t		vbe_interface_len;
}__attribute__((packed));

/* From start.asm */
extern struct multiboot_info *_multiboot;

#endif /* !defined KERNEL_MULTIBOOT_H */
