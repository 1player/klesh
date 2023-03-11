/*
 * string.c
 * Written by The Neuromancer <neuromancer at paranoici dot org>
 *
 * This file is part of the Nehemiah operating system.
 * Make sure you have read the license before copying, reading or
 * modifying this document.
 *
 * Initial release: 2004-11-24
 *
 */

#include <string.h>
#include <types.h>
#include <vararg.h>

int string_compare(unsigned char *str1, unsigned char *str2)
{
	while (*str1 && *str2) {
		if (*str1++ != *str2++)
			return -(*str1 - *str2);
	}

	if (*str1) {
		return -1;
	} else if (*str2) {
		return 1;
	} else {
		return 0;
	}
}



/* 
 * The best string format function ever written (tm) :p
 * Format characters taken from the Linux Programmer's Manual (man printf)
 */
 
#define alternate_form 	0x001
#define zero_padded 	0x002
#define right_just	0x004
#define positive_blank	0x008
#define show_positive	0x010
/* Those are internal flags */
#define sign		0x020	/* The number is signed. */
#define octal		0x040
#define hexadecimal	0x080
#define caps		0x100
#define long_long	0x200

/* I've done 2 version of do_integer: the normal and the long long version.
   This is because the long long version is very slow and every math operation is
   much slower due to compiler-emulated 64-bit operation support.
*/

static void do_integer_ll(unsigned char *result, unsigned long long num, int precision, int flags)
{
char *digits = "0123456789abcdef";
int base;

unsigned char tmp_buffer[32], *tmp = tmp_buffer;
unsigned char sign_char = 0;
size_t tmp_size = 0;
	
	/************************************************
	 * Parse the number and fill in the temp buffer *
	 ************************************************/
	
	if (flags & caps)
		digits = "0123456789ABCDEF";
	
	if (flags & octal)
		base = 8;
	else if (flags & hexadecimal)
		base = 16;
	else
		base = 10;
	
	/*
	 * Write the minus sign only if the user has specified a signed number (%d flag)
	 * and the number is _really_ signed
	 */
	if ((flags & sign) && (num & 0x8000000000000000ULL)) {
		num = ~num + 1;
		sign_char = '-';
	} else if (base == 10 && ( flags & (show_positive | positive_blank) )) {
		sign_char = (flags & show_positive) ? '+' : ' ';
	}
	
	do {
		*tmp++ = digits[num % base];
		num /= base;
		tmp_size++;
	} while (num);
	
	/************************************************
	 * Parse the precision and add the needed zeros *
	 ************************************************/

	precision -= tmp_size + (sign_char != 0);
	
	/* Output the sign character, if any */
	if (sign_char)
		*result++ = sign_char;
	
	/* Output the alternate forms */
	if (flags & alternate_form) {
		if ((flags & octal) && (num))
			*result++ = '0';
		else if (flags & hexadecimal) {
			*result++ = '0'; *result++ = 'x';
		}
	}
	
	/* Output the precision pads */
	if (precision > 0) {
		while (precision--)
			*result++ = '0';
	}
	
	/*
	 * Remember that the tmp buffer is reversed:
	 * if the number was 87, the result in the buffer is 78.
	 * Now we copy it on 'result' starting from the  buffer end.
	 */
	
	while (tmp_size--)
		*result++ = *--tmp;
	
	*result = 0;	
}

static void do_integer(unsigned char *result, unsigned long num, int precision, int flags)
{
char *digits = "0123456789abcdef";
int base;

unsigned char tmp_buffer[16], *tmp = tmp_buffer;
unsigned char sign_char = 0;
size_t tmp_size = 0;
	
	/************************************************
	 * Parse the number and fill in the temp buffer *
	 ************************************************/
	
	if (flags & caps)
		digits = "0123456789ABCDEF";
	
	if (flags & octal)
		base = 8;
	else if (flags & hexadecimal)
		base = 16;
	else
		base = 10;
	
	/*
	 * Write the minus sign only if the user has specified a signed number (%d flag)
	 * and the number is _really_ signed
	 */
	if ((flags & sign) && (num & 0x80000000UL)) {
		num = ~num + 1;
		sign_char = '-';
	} else if (base == 10 && ( flags & (show_positive | positive_blank) )) {
		sign_char = (flags & show_positive) ? '+' : ' ';
	}
	
	do {
		*tmp++ = digits[num % base];
		num /= base;
		tmp_size++;
	} while (num);
	
	/************************************************
	 * Parse the precision and add the needed zeros *
	 ************************************************/

	precision -= tmp_size + (sign_char != 0);
	
	/* Output the sign character, if any */
	if (sign_char)
		*result++ = sign_char;
	
	/* Output the alternate forms */
	if (flags & alternate_form) {
		if ((flags & octal) && (num))
			*result++ = '0';
		else if (flags & hexadecimal) {
			*result++ = '0'; *result++ = 'x';
		}
	}
	
	/* Output the precision pads */
	if (precision > 0) {
		while (precision--)
			*result++ = '0';
	}
	
	/*
	 * Remember that the tmp buffer is reversed:
	 * if the number was 87, the result in the buffer is 78.
	 * Now we copy it on 'result' starting from the  buffer end.
	 */
	
	while (tmp_size--)
		*result++ = *--tmp;
	
	*result = 0;	
}


size_t string_format_indirect(unsigned char *buffer, unsigned char *format, va_list arguments)
{
size_t count = 0;
	
int flags;
int field_width;
int precision;
unsigned char buf[32];			/* Internal buffer */
unsigned char *ptr;
	
	#define EMIT(c) { *buffer++ = (c); count++; }
	
	do
	{
		if (*format != '%')
			EMIT(*format)
		else {
			flags = 0;
			field_width = 0;
			
			format++;
			
			/* Parse flags or double % */
			
		parse_flags:
			
			switch (*format) {
			case '%':
				EMIT(*format)
				continue;
			
			case '#':
				flags |= alternate_form;
				break;
			
			case '0':
				flags |= zero_padded;
				break;
			
			case '-':
				flags |= right_just;
				flags &= ~zero_padded;
				break;
			
			case ' ':
				flags |= positive_blank;
				break;
			
			case '+':
				flags |= show_positive;
				break;
			default:
				goto parse_field;
			}
			
			format++;
			
			goto parse_flags;
			
		parse_field:
			
			/* Parse field width */
			
			if (*format > '0' && *format <= '9') {
				field_width *= 10;
				field_width += (int)(*format - '0');
				
				format++;
				goto parse_field;
			} else if (*format == '*') {
				
				/* The field width is an arguments. Read it */
				field_width = (int)va_arg(arguments, int);
				
				/* If the field width is < 0, then the right justify */
				if (field_width < 0) {
					flags |= right_just;
					field_width = -field_width;
				}
				
				format++;
			}
			
			/* Parse precision */
			
			if (*format == '.') {
				format++;
				
				precision = 0;
				
		parse_precision:
				
				if (*format > '0' && *format <= '9') {					
					precision *= 10;
					precision += (int)(*format - '0');
				
					format++;
					goto parse_precision;
				} else if (*format == '*') {
					precision = (int)va_arg(arguments, int);
					if (precision < 0)
						precision = 0;
				
					format++;
				}
			
			} else {
				precision = 1;
			}
			
			/* Parse length modifiers */
			
			/*
			 * We skip them because we will always use the biggest integer 
			 * format available
			 */
			
		parse_length_modifiers:
			
			switch (*format) {
			case 'h':
			case 'l':
				break;
			case 'L':
				flags |= long_long;
				break;
			default:
				goto parse_conversion;					
			}
			
			format++;
			goto parse_length_modifiers;
			
			
			/* Parse conversion specifiers */
			
		parse_conversion:
			
			switch (*format) {
			case 'd':
				flags |= sign;
				goto do_number;
			
			case 'o':
				flags |= octal;
				goto do_number;
			
			case 'X':
				flags |= caps | hexadecimal;
				goto do_number;
			
			case 'p':
				flags |= alternate_form;
			case 'x':
				flags |= hexadecimal;
				goto do_number;
			
			case 'u':
			do_number: 
				if (!precision)
					continue;

				if (flags & long_long)
					do_integer_ll(buf, va_arg(arguments, unsigned long long), precision, flags);
				else
					do_integer(buf, va_arg(arguments, unsigned long), precision, flags);

				ptr = buf;
			
				goto do_string;
			
			case 'c':
				EMIT((unsigned char)va_arg(arguments, unsigned char))
				break;
			
			case 's':
				flags &= ~zero_padded;
				ptr = (unsigned char *)va_arg(arguments, unsigned char *);
			
				if (!precision)
					continue;			
				else if (precision > 1)
					ptr[precision] = 0;
		
			do_string:
				field_width -= string_length(ptr);
			
				if ((flags & right_just) && (field_width > 0)) {
					while (field_width--)
						EMIT(' ')
				}
				
				while (*ptr)
					EMIT(*ptr++)
				
				if (field_width > 0) {
					char pad = flags & zero_padded ? '0' : ' ';
					while (field_width--)
						EMIT(pad)
				}
				
				break;
			
			case 'n':
				/* Store the characters written */
				*((size_t *)va_arg(arguments, size_t *)) = count;
				
				break;
			}
		}
			
			
	} while (*++format);
	
	return count;
}

size_t string_format_indirect_ex(void (*emit_func)(unsigned char c, void *private), void *private,
 				  unsigned char *format, va_list arguments)
{
size_t count = 0;
	
int flags;
int field_width;
int precision;
unsigned char buf[32];			/* Internal buffer */
unsigned char *ptr;
		
	do
	{
		if (*format != '%')
			emit_func(*format, private);
		else {
			flags = 0;
			field_width = 0;
			
			format++;
			
			/* Parse flags or double % */
			
		parse_flags:
			
			switch (*format) {
			case '%':
				emit_func(*format, private);
				continue;
			
			case '#':
				flags |= alternate_form;
				break;
			
			case '0':
				flags |= zero_padded;
				break;
			
			case '-':
				flags |= right_just;
				flags &= ~zero_padded;
				break;
			
			case ' ':
				flags |= positive_blank;
				break;
			
			case '+':
				flags |= show_positive;
				break;
			default:
				goto parse_field;
			}
			
			format++;
			
			goto parse_flags;
			
		parse_field:
			
			/* Parse field width */
			
			if (*format > '0' && *format <= '9') {
				field_width *= 10;
				field_width += (int)(*format - '0');
				
				format++;
				goto parse_field;
			} else if (*format == '*') {
				
				/* The field width is an arguments. Read it */
				field_width = (int)va_arg(arguments, int);
				
				/* If the field width is < 0, then the right justify */
				if (field_width < 0) {
					flags |= right_just;
					field_width = -field_width;
				}
				
				format++;
			}
			
			/* Parse precision */
			
			if (*format == '.') {
				format++;
				
				precision = 0;
				
		parse_precision:
				
				if (*format > '0' && *format <= '9') {					
					precision *= 10;
					precision += (int)(*format - '0');
				
					format++;
					goto parse_precision;
				} else if (*format == '*') {
					precision = (int)va_arg(arguments, int);
					if (precision < 0)
						precision = 0;
				
					format++;
				}
			
			} else {
				precision = 1;
			}
			
			/* Parse length modifiers */
			
			/*
			 * We skip them because we will always use the biggest integer 
			 * format available
			 */
			
		parse_length_modifiers:
			
			switch (*format) {
			case 'h':
			case 'l':
				break;
			case 'L':
				flags |= long_long;
				break;
			default:
				goto parse_conversion;					
			}
			
			format++;
			goto parse_length_modifiers;
			
			
			/* Parse conversion specifiers */
			
		parse_conversion:
			
			switch (*format) {
			case 'd':
				flags |= sign;
				goto do_number;
			
			case 'o':
				flags |= octal;
				goto do_number;
			
			case 'X':
				flags |= caps | hexadecimal;
				goto do_number;
			
			case 'p':
				flags |= alternate_form;
			case 'x':
				flags |= hexadecimal;
				goto do_number;
			
			case 'u':
			do_number: 
				if (!precision)
					continue;

				if (flags & long_long)
					do_integer_ll(buf, va_arg(arguments, unsigned long long), precision, flags);
				else
					do_integer(buf, va_arg(arguments, unsigned long), precision, flags);

				ptr = buf;
			
				goto do_string;
			
			case 'c':
				emit_func((unsigned char)va_arg(arguments, unsigned char), private);
				break;
			
			case 's':
				flags &= ~zero_padded;
				ptr = (unsigned char *)va_arg(arguments, unsigned char *);
			
				if (!precision)
					continue;			
				else if (precision > 1)
					ptr[precision] = 0;
		
			do_string:
				field_width -= string_length(ptr);
			
				if ((flags & right_just) && (field_width > 0)) {
					while (field_width--)
						emit_func(' ', private);
				}
				
				while (*ptr)
					emit_func(*ptr++, private);
				
				if (field_width > 0) {
					char pad = flags & zero_padded ? '0' : ' ';
					while (field_width--)
						emit_func(pad, private);
				}
				
				break;
			
			case 'n':
				/* Store the characters written */
				*((size_t *)va_arg(arguments, size_t *)) = count;
				
				break;
			}
		}
			
			
	} while (*++format);
	
	return count;
}

size_t string_format_ex(void (*emit_func)(unsigned char c, void *private), void *private, 
			unsigned char *format, ...)
{
va_list arguments;
size_t ret;
	
	va_start(arguments, format);
	
	ret = string_format_indirect_ex(emit_func, private, format, arguments);
	
	va_end(arguments);
	
	return ret;
}

size_t string_format(unsigned char *buffer, unsigned char *format, ...)
{
va_list arguments;
size_t ret;
	
	va_start(arguments, format);
	
	ret = string_format_indirect(buffer, format, arguments);
	
	va_end(arguments);
	
	return ret;
}
