/*
 * This file is part of the Klesh operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 */


#ifndef KERNEL_TYPES_H
#define KERNEL_TYPES_H

typedef signed char		int8_t;
typedef signed short int	int16_t;
typedef signed long int		int32_t;
typedef signed long long int	int64_t;

typedef unsigned char		uint8_t;
typedef unsigned short int	uint16_t;
typedef unsigned long int	uint32_t;
typedef unsigned long long int	uint64_t;

typedef unsigned int		size_t;
typedef signed int		ssize_t;

typedef	void *			handle_t;

/* Misc macros */
#define min(a,b)	((a) < (b) ? (a) : (b))
#define max(a,b)	((a) > (b) ? (a) : (b))
#define NULL		((void *)0)

/* Error codes */
typedef unsigned int		err_t;

#define ERROR_NO_MEMORY		1	/* No free or available memory left */
#define ERROR_NOT_IMPLEMENTED	2	/* Function not implemented */
#define ERROR_USED		3	/* Used slot/entry */
#define ERROR_INVALID		4	/* Invalid argument supplied */
#define ERROR_OUT_OF_BOUNDS	5	/* Argument out of bounds */
#define ERROR_MALFORMED_URL	6	/* Malformed URL */
#define ERROR_NOT_FOUND		7	/* Element/file not found */
#define ERROR_TIMEOUT		8	/* Operation timed out */
#define ERROR_NOT_SUPPORTED	9	/* Data not supported */
#define ERROR_NOT_AVAILABLE	10	/* Resource not available for use */

#endif /* !defined KERNEL_TYPES_H */
