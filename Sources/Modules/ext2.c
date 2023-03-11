/*
 * Drivers/ext2.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2005-04-09
 *
 */

#include <modules.h>
#include <mm.h>
#include <console.h>
#include <kernel.h>
#include <memory.h>

// Static data
static unsigned int sectors_per_block;
static unsigned int inodes_per_table;		// Inodes per inode table block
static unsigned char *block_buffer;

// Superblock
static struct __attribute__((packed)) ext2_superblock
{
	uint32_t inodes_count;
	uint32_t blocks_count;
	uint32_t r_blocks_count;
	uint32_t free_blocks_count;
	uint32_t free_inodes_count;
	uint32_t first_data_block; 	// usually 1 if blocksize == 1024, 0 otherwise
	uint32_t log_block_size; 	// 1024 << log_block_size == block size in bytes
	int32_t	 log_frag_size;  	// as above, but negatives allowed (shift right)
	uint32_t blocks_per_group;
	uint32_t frags_per_group;
	uint32_t inodes_per_group;
	uint32_t mtime; 			// last mount
	uint32_t wtime; 			// last write
	uint16_t mnt_count;
	uint16_t max_mnt_count;
	uint16_t magic;
	uint16_t state;
	uint16_t errors;
	uint16_t minor_rev_level;
	uint32_t lastcheck;
	uint32_t checkinterval;
	uint32_t creator_os;
	uint32_t rev_level;
	uint16_t def_resuid;
	uint16_t def_resgid;
	// rev_level > 0 only (otherwise, defaults assumed)
	uint32_t first_ino; 		// default == 11
	uint16_t inode_size; 		// default == 128
	uint16_t block_group_nr; 	// block group number this superblock is in
	uint32_t feature_compat;
	uint32_t feature_incompat;
	uint32_t feature_ro_compat;
	uint8_t  uuid[16]; 		// 128 bit volume ID
	uint8_t  volume_name[16];
	uint8_t  last_mounted[64]; 	// path last mounted at
	uint32_t algo_bitmap;
	// performance hints
	uint8_t  prealloc_blocks;
	uint8_t  prealloc_dir_blocks;
	uint16_t alignment;
	// ext3 stuff
	uint8_t  journal_uuid[16];
	uint32_t journal_inum;
	uint32_t journal_dev;
	uint32_t last_orphan;
	uint8_t  padding[788];
} *superblock;

// Block groups
struct __attribute__((packed)) ext2_block_group {
	uint32_t	bitmap_block;
	uint32_t	inode_bitmap_block;
	uint32_t	inode_table_block;
	uint16_t	free_blocks;
	uint16_t	free_inodes;
	uint16_t	directories_count;
	uint16_t	pad;
	uint32_t	reserved[3];
};

// Inodes
struct __attribute__((packed)) ext2_inode {
	uint16_t	mode;
	uint16_t	uid;
	uint32_t	size;
	uint32_t	atime;
	uint32_t	ctime;
	uint32_t	mtime;
	uint32_t	dtime;
	uint16_t	gid;
	uint16_t	links_count;
	uint32_t	block_count;
	uint32_t	flags;

	uint32_t	reserved1;

	uint32_t	block_pointer[12];
	uint32_t	indirect_block_pointer;
	uint32_t	double_indirect_block_pointer;
	uint32_t	triple_indirect_block_pointer;
	uint32_t	file_version;
	uint32_t	file_acl;
	uint32_t	directory_acl;
	uint32_t	fragment_addr;

	uint8_t		fragment_number;
	uint8_t		fragment_size;
	uint16_t	pad;
	uint32_t	reserved2[2];
};
#define EXT2_INODE_TYPE_DIRECTORY	040000

// Directory entry
struct __attribute__((packed)) ext2_directory_entry {
	uint32_t	inode;
	uint16_t	entry_length;
	uint8_t		name_length;
	uint8_t		file_type;
	char		name[];
};

static err_t ext2_block_read(unsigned int index)
{
static unsigned int last_block = 0xFFFFFFFF;

	// Simple optimization
	if (last_block == index)
		return 0;
		
	last_block = index;
		
	return fdc_read(block_buffer, index * sectors_per_block, sectors_per_block);
}

static err_t ext2_block_group_read(struct ext2_block_group **block_group, unsigned int group_index)
{
struct ext2_block_group *ret;
unsigned int block_index;

	// Calculate block
	block_index = (group_index * superblock->blocks_per_group) + superblock->first_data_block + 1;

	// Read and allocate a block group
	ext2_block_read(block_index);
	ret = (struct ext2_block_group *)mm_heap_allocate(sizeof(struct ext2_block_group));
	if (!ret)
		return ERROR_NO_MEMORY;
	memory_copy(ret, block_buffer, sizeof(struct ext2_block_group));

	*block_group = ret;
	
	return 0;
}

static err_t ext2_inode_read(struct ext2_inode *inode, unsigned int inode_index)
{
unsigned int group_index;		// Block group the inode belongs to
unsigned int group_inode_index;		// Index of the inode in the block group
unsigned int table_index;		// Index of the inode table block containing the inode
unsigned int table_inode_index;		// Index of the inode in the table block
struct ext2_block_group *block_group;

	// Calculate position of the inode in the file system
	group_index = inode_index / superblock->inodes_per_group;
	group_inode_index = inode_index % superblock->inodes_per_group - 1;
	table_index = group_inode_index / inodes_per_table;
	table_inode_index = group_inode_index % inodes_per_table;

	// Read block group
	ext2_block_group_read(&block_group, group_index);

	// Read the inode table block
	ext2_block_read(block_group->inode_table_block + table_index);
	mm_heap_free(block_group);

	// Copy the inode
	memory_copy(inode, &block_buffer[table_inode_index * sizeof(struct ext2_inode)], sizeof(struct ext2_inode));

	return 0;
}

static err_t ext2_inode_data_read(struct ext2_inode *inode, unsigned int data_block)
{
	if (data_block > 11) {
		console_write("Indirect, double indirect and triple indirect blocks are not supported in the Ext2 driver!\n");
		return ERROR_NOT_SUPPORTED;
	}

	ext2_block_read(inode->block_pointer[data_block]);

	return 0;
}

static err_t ext2_directory_lookup(struct ext2_inode *inode, unsigned char *file, unsigned int *file_inode)
{
struct ext2_directory_entry 	*entry;
unsigned int 			data_blocks, i;
unsigned char			name[256];
unsigned int			remaining_data;

	data_blocks = inode->block_count - 1;

	// Search in the data blocks
	for (i = 0; i < data_blocks; i++) {
		ext2_inode_data_read(inode, i);

		// Search into each entry
		remaining_data = sectors_per_block * 512;
		entry = (struct ext2_directory_entry *)block_buffer;

		while (remaining_data > sizeof(struct ext2_directory_entry)) {
			memory_copy(name, entry->name, entry->name_length);
			name[entry->name_length] = 0;

			if (string_compare(name, file) == 0) {
				*file_inode = entry->inode;
				return 0;
			}

			// Go to the next entry
			remaining_data -= entry->entry_length;
			entry = (struct ext2_directory_entry *)((unsigned)entry + entry->entry_length);
		}
	}
	
	return ERROR_NOT_FOUND;
}

err_t ext2_open(unsigned char *path, void **handle)
{
struct ext2_inode *cur_inode;
unsigned int inode_no = 2, last = 0;
unsigned char *ptr;

	// Sanity check on the path
	if (*path != '/')
		return ERROR_INVALID;
	path++;			// Skip the slash

	cur_inode = (struct ext2_inode *)mm_heap_allocate(sizeof(struct ext2_inode));
	if (!cur_inode)
		return ERROR_NO_MEMORY;

	while (1) {

		// Read the directory
		ext2_inode_read(cur_inode, inode_no);

		if (!(cur_inode->mode & EXT2_INODE_TYPE_DIRECTORY)) {
				console_write("Trying to access a file in something not a directory\n");
				return ERROR_INVALID;
		}

		ptr = path;
		while (*ptr && *ptr != '/')
			ptr++;

		if (*ptr)
			*ptr = 0;
		else
			last = 1;

		// Lookup the file in this directory
		if (ext2_directory_lookup(cur_inode, path, &inode_no) == ERROR_NOT_FOUND)
			return ERROR_NOT_FOUND;

		if (last)
			break;
		else
			path = ptr + 1;
	}

	// Read the file inode
	ext2_inode_read(cur_inode, inode_no);

	*handle = (void *)cur_inode;

	return 0;
}

err_t ext2_file_size(void *handle, unsigned int *size)
{
	*size = ((struct ext2_inode *)handle)->size;
	
	return 0;
}

err_t ext2_read(void *handle, unsigned char *buffer, size_t from, size_t count)
{
struct ext2_inode *inode = (struct ext2_inode *)handle;
unsigned int count_block, block_index, block_offset, block_size, length, i = 0;

	if ((from + count) > inode->size)
		return ERROR_INVALID;

	block_size = sectors_per_block * 512;
	count_block = count / block_size;
	
	do {
		// Initialize variables
		block_index = from / block_size;
		block_offset = from % block_size;
		length = count > block_size ? block_size : count;
		
		// Do actual read
		ext2_inode_data_read(inode, block_index);
		memory_copy(&buffer[i++ * block_size], &block_buffer[block_offset], length);
		
		// Next block
		from += length;		
		count -= block_size;
	} while (count_block--);
	
	return 0;
}

err_t ext2_close(void *handle)
{
	mm_heap_free(handle);

	return 0;
}

err_t ext2_init(void)
{
unsigned int i;
char name[256];

	//console_write("EXT2: Checking the filesystem...\n");
	
	superblock = (struct ext2_superblock *)mm_heap_allocate(sizeof(struct ext2_superblock));
	if (!superblock)
		return ERROR_NO_MEMORY;

	// Read the superblock
	fdc_read(superblock, 2, 2);

	// Check the magic value
	if (superblock->magic != 0xEF53) {
		console_write("The Ext2 boot partition is has an invalid version number or is corrupted!\n");
		return ERROR_NOT_SUPPORTED;
	}

	// Allocate the block buffer
	sectors_per_block = superblock->log_block_size + 2;
	if (sectors_per_block != 2) {
		console_write("Ext2 block sizes other than 1024 bytes are not supported!\n");
		return ERROR_NOT_SUPPORTED;
	}
	block_buffer = (unsigned char *)mm_heap_allocate(sectors_per_block * 512);
	if (!block_buffer)
		return ERROR_NO_MEMORY;

	// Calculate run-time data
	inodes_per_table = (sectors_per_block * 512) / sizeof(struct ext2_inode);

	return 0;
}
