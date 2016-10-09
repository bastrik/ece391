/*									tab:8
 *
 * photo.c - photo display functions
 *
 * "Copyright (c) 2011 by Steven S. Lumetta."
 *
 * Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose, without fee, and without written agreement is
 * hereby granted, provided that the above copyright notice and the following
 * two paragraphs appear in all copies of this software.
 * 
 * IN NO EVENT SHALL THE AUTHOR OR THE UNIVERSITY OF ILLINOIS BE LIABLE TO 
 * ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL 
 * DAMAGES ARISING OUT  OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION, 
 * EVEN IF THE AUTHOR AND/OR THE UNIVERSITY OF ILLINOIS HAS BEEN ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * THE AUTHOR AND THE UNIVERSITY OF ILLINOIS SPECIFICALLY DISCLAIM ANY 
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.  THE SOFTWARE 
 * PROVIDED HEREUNDER IS ON AN "AS IS" BASIS, AND NEITHER THE AUTHOR NOR
 * THE UNIVERSITY OF ILLINOIS HAS ANY OBLIGATION TO PROVIDE MAINTENANCE, 
 * SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS."
 *
 * Author:	    Steve Lumetta
 * Version:	    3
 * Creation Date:   Fri Sep  9 21:44:10 2011
 * Filename:	    photo.c
 * History:
 *	SL	1	Fri Sep  9 21:44:10 2011
 *		First written (based on mazegame code).
 *	SL	2	Sun Sep 11 14:57:59 2011
 *		Completed initial implementation of functions.
 *	SL	3	Wed Sep 14 21:49:44 2011
 *		Cleaned up code for distribution.
 */


#include <string.h>

#include "assert.h"
#include "modex.h"
#include "photo.h"
#include "photo_headers.h"
#include "world.h"


/* types local to this file (declared in types.h) */

/* 
 * A room photo.  Note that you must write the code that selects the
 * optimized palette colors and fills in the pixel data using them as 
 * well as the code that sets up the VGA to make use of these colors.
 * Pixel data are stored as one-byte values starting from the upper
 * left and traversing the top row before returning to the left of
 * the second row, and so forth.  No padding should be used.
 */
struct photo_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t        palette[192][3];     /* optimized palette colors */
    uint8_t*       img;                 /* pixel data               */
};

/* 
 * An object image.  The code for managing these images has been given
 * to you.  The data are simply loaded from a file, where they have 
 * been stored as 2:2:2-bit RGB values (one byte each), including 
 * transparent pixels (value OBJ_CLR_TRANSP).  As with the room photos, 
 * pixel data are stored as one-byte values starting from the upper 
 * left and traversing the top row before returning to the left of the 
 * second row, and so forth.  No padding is used.
 */
struct image_t {
    photo_header_t hdr;			/* defines height and width */
    uint8_t*       img;                 /* pixel data               */
};


/* file-scope variables */

/* 
 * The room currently shown on the screen.  This value is not known to 
 * the mode X code, but is needed when filling buffers in callbacks from 
 * that code (fill_horiz_buffer/fill_vert_buffer).  The value is set 
 * by calling prep_room.
 */
static const room_t* cur_room = NULL; 

/*
 * The Octree node data structure for level 2
 */
 struct octree_L2
 {
 	uint32_t pixel_count;						// tracks the occurence of a color
 	uint32_t sum_red, sum_green, sum_blue;		// tracks the sum of RGB @ node
 	uint32_t child_index_pointer[OCTREE_L2_CHILD];	// tracks L4 children
 };

 /*
 * The Octree node data structure for level 4
 */
 struct octree_L4
 {
 	uint32_t pixel_count;						// tracks the occurence of a color
 	uint32_t index;								// tracks pixel's initial location			
 	uint32_t sum_red, sum_green, sum_blue;		// tracks the sum of RGB @ node
 };



/* 
 * fill_horiz_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the leftmost 
 *                pixel of a line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- leftmost pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_horiz_buffer (int x, int y, unsigned char buf[SCROLL_X_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgx;  /* loop index over pixels in object image      */ 
    int            yoff;  /* y offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_X_DIM; idx++) {
        buf[idx] = (0 <= x + idx && view->hdr.width > x + idx ?
		    view->img[view->hdr.width * y + x + idx] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (y < obj_y || y >= obj_y + img->hdr.height ||
	    x + SCROLL_X_DIM <= obj_x || x >= obj_x + img->hdr.width) {
	    continue;
	}

	/* The y offset of drawing is fixed. */
	yoff = (y - obj_y) * img->hdr.width;

	/* 
	 * The x offsets depend on whether the object starts to the left
	 * or to the right of the starting point for the line being drawn.
	 */
	if (x <= obj_x) {
	    idx = obj_x - x;
	    imgx = 0;
	} else {
	    idx = 0;
	    imgx = x - obj_x;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_X_DIM > idx && img->hdr.width > imgx; idx++, imgx++) {
	    pixel = img->img[yoff + imgx];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * fill_vert_buffer
 *   DESCRIPTION: Given the (x,y) map pixel coordinate of the top pixel of 
 *                a vertical line to be drawn on the screen, this routine 
 *                produces an image of the line.  Each pixel on the line
 *                is represented as a single byte in the image.
 *
 *                Note that this routine draws both the room photo and
 *                the objects in the room.
 *
 *   INPUTS: (x,y) -- top pixel of line to be drawn 
 *   OUTPUTS: buf -- buffer holding image data for the line
 *   RETURN VALUE: none
 *   SIDE EFFECTS: none
 */
void
fill_vert_buffer (int x, int y, unsigned char buf[SCROLL_Y_DIM])
{
    int            idx;   /* loop index over pixels in the line          */ 
    object_t*      obj;   /* loop index over objects in the current room */
    int            imgy;  /* loop index over pixels in object image      */ 
    int            xoff;  /* x offset into object image                  */ 
    uint8_t        pixel; /* pixel from object image                     */
    const photo_t* view;  /* room photo                                  */
    int32_t        obj_x; /* object x position                           */
    int32_t        obj_y; /* object y position                           */
    const image_t* img;   /* object image                                */

    /* Get pointer to current photo of current room. */
    view = room_photo (cur_room);

    /* Loop over pixels in line. */
    for (idx = 0; idx < SCROLL_Y_DIM; idx++) {
        buf[idx] = (0 <= y + idx && view->hdr.height > y + idx ?
		    view->img[view->hdr.width * (y + idx) + x] : 0);
    }

    /* Loop over objects in the current room. */
    for (obj = room_contents_iterate (cur_room); NULL != obj;
    	 obj = obj_next (obj)) {
	obj_x = obj_get_x (obj);
	obj_y = obj_get_y (obj);
	img = obj_image (obj);

        /* Is object outside of the line we're drawing? */
	if (x < obj_x || x >= obj_x + img->hdr.width ||
	    y + SCROLL_Y_DIM <= obj_y || y >= obj_y + img->hdr.height) {
	    continue;
	}

	/* The x offset of drawing is fixed. */
	xoff = x - obj_x;

	/* 
	 * The y offsets depend on whether the object starts below or 
	 * above the starting point for the line being drawn.
	 */
	if (y <= obj_y) {
	    idx = obj_y - y;
	    imgy = 0;
	} else {
	    idx = 0;
	    imgy = y - obj_y;
	}

	/* Copy the object's pixel data. */
	for (; SCROLL_Y_DIM > idx && img->hdr.height > imgy; idx++, imgy++) {
	    pixel = img->img[xoff + img->hdr.width * imgy];

	    /* Don't copy transparent pixels. */
	    if (OBJ_CLR_TRANSP != pixel) {
		buf[idx] = pixel;
	    }
	}
    }
}


/* 
 * image_height
 *   DESCRIPTION: Get height of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_height (const image_t* im)
{
    return im->hdr.height;
}


/* 
 * image_width
 *   DESCRIPTION: Get width of object image in pixels.
 *   INPUTS: im -- object image pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of object image im in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
image_width (const image_t* im)
{
    return im->hdr.width;
}

/* 
 * photo_height
 *   DESCRIPTION: Get height of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: height of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_height (const photo_t* p)
{
    return p->hdr.height;
}


/* 
 * photo_width
 *   DESCRIPTION: Get width of room photo in pixels.
 *   INPUTS: p -- room photo pointer
 *   OUTPUTS: none
 *   RETURN VALUE: width of room photo p in pixels
 *   SIDE EFFECTS: none
 */
uint32_t 
photo_width (const photo_t* p)
{
    return p->hdr.width;
}


/* 
 * prep_room
 *   DESCRIPTION: Prepare a new room for display.  You might want to set
 *                up the VGA palette registers according to the color
 *                palette that you chose for this room.
 *   INPUTS: r -- pointer to the new room
 *   OUTPUTS: none
 *   RETURN VALUE: none
 *   SIDE EFFECTS: changes recorded cur_room for this file
 */
void
prep_room (const room_t* r)
{
    /* Record the current room. */
    photo_t* p = room_photo(r);
    fill_palette_room(p->palette);  // function in modex.c
    cur_room = r;
}


/* 
 * read_obj_image
 *   DESCRIPTION: Read size and pixel data in 2:2:2 RGB format from a
 *                photo file and create an image structure from it.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the image
 */
image_t*
read_obj_image (const char* fname)
{
    FILE*    in;		/* input file               */
    image_t* img = NULL;	/* image structure          */
    uint16_t x;			/* index over image columns */
    uint16_t y;			/* index over image rows    */
    uint8_t  pixel;		/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the image pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (img = malloc (sizeof (*img))) ||
	NULL != (img->img = NULL) || /* false clause for initialization */
	1 != fread (&img->hdr, sizeof (img->hdr), 1, in) ||
	MAX_OBJECT_WIDTH < img->hdr.width ||
	MAX_OBJECT_HEIGHT < img->hdr.height ||
	NULL == (img->img = malloc 
		 (img->hdr.width * img->hdr.height * sizeof (img->img[0])))) {
	if (NULL != img) {
	    if (NULL != img->img) {
	        free (img->img);
	    }
	    free (img);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
    for (y = img->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
	for (x = 0; img->hdr.width > x; x++) {

	    /* 
	     * Try to read one 8-bit pixel.  On failure, clean up and 
	     * return NULL.
	     */
	    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
		free (img->img);
		free (img);
	        (void)fclose (in);
		return NULL;
	    }

	    /* Store the pixel in the image data. */
	    img->img[img->hdr.width * y + x] = pixel;
	}
    }

    /* All done.  Return success. */
    (void)fclose (in);
    return img;
}


/* 
 * read_photo
 *   DESCRIPTION: Read size and pixel data in 5:6:5 RGB format from a
 *                photo file and create a photo structure from it.
 *                Code provided simply maps to 2:2:2 RGB.  You must
 *                replace this code with palette color selection, and
 *                must map the image pixels into the palette colors that
 *                you have defined.
 *   INPUTS: fname -- file name for input
 *   OUTPUTS: none
 *   RETURN VALUE: pointer to newly allocated photo on success, or NULL
 *                 on failure
 *   SIDE EFFECTS: dynamically allocates memory for the photo
 */
photo_t*
read_photo (const char* fname)
{
    FILE*    in;	/* input file               */
    photo_t* p = NULL;	/* photo structure          */
    uint16_t x;		/* index over image columns */
    uint16_t y;		/* index over image rows    */
    uint16_t pixel;	/* one pixel from the file  */

    /* 
     * Open the file, allocate the structure, read the header, do some
     * sanity checks on it, and allocate space to hold the photo pixels.
     * If anything fails, clean up as necessary and return NULL.
     */
    if (NULL == (in = fopen (fname, "r+b")) ||
	NULL == (p = malloc (sizeof (*p))) ||
	NULL != (p->img = NULL) || /* false clause for initialization */
	1 != fread (&p->hdr, sizeof (p->hdr), 1, in) ||
	MAX_PHOTO_WIDTH < p->hdr.width ||
	MAX_PHOTO_HEIGHT < p->hdr.height ||
	NULL == (p->img = malloc 
		 (p->hdr.width * p->hdr.height * sizeof (p->img[0])))) {
	if (NULL != p) {
	    if (NULL != p->img) {
	        free (p->img);
	    }
	    free (p);
	}
	if (NULL != in) {
	    (void)fclose (in);
	}
	return NULL;
    }

    /******************************
     * Optimize color code starts *
     ******************************/

     uint32_t image_len = p->hdr.width * p->hdr.height;

     struct octree_L2 L2[OCTREE_L2_LEN];
     struct octree_L4 L4[OCTREE_L4_LEN];

     // initialize L2
     uint32_t i, j;
     uint32_t L2_index, L4_index, child;
     uint32_t temp;
     uint32_t avg_red, avg_green, avg_blue;

     for (i = 0; i < OCTREE_L2_LEN; i++)
     {
     	L2[i].pixel_count = 0;
     	L2[i].sum_red = 0;
     	L2[i].sum_green = 0;
     	L2[i].sum_blue = 0;

     	for (j = 0; j < OCTREE_L2_CHILD; j++)
     		L2[i].child_index_pointer[j] = -1;  // initially our L2 octree has no L4 child
     }

     // initialize L4
     for (i = 0; i < OCTREE_L4_LEN; i++)
     {
     	L4[i].index = i;			// we want to remember where this pixel is initially
     	L4[i].pixel_count = 0;
     	L4[i].sum_red = 0;
     	L4[i].sum_green = 0;
     	L4[i].sum_blue = 0;
     }

    /* 
     * Loop over rows from bottom to top.  Note that the file is stored
     * in this order, whereas in memory we store the data in the reverse
     * order (top to bottom).
     */
     uint16_t pixels[image_len]; // stores pixel arrangement info for later access

    for (y = p->hdr.height; y-- > 0; ) {

	/* Loop over columns from left to right. */
		for (x = 0; p->hdr.width > x; x++) {

		    /* 
		     * Try to read one 16-bit pixel.  On failure, clean up and 
		     * return NULL.
		     */
		    if (1 != fread (&pixel, sizeof (pixel), 1, in)) {
			free (p->img);
			free (p);
		        (void)fclose (in);
			return NULL;

		    }
		    /* 
		     * 16-bit pixel is coded as 5:6:5 RGB (5 bits red, 6 bits green,
		     * and 6 bits blue).  We change to 2:2:2, which we've set for the
		     * game objects.  You need to use the other 192 palette colors
		     * to specialize the appearance of each photo.
		     *
		     * In this code, you need to calculate the p->palette values,
		     * which encode 6-bit RGB as arrays of three uint8_t's.  When
		     * the game puts up a photo, you should then change the palette 
		     * to match the colors needed for that photo.
		     */

		    /* 
		     * Don't need this place holder code anymore

		     *p->img[p->hdr.width * y + x] = (((pixel >> 14) << 4) |
			 *			    (((pixel >> 9) & 0x3) << 2) |
			 *			    ((pixel >> 3) & 0x3));
			 */

		    pixels[y * p->hdr.width + x] = pixel; // store pixel location for later access

		    i = rgb16_to_rgb12(pixel);	// calls helper to get the location to put pixel

		    L4[i].pixel_count++;		// increment the times we got this pixel
		    L4[i].sum_red   += (pixel >> 11) & 0x1F;	// accum low 5 bit using bitmask 0x1F
		    L4[i].sum_green += (pixel >> 5) & 0x3F;		// accum low 6 bit using bitmask 0x3F
		    L4[i].sum_blue  += pixel & 0x1F;			// accum low 5 bit using bitmask 0x1F		    
		}
    }

    /* All done.  Return success. */
    (void)fclose (in);

    // now we sort L4 base on frequency of color
    qsort(L4, OCTREE_L4_LEN, sizeof(struct octree_L4), qsort_compare);

    // for the highest 128 frequencies...
    for (i = 0; i < OCTREE_L4_SORTED; i++)
    {
    	// we want to add the L4 pixels to their parent in L2
    	L4_index = L4[i].index;
    	L2_index = rgb12_to_rgb6(L4_index);		// curr node's L2 parent idx
    	child = child_from_rgb12(L4_index);		// respective L2's child idx

    	L2[L2_index].child_index_pointer[child] = i;

    	// we want to add the top 128 color into the palette
    	temp = L4[i].pixel_count;	// use temp to store pixel_count
		avg_red = (temp == 0) ? 0 : L4[i].sum_red / temp;
		avg_green = (temp == 0) ? 0 : L4[i].sum_green / temp;
		avg_blue = (temp == 0) ? 0 : L4[i].sum_blue / temp;
    	    	
    	p->palette[i][0] = (uint8_t) (avg_red & 0x1F) << 1;
    	p->palette[i][1] = (uint8_t) (avg_green & 0x3F);
    	p->palette[i][2] = (uint8_t) (avg_blue & 0x1F) << 1;
    }

    // for the rest of the frequencies...
    for (i = OCTREE_L4_SORTED; i < OCTREE_L4_LEN; i++)
    {
    	// merge with L2 nodes
    	L2_index = rgb12_to_rgb6(L4[i].index); 		// curr node's L2 parent idx

    	L2[L2_index].pixel_count += L4[i].pixel_count;
    	L2[L2_index].sum_red += L4[i].sum_red;
     	L2[L2_index].sum_green += L4[i].sum_green;
     	L2[L2_index].sum_blue += L4[i].sum_blue;
    }

    // then we want to add L2 pixels to palette
    for (i = 0; i < OCTREE_L2_LEN; i++)
    {
    	// we want to add the last 64 colors
    	temp = L2[i].pixel_count;	// use temp to store pixel_count
		avg_red = (temp == 0) ? 0 : L2[i].sum_red / temp;
		avg_green = (temp == 0) ? 0 : L2[i].sum_green / temp;
		avg_blue = (temp == 0) ? 0 : L2[i].sum_blue / temp;
		
    	p->palette[i + OCTREE_L4_SORTED][0] = (uint8_t) (avg_red & 0x1F) << 1;
    	p->palette[i + OCTREE_L4_SORTED][1] = (uint8_t) (avg_green & 0x3F);
    	p->palette[i + OCTREE_L4_SORTED][2] = (uint8_t) (avg_blue & 0x1F) << 1;
    }

    // We don't want to do all the work for nothing!
    for (i = 0; i < image_len; i++)
    {
    	L4_index = rgb16_to_rgb12(pixels[i]);
    	L2_index = rgb12_to_rgb6 (L4_index);
    	child = child_from_rgb12 (L4_index);
    	uint32_t L4_index_sorted = L2[L2_index].child_index_pointer[child];

    	p->img[i] = (L4_index_sorted == -1) ? 
    		L2_index + OCTREE_L4_SORTED + 64 : L4_index_sorted + 64;

    }


    return p;
}


/*
 *	rgb16_to_rgb12
 *
 *	Description: 	helper that converts 16 bit RGB to 12 bit RGB
 *	Input: 			16 bit RGB (5:6:5)
 *	Output: 		None
 *	Return Value:	12 bit RGB (4:4:4)
 *	Side Effects: 	None
 */
int rgb16_to_rgb12(int rgb16)
{
	int r = ((rgb16 >> 12)& 0x0F) << 8;		// take high 4 bits
	int g = ((rgb16 >> 7) & 0x0F) << 4;		// 0x0F masks keep lower 4 bit
	int b = (rgb16 >> 1) & 0x0F;
	return (r | g | b);
}

/*
 *	rgb12_to_rgb6
 *
 *	Description: 	helper that converts 12 bit RGB to 6 bit RGB
 *	Input: 			12 bit RGB (4:4:4)
 *	Output: 		None
 *	Return Value:	6 bit RGB (2:2:2)
 *	Side Effects: 	None
 */
int rgb12_to_rgb6(int rgb12)
{
	int r = ((rgb12 >> 10) & 0x03) << 4;	// take high 2 bits
	int g = ((rgb12 >> 6) & 0x03) << 2;		// 0x03 masks keep lower 2 bit
	int b = (rgb12 >> 2) & 0x03;
	return (r | g | b);
}

/*
 *	child_from_rgb12
 *
 *	Description: 	helper that converts 12 bit RGB to L2 child index
 *	Input: 			12 bit RGB (4:4:4)
 *	Output: 		None
 *	Return Value:	L2 child index value
 *	Side Effects: 	None
 */
int child_from_rgb12(int rgb12)
{
	int r = ((rgb12 >> 8) & 0x03) << 4;		// take low 2 bits
	int g = ((rgb12 >> 4) & 0x03) << 2;
	int b =  rgb12 & 0x03;
	return (r | g | b);
}

/*
 *	qsort_compare
 *
 *	Description: 	overriden compare function for system qsort
 *	Input: 			2 nodes from an octree
 *	Output: 		None
 *	Return Value: 	difference between pixel count of 2 nodes
 *	Side Effects: None
 */

int qsort_compare(const void* p1, const void* p2)
{
	const struct octree_L4* a = p1;
	const struct octree_L4* b = p2;
	return (b->pixel_count - a->pixel_count);
}
