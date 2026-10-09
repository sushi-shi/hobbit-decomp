
/* pngset.c - storage of image information into info struct
 *
 * libpng 1.0.5m - January 7, 2000
 * For conditions of distribution and use, see copyright notice in png.h
 * Copyright (c) 1995, 1996 Guy Eric Schalnat, Group 42, Inc.
 * Copyright (c) 1996, 1997 Andreas Dilger
 * Copyright (c) 1998, 1999, 2000 Glenn Randers-Pehrson
 *
 * The functions here are used during reads to store data from the file
 * into the info struct, and during writes to store application data
 * into the info struct for writing into the file.  This abstracts the
 * info struct and allows us to change the structure in the future.
 */

#define PNG_INTERNAL
#include <xCore/Auxiliary/Bitmap/PNG/png.h>

#if defined(PNG_bKGD_SUPPORTED)
void
xpng_set_bKGD(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_color_16p background)
{
   xpng_debug1(1, "in %s storage function\n", "bKGD");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   xpng_memcpy(&(info_ptr->background), background, sizeof(xpng_color_16));
   info_ptr->valid |= PNG_INFO_bKGD;
}
#endif

#if defined(PNG_cHRM_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
void
xpng_set_cHRM(xpng_structp xpng_ptr, xpng_infop info_ptr,
   double white_x, double white_y, double red_x, double red_y,
   double green_x, double green_y, double blue_x, double blue_y)
{
   xpng_debug1(1, "in %s storage function\n", "cHRM");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->x_white = (float)white_x;
   info_ptr->y_white = (float)white_y;
   info_ptr->x_red   = (float)red_x;
   info_ptr->y_red   = (float)red_y;
   info_ptr->x_green = (float)green_x;
   info_ptr->y_green = (float)green_y;
   info_ptr->x_blue  = (float)blue_x;
   info_ptr->y_blue  = (float)blue_y;
#ifdef PNG_FIXED_POINT_SUPPORTED
   info_ptr->int_x_white = (xpng_fixed_point)(white_x*100000.+0.5);
   info_ptr->int_y_white = (xpng_fixed_point)(white_y*100000.+0.5);
   info_ptr->int_x_red   = (xpng_fixed_point)(red_x*100000.+0.5);
   info_ptr->int_y_red   = (xpng_fixed_point)(red_y*100000.+0.5);
   info_ptr->int_x_green = (xpng_fixed_point)(green_x*100000.+0.5);
   info_ptr->int_y_green = (xpng_fixed_point)(green_y*100000.+0.5);
   info_ptr->int_x_blue  = (xpng_fixed_point)(blue_x*100000.+0.5);
   info_ptr->int_y_blue  = (xpng_fixed_point)(blue_y*100000.+0.5);
#endif
   info_ptr->valid |= PNG_INFO_cHRM;
}
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
void
xpng_set_cHRM_fixed(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_fixed_point white_x, xpng_fixed_point white_y, xpng_fixed_point red_x,
   xpng_fixed_point red_y, xpng_fixed_point green_x, xpng_fixed_point green_y,
   xpng_fixed_point blue_x, xpng_fixed_point blue_y)
{
   xpng_debug1(1, "in %s storage function\n", "cHRM");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->int_x_white = white_x;
   info_ptr->int_y_white = white_y;
   info_ptr->int_x_red   = red_x;
   info_ptr->int_y_red   = red_y;
   info_ptr->int_x_green = green_x;
   info_ptr->int_y_green = green_y;
   info_ptr->int_x_blue  = blue_x;
   info_ptr->int_y_blue  = blue_y;
#ifdef PNG_FLOATING_POINT_SUPPORTED
   info_ptr->x_white = (float)(white_x/100000.);
   info_ptr->y_white = (float)(white_y/100000.);
   info_ptr->x_red   = (float)(red_x/100000.);
   info_ptr->y_red   = (float)(red_y/100000.);
   info_ptr->x_green = (float)(green_x/100000.);
   info_ptr->y_green = (float)(green_y/100000.);
   info_ptr->x_blue  = (float)(blue_x/100000.);
   info_ptr->y_blue  = (float)(blue_y/100000.);
#endif
   info_ptr->valid |= PNG_INFO_cHRM;
}
#endif
#endif

#if defined(PNG_gAMA_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
void
xpng_set_gAMA(xpng_structp xpng_ptr, xpng_infop info_ptr, double file_gamma)
{
   xpng_debug1(1, "in %s storage function\n", "gAMA");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->gamma = (float)file_gamma;
#ifdef PNG_FIXED_POINT_SUPPORTED
   info_ptr->int_gamma = (int)(file_gamma*100000.+.5);
#endif
   info_ptr->valid |= PNG_INFO_gAMA;
}
#endif
#endif
void
xpng_set_gAMA_fixed(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_fixed_point
   int_gamma)
{
   xpng_debug1(1, "in %s storage function\n", "gAMA");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

#ifdef PNG_FLOATING_POINT_SUPPORTED
   info_ptr->gamma = (float)(int_gamma/100000.);
#endif
   info_ptr->int_gamma = int_gamma;
   info_ptr->valid |= PNG_INFO_gAMA;
}

#if defined(PNG_hIST_SUPPORTED)
void
xpng_set_hIST(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_uint_16p hist)
{
   xpng_debug1(1, "in %s storage function\n", "hIST");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->hist = xpng_malloc(xpng_ptr, sizeof(xpng_uint_16) *
      info_ptr->num_palette);
   xpng_memcpy(info_ptr->hist, hist, sizeof(xpng_uint_16) * info_ptr->num_palette);
   info_ptr->valid |= PNG_INFO_hIST;
}
#endif

void
xpng_set_IHDR(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_uint_32 width, xpng_uint_32 height, int bit_depth,
   int color_type, int interlace_type, int compression_type,
   int filter_type)
{
   int rowbytes_per_pixel;
   xpng_debug1(1, "in %s storage function\n", "IHDR");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->width = width;
   info_ptr->height = height;
   info_ptr->bit_depth = (xpng_byte)bit_depth;
   info_ptr->color_type =(xpng_byte) color_type;
   info_ptr->compression_type = (xpng_byte)compression_type;
   info_ptr->filter_type = (xpng_byte)filter_type;
   info_ptr->interlace_type = (xpng_byte)interlace_type;
   if (info_ptr->color_type == PNG_COLOR_TYPE_PALETTE)
      info_ptr->channels = 1;
   else if (info_ptr->color_type & PNG_COLOR_MASK_COLOR)
      info_ptr->channels = 3;
   else
      info_ptr->channels = 1;
   if (info_ptr->color_type & PNG_COLOR_MASK_ALPHA)
      info_ptr->channels++;
   info_ptr->pixel_depth = (xpng_byte)(info_ptr->channels * info_ptr->bit_depth);

   /* check for overflow */
   rowbytes_per_pixel = (info_ptr->pixel_depth + 7) >> 3;
   if (( width > PNG_MAX_UINT/rowbytes_per_pixel))
   {
      xpng_warning(xpng_ptr,
         "Width too large to process image data; rowbytes will overflow.");
      info_ptr->rowbytes = (xpng_size_t)0;
   }
   else
      info_ptr->rowbytes = (info_ptr->width * info_ptr->pixel_depth + 7) >> 3;
}

#if defined(PNG_oFFs_SUPPORTED)
void
xpng_set_oFFs(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_int_32 offset_x, xpng_int_32 offset_y, int unit_type)
{
   xpng_debug1(1, "in %s storage function\n", "oFFs");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->x_offset = offset_x;
   info_ptr->y_offset = offset_y;
   info_ptr->offset_unit_type = (xpng_byte)unit_type;
   info_ptr->valid |= PNG_INFO_oFFs;
}
#endif

#if defined(PNG_pCAL_SUPPORTED)
void
xpng_set_pCAL(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_charp purpose, xpng_int_32 X0, xpng_int_32 X1, int type, int nparams,
   xpng_charp units, xpng_charpp params)
{
   xpng_uint_32 length;
   int i;

   xpng_debug1(1, "in %s storage function\n", "pCAL");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   length = xpng_strlen(purpose) + 1;
   xpng_debug1(3, "allocating purpose for info (%d bytes)\n", length);
   info_ptr->pcal_purpose = (xpng_charp)xpng_malloc(xpng_ptr, length);
   xpng_memcpy(info_ptr->pcal_purpose, purpose, (xpng_size_t)length);

   xpng_debug(3, "storing X0, X1, type, and nparams in info\n");
   info_ptr->pcal_X0 = X0;
   info_ptr->pcal_X1 = X1;
   info_ptr->pcal_type = (xpng_byte)type;
   info_ptr->pcal_nparams = (xpng_byte)nparams;

   length = xpng_strlen(units) + 1;
   xpng_debug1(3, "allocating units for info (%d bytes)\n", length);
   info_ptr->pcal_units = (xpng_charp)xpng_malloc(xpng_ptr, length);
   xpng_memcpy(info_ptr->pcal_units, units, (xpng_size_t)length);

   info_ptr->pcal_params = (xpng_charpp)xpng_malloc(xpng_ptr,
      (xpng_uint_32)((nparams + 1) * sizeof(xpng_charp)));
   info_ptr->pcal_params[nparams] = NULL;

   for (i = 0; i < nparams; i++)
   {
      length = xpng_strlen(params[i]) + 1;
      xpng_debug2(3, "allocating parameter %d for info (%d bytes)\n", i, length);
      info_ptr->pcal_params[i] = (xpng_charp)xpng_malloc(xpng_ptr, length);
      xpng_memcpy(info_ptr->pcal_params[i], params[i], (xpng_size_t)length);
   }

   info_ptr->valid |= PNG_INFO_pCAL;
}
#endif

#if defined(PNG_READ_sCAL_SUPPORTED) || defined(PNG_WRITE_sCAL_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
void
xpng_set_sCAL(xpng_structp xpng_ptr, xpng_infop info_ptr,
             int unit, double width, double height)
{
   xpng_debug1(1, "in %s storage function\n", "sCAL");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->scal_unit = (xpng_byte)unit;
   info_ptr->scal_pixel_width = width;
   info_ptr->scal_pixel_height = height;

   info_ptr->valid |= PNG_INFO_sCAL;
}
#else
#ifdef PNG_FIXED_POINT_SUPPORTED
void
xpng_set_sCAL_s(xpng_structp xpng_ptr, xpng_infop info_ptr,
             int unit, xpng_charp swidth, xpng_charp sheight)
{
   xpng_uint_32 length;

   xpng_debug1(1, "in %s storage function\n", "sCAL");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->scal_unit = (xpng_byte)unit;

   length = xpng_strlen(swidth) + 1;
   xpng_debug1(3, "allocating unit for info (%d bytes)\n", length);
   info_ptr->scal_s_width = (xpng_charp)xpng_malloc(xpng_ptr, length);
   xpng_memcpy(info_ptr->scal_s_width, swidth, (xpng_size_t)length);

   length = xpng_strlen(sheight) + 1;
   xpng_debug1(3, "allocating unit for info (%d bytes)\n", length);
   info_ptr->scal_s_width = (xpng_charp)xpng_malloc(xpng_ptr, length);
   xpng_memcpy(info_ptr->scal_s_height, sheight, (xpng_size_t)length);

   info_ptr->valid |= PNG_INFO_sCAL;
}
#endif
#endif
#endif

#if defined(PNG_pHYs_SUPPORTED)
void
xpng_set_pHYs(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_uint_32 res_x, xpng_uint_32 res_y, int unit_type)
{
   xpng_debug1(1, "in %s storage function\n", "pHYs");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->x_pixels_per_unit = res_x;
   info_ptr->y_pixels_per_unit = res_y;
   info_ptr->phys_unit_type = (xpng_byte)unit_type;
   info_ptr->valid |= PNG_INFO_pHYs;
}
#endif

void
xpng_set_PLTE(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_colorp palette, int num_palette)
{
   xpng_debug1(1, "in %s storage function\n", "PLTE");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->palette = palette;
   info_ptr->num_palette = (xpng_uint_16)num_palette;
   info_ptr->valid |= PNG_INFO_PLTE;
}

#if defined(PNG_sBIT_SUPPORTED)
void
xpng_set_sBIT(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_color_8p sig_bit)
{
   xpng_debug1(1, "in %s storage function\n", "sBIT");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   xpng_memcpy(&(info_ptr->sig_bit), sig_bit, sizeof (xpng_color_8));
   info_ptr->valid |= PNG_INFO_sBIT;
}
#endif

#if defined(PNG_sRGB_SUPPORTED)
void
xpng_set_sRGB(xpng_structp xpng_ptr, xpng_infop info_ptr, int intent)
{
   xpng_debug1(1, "in %s storage function\n", "sRGB");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   info_ptr->srgb_intent = (xpng_byte)intent;
   info_ptr->valid |= PNG_INFO_sRGB;
}

void
xpng_set_sRGB_gAMA_and_cHRM(xpng_structp xpng_ptr, xpng_infop info_ptr,
   int intent)
{
#if defined(PNG_gAMA_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
   float file_gamma;
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   xpng_fixed_point int_file_gamma;
#endif
#endif
#if defined(PNG_cHRM_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
   float white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y;
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   xpng_fixed_point int_white_x, int_white_y, int_red_x, int_red_y, int_green_x,
      int_green_y, int_blue_x, int_blue_y;
#endif
#endif
   xpng_debug1(1, "in %s storage function\n", "sRGB_gAMA_and_cHRM");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   xpng_set_sRGB(xpng_ptr, info_ptr, intent);

#if defined(PNG_gAMA_SUPPORTED)
#ifdef PNG_FLOATING_POINT_SUPPORTED
   file_gamma = (float).45455;
   xpng_set_gAMA(xpng_ptr, info_ptr, file_gamma);
#endif
#ifdef PNG_FIXED_POINT_SUPPORTED
   int_file_gamma = 45455L;
   xpng_set_gAMA_fixed(xpng_ptr, info_ptr, int_file_gamma);
#endif
#endif

#if defined(PNG_cHRM_SUPPORTED)
#ifdef PNG_FIXED_POINT_SUPPORTED
   int_white_x = 31270L;
   int_white_y = 32900L;
   int_red_x   = 64000L;
   int_red_y   = 33000L;
   int_green_x = 30000L;
   int_green_y = 60000L;
   int_blue_x  = 15000L;
   int_blue_y  =  6000L;

   xpng_set_cHRM_fixed(xpng_ptr, info_ptr,
      int_white_x, int_white_y, int_red_x, int_red_y, int_green_x, int_green_y,
      int_blue_x, int_blue_y);
#endif
#ifdef PNG_FLOATING_POINT_SUPPORTED
   white_x = (float).3127;
   white_y = (float).3290;
   red_x   = (float).64;
   red_y   = (float).33;
   green_x = (float).30;
   green_y = (float).60;
   blue_x  = (float).15;
   blue_y  = (float).06;

   xpng_set_cHRM(xpng_ptr, info_ptr,
      white_x, white_y, red_x, red_y, green_x, green_y, blue_x, blue_y);
#endif
#endif
}
#endif


#if defined(PNG_iCCP_SUPPORTED)
void
xpng_set_iCCP(xpng_structp xpng_ptr, xpng_infop info_ptr,
             xpng_charp name, int compression_type,
             xpng_charp profile, xpng_uint_32 proflen)
{
   xpng_debug1(1, "in %s storage function\n", "iCCP");
   if (xpng_ptr == NULL || info_ptr == NULL || name == NULL || profile == NULL)
      return;

   info_ptr->iccp_name = xpng_malloc(xpng_ptr, xpng_strlen(name)+1);
   strcpy(info_ptr->iccp_name, name);
   info_ptr->iccp_profile = xpng_malloc(xpng_ptr, proflen);
   xpng_memcpy(info_ptr->iccp_profile, profile, (xpng_size_t)proflen);
   info_ptr->iccp_proflen = proflen;
   /* Compression is always zero but is here so the API and info structure
    * does not have to change * if we introduce multiple compression types */
   info_ptr->iccp_compression = (xpng_byte)compression_type;
   info_ptr->valid |= PNG_INFO_iCCP;
}
#endif

#if defined(PNG_TEXT_SUPPORTED)
void
xpng_set_text(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_textp text_ptr,
   int num_text)
{
   int i;

   xpng_debug1(1, "in %s storage function\n", (xpng_ptr->chunk_name[0] == '\0' ?
      "text" : (xpng_const_charp)xpng_ptr->chunk_name));

   if (xpng_ptr == NULL || info_ptr == NULL || num_text == 0)
      return;

   /* Make sure we have enough space in the "text" array in info_struct
    * to hold all of the incoming text_ptr objects.
    */
   if (info_ptr->num_text + num_text > info_ptr->max_text)
   {
      if (info_ptr->text != NULL)
      {
         xpng_textp old_text;
         int old_max;

         old_max = info_ptr->max_text;
         info_ptr->max_text = info_ptr->num_text + num_text + 8;
         old_text = info_ptr->text;
         info_ptr->text = (xpng_textp)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(info_ptr->max_text * sizeof (xpng_text)));
         xpng_memcpy(info_ptr->text, old_text, (xpng_size_t)(old_max *
            sizeof(xpng_text)));
         xpng_free(xpng_ptr, old_text);
      }
      else
      {
         info_ptr->max_text = num_text + 8;
         info_ptr->num_text = 0;
         info_ptr->text = (xpng_textp)xpng_malloc(xpng_ptr,
            (xpng_uint_32)(info_ptr->max_text * sizeof (xpng_text)));
      }
      xpng_debug1(3, "allocated %d entries for info_ptr->text\n",
         info_ptr->max_text);
   }

   for (i = 0; i < num_text; i++)
   {
      xpng_size_t text_length,key_len,lang_len,lang_key_len;
      xpng_textp textp = &(info_ptr->text[info_ptr->num_text]);

      if (text_ptr[i].key == (xpng_charp)NULL)
          continue;

      key_len = xpng_strlen(text_ptr[i].key);

      if(text_ptr[i].compression > 0)
      {
        /* set iTXt data */
        lang_len = xpng_strlen(text_ptr[i].lang);
        lang_key_len = xpng_strlen(text_ptr[i].lang_key);
      }
      else
      {
        lang_len = 0;
        lang_key_len = 0;
      }

      if (text_ptr[i].text[0] == '\0')
      {
         text_length = 0;
         if(text_ptr[i].compression > 0)
            textp->compression = PNG_ITXT_COMPRESSION_NONE;
         else
            textp->compression = PNG_TEXT_COMPRESSION_NONE;
      }
      else
      {
         text_length = xpng_strlen(text_ptr[i].text);
         textp->compression = text_ptr[i].compression;
      }

      textp->key = (xpng_charp)xpng_malloc(xpng_ptr,
         (xpng_uint_32)(key_len + lang_len + lang_key_len + text_length + 4));
      /* Caution: the calling program, not libpng, is responsible for
         freeing this, if libpng wasn't the caller. */
      xpng_debug2(2, "Allocated %d bytes at %x in xpng_set_text\n",
         key_len + lang_len + lang_key_len + text_length + 4, textp->key);

      xpng_memcpy(textp->key, text_ptr[i].key,
         (xpng_size_t)(key_len));
      *(textp->key+key_len) = '\0';
      if (text_ptr[i].compression > 0)
      {
         textp->lang=textp->key + key_len + 1;
         xpng_memcpy(textp->lang, text_ptr[i].lang, lang_len);
         *(textp->lang+lang_len) = '\0';
         textp->lang_key=textp->lang + lang_len + 1;
         xpng_memcpy(textp->lang_key, text_ptr[i].lang_key, lang_key_len);
         *(textp->lang_key+lang_key_len) = '\0';
         textp->text=textp->lang_key + lang_key_len + 1;
      }
      else
      {
         textp->lang=NULL;
         textp->lang_key=NULL;
         textp->text=textp->key + key_len + 1;
      }

      if(text_length)
      {
         xpng_memcpy(textp->text, text_ptr[i].text,
            (xpng_size_t)(text_length));
         *(textp->text+text_length) = '\0';
      }
      else
         textp->text--;

      if(textp->compression > 0)
      {
         textp->text_length = 0;
         textp->itxt_length = text_length;
      }
      else
      {
         textp->text_length = text_length;
         textp->itxt_length = 0;
      }

      info_ptr->text[info_ptr->num_text]= *textp;
      info_ptr->num_text++;
      xpng_debug1(3, "transferred text chunk %d\n", info_ptr->num_text);
   }
}
#endif

#if defined(PNG_tIME_SUPPORTED)
void
xpng_set_tIME(xpng_structp xpng_ptr, xpng_infop info_ptr, xpng_timep mod_time)
{
   xpng_debug1(1, "in %s storage function\n", "tIME");
   if (xpng_ptr == NULL || info_ptr == NULL ||
       (xpng_ptr->mode & PNG_WROTE_tIME))
      return;

   xpng_memcpy(&(info_ptr->mod_time), mod_time, sizeof (xpng_time));
   info_ptr->valid |= PNG_INFO_tIME;
}
#endif

#if defined(PNG_tRNS_SUPPORTED)
void
xpng_set_tRNS(xpng_structp xpng_ptr, xpng_infop info_ptr,
   xpng_bytep trans, int num_trans, xpng_color_16p trans_values)
{
   xpng_debug1(1, "in %s storage function\n", "tRNS");
   if (xpng_ptr == NULL || info_ptr == NULL)
      return;

   if (trans != NULL)
   {
      info_ptr->trans = xpng_malloc(xpng_ptr, num_trans);
      xpng_memcpy(info_ptr->trans, trans, num_trans);
   }

   if (trans_values != NULL)
   {
      xpng_memcpy(&(info_ptr->trans_values), trans_values,
         sizeof(xpng_color_16));
      if (num_trans == 0)
        num_trans = 1;
   }
   info_ptr->num_trans = (xpng_uint_16)num_trans;
   info_ptr->valid |= PNG_INFO_tRNS;
}
#endif

#if defined(PNG_sPLT_SUPPORTED)
void
xpng_set_spalettes(xpng_structp xpng_ptr,
             xpng_infop info_ptr, xpng_spalette_p entries, int nentries)
{
    xpng_spalette_p        np;
    int i;

    np = (xpng_spalette_p)xpng_malloc(xpng_ptr,
        (info_ptr->splt_palettes_num + nentries) * sizeof(xpng_spalette));

    xpng_memcpy(np, info_ptr->splt_palettes,
           info_ptr->splt_palettes_num * sizeof(xpng_spalette));
    xpng_free(xpng_ptr, info_ptr->splt_palettes);

    for (i = 0; i < nentries; i++)
    {
        xpng_spalette_p to = np + info_ptr->splt_palettes_num + i;
        xpng_spalette_p from = entries + i;

        to->name = (xpng_charp)xpng_malloc(xpng_ptr,
                                        xpng_strlen(from->name) + 1);
        xpng_strcpy(to->name, from->name);
        to->entries = (xpng_spalette_entryp)xpng_malloc(xpng_ptr,
                                 from->nentries * sizeof(xpng_spalette));
        xpng_memcpy(to->entries, from->entries,
               from->nentries * sizeof(xpng_spalette));
	to->nentries = from->nentries;
	to->depth = from->depth;
    }

    info_ptr->splt_palettes = np;
    info_ptr->splt_palettes_num += nentries;
    info_ptr->valid |= PNG_INFO_sPLT;
}
#endif /* PNG_sPLT_SUPPORTED */

#if defined(PNG_WRITE_UNKNOWN_CHUNKS_SUPPORTED)
void
xpng_set_unknown_chunks(xpng_structp xpng_ptr,
   xpng_infop info_ptr, xpng_unknown_chunkp unknowns, int num_unknowns)
{
    xpng_unknown_chunkp np;
    int i;

    if (xpng_ptr == NULL || info_ptr == NULL || num_unknowns == 0)
        return;

    np = (xpng_unknown_chunkp)xpng_malloc(xpng_ptr,
        (info_ptr->unknown_chunks_num + num_unknowns) *
        sizeof(xpng_unknown_chunk));

    xpng_memcpy(np, info_ptr->unknown_chunks,
           info_ptr->unknown_chunks_num * sizeof(xpng_unknown_chunk));
    xpng_free(xpng_ptr, info_ptr->unknown_chunks);

    for (i = 0; i < num_unknowns; i++)
    {
        xpng_unknown_chunkp to = np + info_ptr->unknown_chunks_num + i;
        xpng_unknown_chunkp from = unknowns + i;

        xpng_strcpy((xpng_charp)to->name, (xpng_charp)from->name);
        to->data = (xpng_bytep)xpng_malloc(xpng_ptr, from->size);
        xpng_memcpy(to->data, from->data, from->size);
        to->size = from->size;

        /* note our location in the read or write sequence */
        to->location = (xpng_byte)(xpng_ptr->mode & 0xff);
    }

    info_ptr->unknown_chunks = np;
    info_ptr->unknown_chunks_num += num_unknowns;
}
#endif

#if defined(PNG_READ_EMPTY_PLTE_SUPPORTED)
void
xpng_permit_empty_plte (xpng_structp xpng_ptr, int empty_plte_permitted)
{
   xpng_debug1(1, "in xpng_permit_empty_plte\n", "");
   if (xpng_ptr == NULL)
      return;
   xpng_ptr->empty_plte_permitted=(xpng_byte)empty_plte_permitted;
}
#endif

#if defined(PNG_UNKNOWN_CHUNKS_SUPPORTED)
void
xpng_set_keep_unknown_chunks(xpng_structp xpng_ptr, int keep, xpng_bytep
   chunk_list, int num_chunks)
{
    xpng_bytep new_list, p;
    int i, old_num_chunks;
    if (num_chunks == 0)
    {
      if(keep == HANDLE_CHUNK_ALWAYS || keep == HANDLE_CHUNK_IF_SAFE)
        xpng_ptr->flags |= PNG_FLAG_KEEP_UNKNOWN_CHUNKS;
      else
        xpng_ptr->flags &= ~PNG_FLAG_KEEP_UNKNOWN_CHUNKS;

      if(keep == HANDLE_CHUNK_ALWAYS)
        xpng_ptr->flags |= PNG_FLAG_KEEP_UNSAFE_CHUNKS;
      else
        xpng_ptr->flags &= ~PNG_FLAG_KEEP_UNSAFE_CHUNKS;
      return;
    }
    if (chunk_list == NULL)
      return;
    old_num_chunks=xpng_ptr->num_chunk_list;
    new_list=xpng_malloc(xpng_ptr,5*(num_chunks+old_num_chunks));
    if(xpng_ptr->chunk_list != (xpng_bytep)NULL)
    {
       xpng_memcpy(new_list, xpng_ptr->chunk_list, 5*old_num_chunks);
       xpng_free_chunk_list(xpng_ptr);
    }
    xpng_memcpy(new_list+5*old_num_chunks, chunk_list, 5*num_chunks);
    for (p=new_list+5*old_num_chunks+4, i=0; i<num_chunks; i++, p+=5)
       *p=(xpng_byte)keep;
    xpng_ptr->num_chunk_list=old_num_chunks+num_chunks;
    xpng_ptr->chunk_list=new_list;
}
#endif

#if defined(PNG_READ_USER_CHUNKS_SUPPORTED)
void
xpng_set_read_user_chunk_fn(xpng_structp xpng_ptr, xpng_voidp user_chunk_ptr,
   xpng_user_chunk_ptr read_user_chunk_fn)
{
   xpng_debug(1, "in xpng_set_read_user_chunk_fn\n");
   xpng_ptr->read_user_chunk_fn = read_user_chunk_fn;
   xpng_ptr->user_chunk_ptr = user_chunk_ptr;
}
#endif

