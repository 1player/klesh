OUTPUT_ARCH(i386)
OUTPUT_FORMAT(elf32-i386)

ENTRY(_nehemiah)
SECTIONS
{
	. = 0x100000;
	
	_start = .;
	
	.text :
	{ 
		_text_start = .;
		*(.text)
		_text_end = .;
	}

	.data :
	{
		_data_start = .;
		*(*data*)
		*(COMMON)
		_data_end = .;
	}

	.bss :
	{
		_bss_start = .;
		*(.bss)
		_bss_end = .;
	}

	. = ALIGN(4096);
	.page_aligned :
	{
		_page_aligned_start = .;
		*(.page_aligned)	
		_page_aligned_end = .;
	}
	
	.init :
	{
		_init_start = .;
		*(.init)	
		_init_end = .;
	}
	. = ALIGN(4096);

	_end = .;
}

