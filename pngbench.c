// Compilation: gcc -O2 pngbench.c -o pngbench -lpng
/*
===========================================================================

The libpng png decoding code was adapted from RBDoom3BFG by Daniel Gibson and patched by Tobias Frost.

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company.
Copyright (C) 2012-2014 Robert Beckebans

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
===========================================================================
*/

#include <stdio.h>
#include <string.h>
#include <limits.h> // PATH_MAX
#include <png.h>
#include <time.h>
#include <stdlib.h>

#define eprintf(...) fprintf(stderr, __VA_ARGS__)

const char* progName = "pngbench";

typedef unsigned char byte;

static void printUsage()
{
	eprintf("Usage: %s <imgname>\n", progName);
	eprintf(" e.g.: %s test.png\n", progName);
	eprintf("       %s /path/to/file.png\n", progName);
}

struct image
{
	unsigned char* data;
	int w;
	int h;
	int format; // 3: RGB, 4: RGBA
};

// the libpng png loading code is stolen from rbdoom3bfg's LoadPNG()

static void freeImage(struct image* img)
{
	free(img->data);
	img->data = NULL;
}

static void png_Error( png_structp pngPtr, png_const_charp msg )
{
	eprintf( "%s\n", msg );
	exit(1);
}

static void png_Warning( png_structp pngPtr, png_const_charp msg )
{
	eprintf( "%s\n", msg );
}

static void	png_ReadData( png_structp pngPtr, png_bytep data, png_size_t length )
{
	// There is a get_io_ptr but not a set_io_ptr.. Therefore we need some tmp storage here.
	byte **ioptr = (byte **)png_get_io_ptr(pngPtr);

	memcpy( data, *ioptr, length );
	*ioptr += length;
}

static struct image loadImage(const char* filename, byte* fbuffer, byte* readptr, int len)
{
	struct image ret = {0};
	
		// create png_struct with the custom error handlers
	png_structp pngPtr = png_create_read_struct( PNG_LIBPNG_VER_STRING, ( png_voidp ) NULL, png_Error, png_Warning );
	if( !pngPtr )
	{
		eprintf( "LoadPNG( %s ): png_create_read_struct failed\n", filename );
		exit(1);
	}
	
	// allocate the memory for image information
	png_infop infoPtr = png_create_info_struct( pngPtr );
	if( !infoPtr )
	{
		eprintf( "LoadPNG( %s ): png_create_info_struct failed\n", filename );
		exit(1);
	}
	
	readptr = fbuffer;
	png_set_read_fn( pngPtr, &readptr, png_ReadData );
	
	png_set_sig_bytes( pngPtr, 0 );
	
	png_read_info( pngPtr, infoPtr );
	
	png_uint_32 pngWidth, pngHeight;
	int bitDepth, colorType, interlaceType;
	png_get_IHDR( pngPtr, infoPtr, &pngWidth, &pngHeight, &bitDepth, &colorType, &interlaceType, NULL, NULL );
	
	// 16 bit -> 8 bit
	png_set_strip_16( pngPtr );
	
	// 1, 2, 4 bit -> 8 bit
	if( bitDepth < 8 )
	{
		png_set_packing( pngPtr );
	}

	if( colorType & PNG_COLOR_MASK_PALETTE )
	{
		png_set_expand( pngPtr );
	}

	if( !( colorType & PNG_COLOR_MASK_COLOR ) )
	{
		png_set_gray_to_rgb( pngPtr );
	}

	// set paletted or RGB images with transparency to full alpha so we get RGBA
	if( png_get_valid( pngPtr, infoPtr, PNG_INFO_tRNS ) )
	{
		png_set_tRNS_to_alpha( pngPtr );
	}

	// make sure every pixel has an alpha value
	if( !( colorType & PNG_COLOR_MASK_ALPHA ) )
	{
		png_set_filler( pngPtr, 255, PNG_FILLER_AFTER );
	}

	png_read_update_info( pngPtr, infoPtr );

	byte* out = ( byte* )malloc( pngWidth * pngHeight * 4 );

	ret.data = out;
	ret.w = pngWidth;
	ret.h = pngHeight;
	ret.format = 4;

	png_uint_32 rowBytes = png_get_rowbytes( pngPtr, infoPtr );

	png_bytep* rowPointers = ( png_bytep* ) malloc( sizeof( png_bytep ) * pngHeight );
	for( png_uint_32 row = 0; row < pngHeight; row++ )
	{
		rowPointers[row] = ( png_bytep )( out + ( row * pngWidth * 4 ) );
	}

	png_read_image( pngPtr, rowPointers );

	png_read_end( pngPtr, infoPtr );

	png_destroy_read_struct( &pngPtr, &infoPtr, NULL );

	free( rowPointers );

	return ret;
}

int main(int argc, char** argv)
{
	progName = argv[0];
	if(argc < 2)
	{
		printUsage();
		exit(1);
	}

	const char* filename = argv[1];
	
	byte*	fbuffer;
	byte*   readptr;
	int     len;
	{
		FILE* f = fopen(filename, "r");
		if(f == NULL)
		{
			eprintf("ERROR: Couldn't open %s!\n", filename);
			exit(1);
		}
		
		fseek(f, 0, SEEK_END);
		len = ftell(f);
		fseek(f, 0, SEEK_SET); // go back to start
		
		fbuffer = ( byte* )malloc( len + 4096 );
		byte* buf = fbuffer;
		int remaining = len;
		while(remaining)
		{
			int block = remaining;
			int read = fread( buf, 1, block, f );
			if(read < 0)
			{
				eprintf("Error while reading file!\n");
				exit(1);
			}
			remaining -= read;
			buf += read;
		}
		
	}
	
#define numIterations 100
	
	struct timespec before = {0};
	struct timespec after = {0};
	
	clock_gettime(CLOCK_MONOTONIC_RAW, &before);
	
	int i;
	for(i=0; i<numIterations; ++i)
	{
		struct image img = loadImage(filename, fbuffer, readptr, len);
		freeImage(&img);
	}
	
	clock_gettime(CLOCK_MONOTONIC_RAW, &after);
	
	int secs = after.tv_sec - before.tv_sec;
	int nsecs = after.tv_nsec - before.tv_nsec;
	
	if(nsecs < 0)
	{
		--secs;
		nsecs += 1000000000;
	}
	
	double ms = 1000.0*secs;
	ms += nsecs * 0.000001;
	printf("Decoding %s %d times took %fms => %fms avg\n", filename, numIterations, ms, ms/((float)numIterations));
	
	return 0;
}
