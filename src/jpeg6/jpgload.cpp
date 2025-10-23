
    
#include "jpeglib.h"
#include "jpeg6\jerror.h"
#include <memory.h>
#include <setjmp.h>

#if 1
#define INPUT_BUF_SIZE  4096

typedef struct {
	struct jpeg_source_mgr pub;	/* public fields */

	int src_size;
	JOCTET * src_buffer;

	JOCTET * buffer;		/* start of buffer */
	boolean start_of_file;	/* have we gotten any data yet? */
} my_source_mgr;
typedef my_source_mgr * my_src_ptr;

static void my_init_source(j_decompress_ptr cinfo) {
	my_src_ptr src = (my_src_ptr)cinfo->src;
	src->start_of_file = TRUE;
}

static boolean my_fill_input_buffer(j_decompress_ptr cinfo) {
	my_src_ptr src = (my_src_ptr)cinfo->src;
	size_t nbytes;

	if (src->src_size > INPUT_BUF_SIZE)
		nbytes = INPUT_BUF_SIZE;
	else
		nbytes = src->src_size;

	memcpy (src->buffer, src->src_buffer, nbytes);
	src->src_buffer += nbytes;
	src->src_size -= nbytes;

	if (nbytes <= 0) {
		if (src->start_of_file)	/* Treat empty input file as fatal error */
			ERREXIT(cinfo, JERR_INPUT_EMPTY);
		WARNMS(cinfo, JWRN_JPEG_EOF);
		/* Insert a fake EOI marker */
		src->buffer[0] = (JOCTET) 0xFF;
		src->buffer[1] = (JOCTET) JPEG_EOI;
		nbytes = 2;
	}

	src->pub.next_input_byte = src->buffer;
	src->pub.bytes_in_buffer = nbytes;
	src->start_of_file = FALSE;
	return TRUE;
}

static void my_skip_input_data(j_decompress_ptr cinfo, long num_bytes) {
	my_src_ptr src = (my_src_ptr)cinfo->src;
	if (num_bytes > 0) {
		while (num_bytes > (long) src->pub.bytes_in_buffer) {
			num_bytes -= (long) src->pub.bytes_in_buffer;
			(void) my_fill_input_buffer(cinfo);
		}
		src->pub.next_input_byte += (size_t) num_bytes;
		src->pub.bytes_in_buffer -= (size_t) num_bytes;
	}
}

static void my_term_source(j_decompress_ptr cinfo) {
}

static void jpeg_buffer_src(j_decompress_ptr cinfo, void* buffer, int bufsize) {
	my_src_ptr src;
	if (cinfo->src == NULL) {	/* first time for this JPEG object? */
		cinfo->src = (struct jpeg_source_mgr *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			sizeof (my_source_mgr));
		src = (my_src_ptr) cinfo->src;
		src->buffer = (JOCTET *)
			(*cinfo->mem->alloc_small) ((j_common_ptr) cinfo, JPOOL_PERMANENT,
			INPUT_BUF_SIZE * sizeof (JOCTET));
	}

	src = (my_src_ptr) cinfo->src;
	src->pub.init_source = my_init_source;
	src->pub.fill_input_buffer = my_fill_input_buffer;
	src->pub.skip_input_data = my_skip_input_data;
	src->pub.resync_to_restart = jpeg_resync_to_restart; /* use default method */
	src->pub.term_source = my_term_source;
	src->src_buffer = (JOCTET *)buffer;
	src->src_size = bufsize;
	src->pub.bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
	src->pub.next_input_byte = NULL; /* until buffer loaded */
}

static char errormsg[JMSG_LENGTH_MAX];
typedef struct my_jpeg_error_mgr {
	struct jpeg_error_mgr pub;  // "public" fields
	jmp_buf setjmp_buffer;      // for return to caller 
} bt_jpeg_error_mgr;

static void my_jpeg_error_exit (j_common_ptr cinfo) {
	my_jpeg_error_mgr* myerr = (bt_jpeg_error_mgr*) cinfo->err;
	(*cinfo->err->format_message) (cinfo, errormsg);
	longjmp (myerr->setjmp_buffer, 1);
}

static void j_putRGBScanline(unsigned char* jpegline, int widthPix, unsigned char* outBuf, int row) {
	int offset = row * widthPix * 4;
	int count;
	for (count = 0; count < widthPix; count++) {
		unsigned char iRed, iBlu, iGrn;
		unsigned char *oRed, *oBlu, *oGrn, *oAlp;

		iRed = *(jpegline + count * 3 + 0);
		iGrn = *(jpegline + count * 3 + 1);
		iBlu = *(jpegline + count * 3 + 2);

		oRed = outBuf + offset + count * 4 + 0;
		oGrn = outBuf + offset + count * 4 + 1;
		oBlu = outBuf + offset + count * 4 + 2;
		oAlp = outBuf + offset + count * 4 + 3;

		*oRed = iRed;
		*oGrn = iGrn;
		*oBlu = iBlu;
		*oAlp = 255;
	}
}

static void j_putRGBAScanline(unsigned char* jpegline, int widthPix, unsigned char* outBuf, int row) {
	int offset = row * widthPix * 4;
	int count;

	for (count = 0; count < widthPix; count++) {
		unsigned char iRed, iBlu, iGrn, iAlp;
		unsigned char *oRed, *oBlu, *oGrn, *oAlp;

		iRed = *(jpegline + count * 4 + 0);
		iGrn = *(jpegline + count * 4 + 1);
		iBlu = *(jpegline + count * 4 + 2);
		iAlp = *(jpegline + count * 4 + 3);

		oRed = outBuf + offset + count * 4 + 0;
		oGrn = outBuf + offset + count * 4 + 1;
		oBlu = outBuf + offset + count * 4 + 2;
		oAlp = outBuf + offset + count * 4 + 3;

		*oRed = iRed;
		*oGrn = iGrn;
		*oBlu = iBlu;

		//!\todo fix jpeglib, it leaves alpha channel uninitialised
#if 1
		*oAlp = 255;
#else
		*oAlp = iAlp;
#endif
	}
}

static void j_putGrayScanlineToRGB(unsigned char* jpegline, int widthPix, unsigned char* outBuf, int row) {
	int offset = row * widthPix * 4;
	int count;

	for (count = 0; count < widthPix; count++) {
		unsigned char iGray;
		unsigned char *oRed, *oBlu, *oGrn, *oAlp;

		// get our grayscale value
		iGray = *(jpegline + count);

		oRed = outBuf + offset + count * 4;
		oGrn = outBuf + offset + count * 4 + 1;
		oBlu = outBuf + offset + count * 4 + 2;
		oAlp = outBuf + offset + count * 4 + 3;

		*oRed = iGray;
		*oGrn = iGray;
		*oBlu = iGray;
		*oAlp = 255;
	}
}

extern void Sys_Printf(char *format, ...);

GLOBAL void LoadJPGBuff(unsigned char *fbuffer, unsigned char **pic, int *width, int *height, int size) {
	struct jpeg_decompress_struct cinfo;
	struct my_jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error (&jerr.pub);
	jerr.pub.error_exit = my_jpeg_error_exit;

	if (setjmp (jerr.setjmp_buffer)) //< TODO: use c++ exceptions instead of setjmp/longjmp to handle errors
	{
		Sys_Printf("WARNING: JPEG library error: %s\n", errormsg);
		jpeg_destroy_decompress (&cinfo);
		return;
	}

	jpeg_create_decompress (&cinfo);
	jpeg_buffer_src(&cinfo, (void *)fbuffer, size);
	jpeg_read_header (&cinfo, TRUE);
	jpeg_start_decompress (&cinfo);

	int row_stride = cinfo.output_width * cinfo.output_components;

	int nSize = cinfo.output_width * cinfo.output_height * 4;
	unsigned char *out = (unsigned char *)malloc(nSize + 1);

	JSAMPARRAY buffer = (*cinfo.mem->alloc_sarray) ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines (&cinfo, buffer, 1);

		if (cinfo.out_color_components == 4)
			j_putRGBAScanline(buffer[0], cinfo.output_width, out, cinfo.output_scanline - 1);
		else if (cinfo.out_color_components == 3)
			j_putRGBScanline(buffer[0], cinfo.output_width, out, cinfo.output_scanline - 1);
		else if (cinfo.out_color_components == 1)
			j_putGrayScanlineToRGB(buffer[0], cinfo.output_width, out, cinfo.output_scanline - 1);
	}

	jpeg_finish_decompress (&cinfo);
	jpeg_destroy_decompress (&cinfo);

	
	*pic = out;
	*width = cinfo.output_width;
	*height = cinfo.output_height;
}
#else
jmp_buf jmp_et;

void my_error_exit(j_common_ptr cinfo) {
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	// my_error_ptr myerr = (my_error_ptr)cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	// (*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(jmp_et, 1);
}

GLOBAL void LoadJPGBuff(unsigned char *fbuffer, unsigned char **pic, int *width, int *height, int size ) 
{
  /* This struct contains the JPEG decompression parameters and pointers to
   * working space (which is allocated as needed by the JPEG library).
   */

	
  struct jpeg_decompress_struct cinfo;
  /* We use our private extension JPEG error handler.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  /* This struct represents a JPEG error handler.  It is declared separately
   * because applications often want to supply a specialized error handler
   * (see the second half of this file for an example).  But here we just
   * take the easy way out and use the standard error handler, which will
   * print a message on stderr and call exit() if compression fails.
   * Note that this struct must live as long as the main JPEG parameter
   * struct, to avoid dangling-pointer problems.
   */
  
  
  /* More stuff */
  JSAMPARRAY buffer;		/* Output row buffer */
  int row_stride;		/* physical row width in output buffer */
  unsigned char *out;
  byte  *bbuf;
  int nSize;
  struct jpeg_error_mgr jerr;
  jerr.error_exit = my_error_exit;
  if (setjmp(jmp_et)) {
	  pic = nullptr;
	  return;
  }

  /* Step 1: allocate and initialize JPEG decompression object */

  /* We have to set up the error handler first, in case the initialization
   * step fails.  (Unlikely, but it could happen if you are out of memory.)
   * This routine fills in the contents of struct jerr, and returns jerr's
   * address which we place into the link field in cinfo.
   */
  cinfo.err = jpeg_std_error(&jerr);

  /* Now we can initialize the JPEG decompression object. */
  jpeg_create_decompress(&cinfo);

  /* Step 2: specify data source (eg, a file) */

  jpeg_stdio_src(&cinfo, fbuffer);

  /* Step 3: read file parameters with jpeg_read_header() */

  (void) jpeg_read_header(&cinfo, TRUE);
  /* We can ignore the return value from jpeg_read_header since
   *   (a) suspension is not possible with the stdio data source, and
   *   (b) we passed TRUE to reject a tables-only JPEG file as an error.
   * See libjpeg.doc for more info.
   */

  /* Step 4: set parameters for decompression */

  /* In this example, we don't need to change any of the defaults set by
   * jpeg_read_header(), so we do nothing here.
   */

  /* Step 5: Start decompressor */

  (void) jpeg_start_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* We may need to do some setup of our own at this point before reading
   * the data.  After jpeg_start_decompress() we have the correct scaled
   * output image dimensions available, as well as the output colormap
   * if we asked for color quantization.
   * In this example, we need to make an output work buffer of the right size.
   */ 
  /* JSAMPLEs per row in output buffer */
  row_stride = cinfo.output_width * cinfo.output_components;

  nSize = cinfo.output_width*cinfo.output_height*cinfo.output_components;
  out = reinterpret_cast<unsigned char*>(malloc(nSize+1));
  memset(out, 0, nSize+1);

  *pic = out;
  *width = cinfo.output_width;
  *height = cinfo.output_height;

  /* Step 6: while (scan lines remain to be read) */
  /*           jpeg_read_scanlines(...); */

  /* Here we use the library's state variable cinfo.output_scanline as the
   * loop counter, so that we don't have to keep track ourselves.
   */
  while (cinfo.output_scanline < cinfo.output_height) {
    /* jpeg_read_scanlines expects an array of pointers to scanlines.
     * Here the array is only one element long, but you could ask for
     * more than one scanline at a time if that's more convenient.
     */
	  bbuf = ((out+(row_stride*cinfo.output_scanline)));
  	buffer = &bbuf;
    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
  }

  // clear all the alphas to 255
  {
	  int	i, j;
		byte	*buf;

		buf = *pic;

	  j = cinfo.output_width * cinfo.output_height * 4;
	  for ( i = 3 ; i < j ; i+=4 ) {
		  buf[i] = 255;
	  }
  }

  /* Step 7: Finish decompression */

  (void) jpeg_finish_decompress(&cinfo);
  /* We can ignore the return value since suspension is not possible
   * with the stdio data source.
   */

  /* Step 8: Release JPEG decompression object */

  /* This is an important step since it will release a good deal of memory. */
  jpeg_destroy_decompress(&cinfo);

  /* After finish_decompress, we can close the input file.
   * Here we postpone it until after no more JPEG errors are possible,
   * so as to simplify the setjmp error logic above.  (Actually, I don't
   * think that jpeg_destroy can do an error exit, but why assume anything...)
   */
  //free (fbuffer);

  /* At this point you may want to check to see whether any corrupt-data
   * warnings occurred (test whether jerr.pub.num_warnings is nonzero).
   */

  /* And we're done! */
}

#endif