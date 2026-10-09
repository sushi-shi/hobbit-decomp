
/* pngerror.c - stub functions for i/o and memory allocation
 *
 * libpng 1.0.5m - January 7, 2000
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998, 1999, 2000 Glenn Randers-Pehrson
 *
 * This file provides a location for all error handling.  Users who
 * need special error handling are expected to write replacement functions
 * and use xpng_set_error_fn() to use those functions.  See the instructions
 * at each function.
 */

#define PNG_INTERNAL
#include <xCore/Auxiliary/Bitmap/PNG/png.h>

static void xpng_default_error PNGARG((xpng_structp xpng_ptr,
                                      xpng_const_charp message));
static void xpng_default_warning PNGARG((xpng_structp xpng_ptr,
                                        xpng_const_charp message));

/* This function is called whenever there is a fatal error.  This function
 * should not be changed.  If there is a need to handle errors differently,
 * you should supply a replacement error function and use xpng_set_error_fn()
 * to replace the error function at run-time.
 */
void
xpng_error(xpng_structp xpng_ptr, xpng_const_charp message)
{
   if (xpng_ptr->error_fn != NULL)
      (*(xpng_ptr->error_fn))(xpng_ptr, message);

   /* if the following returns or doesn't exist, use the default function,
      which will not return */
   xpng_default_error(xpng_ptr, message);
}

/* This function is called whenever there is a non-fatal error.  This function
 * should not be changed.  If there is a need to handle warnings differently,
 * you should supply a replacement warning function and use
 * xpng_set_error_fn() to replace the warning function at run-time.
 */
void
xpng_warning(xpng_structp xpng_ptr, xpng_const_charp message)
{
   if (xpng_ptr->warning_fn != NULL)
      (*(xpng_ptr->warning_fn))(xpng_ptr, message);
   else
      xpng_default_warning(xpng_ptr, message);
}

/* These utilities are used internally to build an error message that relates
 * to the current chunk.  The chunk name comes from xpng_ptr->chunk_name,
 * this is used to prefix the message.  The message is limited in length
 * to 63 bytes, the name characters are output as hex digits wrapped in []
 * if the character is invalid.
 */
#define isnonalpha(c) ((c) < 41 || (c) > 122 || ((c) > 90 && (c) < 97))
static PNG_CONST char xpng_digit[16] = {
   '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
};

static void
xpng_format_buffer(xpng_structp xpng_ptr, xpng_charp buffer, xpng_const_charp message)
{
   int iout = 0, iin = 0;

   while (iin < 4) {
      int c = xpng_ptr->chunk_name[iin++];
      if (isnonalpha(c)) {
         buffer[iout++] = '[';
         buffer[iout++] = xpng_digit[(c & 0xf0) >> 4];
         buffer[iout++] = xpng_digit[c & 0x0f];
         buffer[iout++] = ']';
      } else {
         buffer[iout++] = (xpng_byte)c;
      }
   }

   if (message == NULL)
      buffer[iout] = 0;
   else {
      buffer[iout++] = ':';
      buffer[iout++] = ' ';
      xpng_memcpy(buffer+iout, message, 64);
      buffer[iout+63] = 0;
   }
}

void
xpng_chunk_error(xpng_structp xpng_ptr, xpng_const_charp message)
{
   char msg[16+64];
   xpng_format_buffer(xpng_ptr, msg, message);
   xpng_error(xpng_ptr, msg);
}

void
xpng_chunk_warning(xpng_structp xpng_ptr, xpng_const_charp message)
{
   char msg[16+64];
   xpng_format_buffer(xpng_ptr, msg, message);
   xpng_warning(xpng_ptr, msg);
}

/* This is the default error handling function.  Note that replacements for
 * this function MUST NOT RETURN, or the program will likely crash.  This
 * function is used by default, or if the program supplies NULL for the
 * error function pointer in xpng_set_error_fn().
 */
static void
xpng_default_error(xpng_structp xpng_ptr, xpng_const_charp message)
{
#ifndef PNG_NO_CONSOLE_IO
   fprintf(stderr, "libpng error: %s\n", message);
#endif

#ifdef USE_FAR_KEYWORD
   {
      jmp_buf jmpbuf;
      xpng_memcpy(jmpbuf,xpng_ptr->jmpbuf,sizeof(jmp_buf));
      longjmp(jmpbuf, 1);
   }
#else
   longjmp(xpng_ptr->jmpbuf, 1);
#endif
}

/* This function is called when there is a warning, but the library thinks
 * it can continue anyway.  Replacement functions don't have to do anything
 * here if you don't want them to.  In the default configuration, xpng_ptr is
 * not used, but it is passed in case it may be useful.
 */
static void
xpng_default_warning(xpng_structp xpng_ptr, xpng_const_charp message)
{
#ifndef PNG_NO_CONSOLE_IO
   fprintf(stderr, "libpng warning: %s\n", message);
#endif
   if (xpng_ptr == NULL)
      return;
}

/* This function is called when the application wants to use another method
 * of handling errors and warnings.  Note that the error function MUST NOT
 * return to the calling routine or serious problems will occur.  The return
 * method used in the default routine calls longjmp(xpng_ptr->jmpbuf, 1)
 */
void
xpng_set_error_fn(xpng_structp xpng_ptr, xpng_voidp error_ptr,
   xpng_error_ptr error_fn, xpng_error_ptr warning_fn)
{
   xpng_ptr->error_ptr = error_ptr;
   xpng_ptr->error_fn = error_fn;
   xpng_ptr->warning_fn = warning_fn;
}


/* This function returns a pointer to the error_ptr associated with the user
 * functions.  The application should free any memory associated with this
 * pointer before xpng_write_destroy and xpng_read_destroy are called.
 */
xpng_voidp
xpng_get_error_ptr(xpng_structp xpng_ptr)
{
   return ((xpng_voidp)xpng_ptr->error_ptr);
}



