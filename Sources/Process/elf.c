/*
 * Process/elf.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-05-08
 *
 */

#include <elf.h>
#include <mm.h>

err_t elf_check_header(Elf32_Ehdr *header)
{
	if ( (header->e_ident[EI_MAG0] != ELFMAG0) ||
		(header->e_ident[EI_MAG1] != ELFMAG1) ||
		(header->e_ident[EI_MAG2] != ELFMAG2) ||
		(header->e_ident[EI_MAG3] != ELFMAG3))
			return ERROR_INVALID;
			
	if (header->e_ident[EI_CLASS] != ELFCLASS32)
		return ERROR_INVALID;
		
	if (header->e_ident[EI_DATA] != ELFDATA2LSB)
		return ERROR_INVALID;
		
	if (header->e_ident[EI_VERSION] != EV_CURRENT)
		return ERROR_INVALID;
		
	if (header->e_machine != EM_386)
		return ERROR_INVALID;
		
	if (header->e_version != EV_CURRENT)
		return ERROR_INVALID;
		
	if ((header->e_entry < MM_AREA_USER_CODEDATA_START) || (header->e_entry >= MM_AREA_USER_CODEDATA_END))
		return ERROR_INVALID;

	return 0;
}
