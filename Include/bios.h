/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */

#ifndef KERNEL_BIOS_H
#define KERNEL_BIOS_H

#include <types.h>

/*
 * Structure: e820_map_entry
 *
 * Entry of the E820 memory map got from the Multiboot-compliant bootloader
 *
 * Fields:
 *	struct-size - Size of the entry in bytes
 *	base-addr - 64-bit start address of the area
 *	length - 64-bit length of the area
 *	type - Type of area (see below)
 *
 * Type of entries:
 *	typeAvailable - Available memory
 *	typeReserved - Reserved memory for I/O, etc. Not usable
 *	typeACPI - This area contains the ACPI tables. Will become available when tables have been read
 *	typeNVS - ?
 *	typeRemoved - Removed entry since has been coalesced with another one (do _not_ use)
 */

enum { typeAvailable = 1, typeReserved, typeACPI, typeNVS, typeRemoved = -1 };

struct e820_map_entry {
	uint32_t	struct_size;

	uint64_t	base_addr;
	uint64_t	length;

	uint32_t	type;

}__attribute__((packed));

#endif /* !defined KERNEL_BIOS_H */
