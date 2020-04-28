#include "jpeg.h"

#include <stdio.h>
#include <memory.h>
#include <setjmp.h>

extern "C"
{
#include <jpeglib.h>
#include <jerror.h>
}

#include "assert.h"

#include "libloader.h"
KRL_BEGIN(LibJpeg, L"jpegd.dll", L"jpeg.dll")
KRL_IMPORT(jpeg_std_error)
KRL_IMPORT(jpeg_set_defaults)
KRL_IMPORT(jpeg_set_quality)
KRL_IMPORT(jpeg_start_compress)
KRL_IMPORT(jpeg_write_scanlines)
KRL_IMPORT(jpeg_finish_compress)
KRL_IMPORT(jpeg_destroy_compress)
KRL_IMPORT(jpeg_destroy_decompress)
KRL_IMPORT(jpeg_read_header)
KRL_IMPORT(jpeg_start_decompress)
KRL_IMPORT(jpeg_finish_decompress)
KRL_IMPORT(jpeg_CreateDecompress)
KRL_IMPORT(jpeg_CreateCompress)
KRL_IMPORT(jpeg_read_scanlines)
KRL_IMPORT(jpeg_resync_to_restart)
KRL_END()

using namespace kr;

struct my_error_mgr {
	struct jpeg_error_mgr pub;    /* "public" fields */

	jmp_buf setjmp_buffer;        /* for return to caller */
};
typedef struct my_error_mgr * my_error_ptr;

METHODDEF(void)
my_error_exit(j_common_ptr cinfo)
{
	/* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
	my_error_ptr myerr = (my_error_ptr)cinfo->err;

	/* Always display the message. */
	/* We could postpone this until after returning, if we chose. */
	(*cinfo->err->output_message) (cinfo);

	/* Return control to the setjmp point */
	longjmp(myerr->setjmp_buffer, 1);
}


namespace
{
	constexpr size_t BUFFERING_SIZE = 8192;

	struct kr_jpeg_source_mgr : jpeg_source_mgr {
		KrbFile* file;
		JOCTET buffer[BUFFERING_SIZE];

		static void make(j_decompress_ptr cinfo, KrbFile* in) noexcept
		{
			if (cinfo->src == NULL)
			{
				/* first time for this JPEG object? */
				cinfo->src = (struct jpeg_source_mgr*)
					(*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(kr_jpeg_source_mgr));
			}

			kr_jpeg_source_mgr* src = static_cast<kr_jpeg_source_mgr*> (cinfo->src);
			src->init_source = [](j_decompress_ptr cinfo) {
				kr_jpeg_source_mgr* src = (kr_jpeg_source_mgr*)(cinfo->src);
			};
			src->fill_input_buffer = [](j_decompress_ptr cinfo)->boolean{
				kr_jpeg_source_mgr* src = (kr_jpeg_source_mgr*)(cinfo->src);
				src->next_input_byte = src->buffer;
				src->bytes_in_buffer = src->file->read(src->buffer, BUFFERING_SIZE);
				return src->bytes_in_buffer != 0;
			};
			src->skip_input_data = [](j_decompress_ptr cinfo, long count)
			{
				kr_jpeg_source_mgr* src = (kr_jpeg_source_mgr*)(cinfo->src);
				if ((size_t)count <= src->bytes_in_buffer)
				{
					src->bytes_in_buffer -= count;
					src->next_input_byte += count;
				}
				else
				{
					src->bytes_in_buffer = 0;
					src->file->seek_cur(count - src->bytes_in_buffer);
				}
			};
			LibJpeg* libjpeg = LibJpeg::getInstance();
			if (libjpeg == nullptr) return;
			src->resync_to_restart = libjpeg->jpeg_resync_to_restart; /* use default method */
			src->term_source = [](j_decompress_ptr cinfo){};
			src->file = in;
			src->bytes_in_buffer = 0; /* forces fill_input_buffer on first read */
			src->next_input_byte = NULL; /* until buffer loaded */
		}
	};

	struct kr_jpeg_destination_mgr : jpeg_destination_mgr {
		KrbFile* file;
		JOCTET buffer[BUFFERING_SIZE];

		static void make(j_compress_ptr cinfo, KrbFile* in) noexcept
		{
			if (cinfo->dest == NULL)
			{
				/* first time for this JPEG object? */
				cinfo->dest = (struct jpeg_destination_mgr*)
					(*cinfo->mem->alloc_small) ((j_common_ptr)cinfo, JPOOL_PERMANENT, sizeof(kr_jpeg_destination_mgr));
			}

			kr_jpeg_destination_mgr* dest = static_cast<kr_jpeg_destination_mgr*> (cinfo->dest);
			dest->init_destination = [](j_compress_ptr cinfo) {
				kr_jpeg_destination_mgr* dest = (kr_jpeg_destination_mgr*)(cinfo->dest);
			};
			dest->empty_output_buffer = [](j_compress_ptr cinfo)->boolean {
				kr_jpeg_destination_mgr* dest = (kr_jpeg_destination_mgr*)(cinfo->dest);
				dest->file->write(dest->buffer, BUFFERING_SIZE);
				dest->next_output_byte = dest->buffer;
				dest->free_in_buffer = BUFFERING_SIZE;
				return true;
			};
			dest->term_destination = [](j_compress_ptr cinfo) {
				kr_jpeg_destination_mgr* dest = (kr_jpeg_destination_mgr*)(cinfo->dest);
				dest->file->write(dest->buffer, BUFFERING_SIZE);
				dest->next_output_byte = dest->buffer;
				dest->free_in_buffer = BUFFERING_SIZE;
			};
			dest->file = in;
		}
	};
}

bool kr::backend::Jpeg::save(const KrbImageSaveInfo* info, KrbFile* file) noexcept
{
	LibJpeg* libjpeg = LibJpeg::getInstance();
	if (libjpeg == nullptr) return false;
	assert(info->pixelformat == PixelFormatBGR8);

	/* This struct contains the JPEG compression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	* It is possible to have several such structures, representing multiple
	* compression/decompression processes, in existence at once.  We refer
	* to any one struct (and its associated working data) as a "JPEG object".
	*/
	struct jpeg_compress_struct cinfo;
	/* This struct represents a JPEG error handler.  It is declared separately
	* because applications often want to supply a specialized error handler
	* (see the second half of this file for an example).  But here we just
	* take the easy way out and use the standard error handler, which will
	* print a message on stderr and call exit() if compression fails.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct jpeg_error_mgr jerr;
	/* Step 1: allocate and initialize JPEG compression object */

	/* We have to set up the error handler first, in case the initialization
	* step fails.  (Unlikely, but it could happen if you are out of memory.)
	* This routine fills in the contents of struct jerr, and returns jerr's
	* address which we place into the link field in cinfo.
	*/
	cinfo.err = libjpeg->jpeg_std_error(&jerr);
	/* Now we can initialize the JPEG compression object. */
	libjpeg->jpeg_create_compress(&cinfo);
	kr_jpeg_destination_mgr::make(&cinfo, file);

	/* Step 2: specify data destination (eg, a file) */
	/* Note: steps 2 and 3 can be done in either order. */

	/* Here we use the library-supplied code to send compressed data to a
	* stdio stream.  You can also write your own code to do something else.
	* VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
	* requires it in order to write binary files.
	*/
	// if (!kr_fopen_write(&outfile, filename)) return false;
	// jpeg__dest
	// jpeg_stdio_dest(&cinfo, outfile);

	/* Step 3: set parameters for compression */

	/* First we supply a description of the input image.
	* Four fields of the cinfo struct must be filled in:
	*/
	cinfo.image_width = info->width;      /* image width and height, in pixels */
	cinfo.image_height = info->height;
	cinfo.input_components = 3;           /* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB;       /* colorspace of input image */
										  /* Now use the library's routine to set default compression parameters.
										  * (You must set at least cinfo.in_color_space before calling this,
										  * since the defaults depend on the source color space.)
										  */
	libjpeg->jpeg_set_defaults(&cinfo);
	/* Now you can set any non-default parameters you wish to.
	* Here we just illustrate the use of quality (quantization table) scaling:
	*/
	libjpeg->jpeg_set_quality(&cinfo, info->jpegQuality, TRUE /* limit to baseline-JPEG values */);

	/* Step 4: Start compressor */

	/* TRUE ensures that we will write a complete interchange-JPEG file.
	* Pass TRUE unless you are very sure of what you're doing.
	*/
	libjpeg->jpeg_start_compress(&cinfo, TRUE);

	/* Step 5: while (scan lines remain to be written) */
	/*           jpeg_write_scanlines(...); */

	/* Here we use the library's state variable cinfo.next_scanline as the
	* loop counter, so that we don't have to keep track ourselves.
	* To keep things simple, we pass one scanline per call; you can pass
	* more if you wish, though.
	*/
	int row_stride = info->pitchBytes; /* JSAMPLEs per row in image_buffer */

	JSAMPLE* src = (JSAMPLE*)info->data;

	while (cinfo.next_scanline < cinfo.image_height) {
		/* jpeg_write_scanlines expects an array of pointers to scanlines.
		* Here the array is only one element long, but you could pass
		* more than one scanline at a time if that's more convenient.
		*/
		JSAMPROW row_pointer[1];      /* pointer to JSAMPLE row[s] */
		row_pointer[0] = src + cinfo.next_scanline * row_stride;
		(void)libjpeg->jpeg_write_scanlines(&cinfo, row_pointer, 1);
	}

	/* Step 6: Finish compression */
	libjpeg->jpeg_finish_compress(&cinfo);

	/* Step 7: release JPEG compression object */

	/* This is an important step since it will release a good deal of memory. */
	libjpeg->jpeg_destroy_compress(&cinfo);

	/* And we're done! */
	return true;
}

bool kr::backend::Jpeg::load(KrbImageCallback* callback, KrbFile* file) noexcept
{
	LibJpeg* libjpeg = LibJpeg::getInstance();
	if (libjpeg == nullptr) return false;
	/* This struct contains the JPEG decompression parameters and pointers to
	* working space (which is allocated as needed by the JPEG library).
	*/
	struct jpeg_decompress_struct cinfo;
	/* We use our private extension JPEG error handler.
	* Note that this struct must live as long as the main JPEG parameter
	* struct, to avoid dangling-pointer problems.
	*/
	struct my_error_mgr jerr;
	/* More stuff */
	JSAMPARRAY buffer;            /* Output row buffer */
	int row_stride;               /* physical row width in output buffer */

								  /* In this example we want to open the input file before doing anything else,
								  * so that the setjmp() error recovery below can assume the file is open.
								  * VERY IMPORTANT: use "b" option to fopen() if you are on a machine that
								  * requires it in order to read binary files.
								  */

								  /* Step 1: allocate and initialize JPEG decompression object */

								  /* We set up the normal JPEG error routines, then override error_exit. */
	cinfo.err = libjpeg->jpeg_std_error(&jerr.pub);
	jerr.pub.error_exit = my_error_exit;
	/* Establish the setjmp return context for my_error_exit to use. */
	if (setjmp(jerr.setjmp_buffer)) {
		/* If we get here, the JPEG code has signaled an error.
		* We need to clean up the JPEG object, close the input file, and return.
		*/
		libjpeg->jpeg_destroy_decompress(&cinfo);
		return false;
	}
	/* Now we can initialize the JPEG decompression object. */
	libjpeg->jpeg_create_decompress(&cinfo);

	/* Step 2: specify data source (eg, a file) */
	kr_jpeg_source_mgr::make(&cinfo, file);

	/* Step 3: read file parameters with jpeg_read_header() */

	(void)libjpeg->jpeg_read_header(&cinfo, TRUE);
	/* We can ignore the return value from jpeg_read_header since
	*   (a) suspension is not possible with the stdio data source, and
	*   (b) we passed TRUE to reject a tables-only JPEG file as an error.
	* See libjpeg.txt for more info.
	*/

	/* Step 4: set parameters for decompression */

	/* In this example, we don't need to change any of the defaults set by
	* jpeg_read_header(), so we do nothing here.
	*/

	/* Step 5: Start decompressor */

	(void)libjpeg->jpeg_start_decompress(&cinfo);
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

	KrbImageInfo imginfo;
	imginfo.width = cinfo.output_width;
	imginfo.pitchBytes = row_stride;
	imginfo.height = cinfo.output_height;
	imginfo.pixelformat = PixelFormatBGR8;
	char* dest = (char*)callback->start(callback, &imginfo);
	if (!dest)
	{
		libjpeg->jpeg_finish_decompress(&cinfo);
		libjpeg->jpeg_destroy_decompress(&cinfo);
		return false;
	}

	/* Make a one-row-high sample array that will go away when done with image */
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr)&cinfo, JPOOL_IMAGE, row_stride, 1);

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
		(void)libjpeg->jpeg_read_scanlines(&cinfo, buffer, 1);
		/* Assume put_scanline_someplace wants a pointer and sample count. */
		memcpy(dest, buffer[0], row_stride);
		dest += imginfo.pitchBytes;
	}

	/* Step 7: Finish decompression */

	libjpeg->jpeg_finish_decompress(&cinfo);
	/* We can ignore the return value since suspension is not possible
	* with the stdio data source.
	*/

	/* Step 8: Release JPEG decompression object */

	/* This is an important step since it will release a good deal of memory. */
	libjpeg->jpeg_destroy_decompress(&cinfo);

	/* After finish_decompress, we can close the input file.
	* Here we postpone it until after no more JPEG errors are possible,
	* so as to simplify the setjmp error logic above.  (Actually, I don't
	* think that jpeg_destroy can do an error exit, but why assume anything...)
	*/

	/* At this point you may want to check to see whether any corrupt-data
	* warnings occurred (test whether jerr.pub.num_warnings is nonzero).
	*/

	/* And we're done! */
	return true;
}
