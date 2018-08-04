/**
 * @file bmp.c
 * @author Namir Hassan
 * @date 03/23/2018
 * @brief Bitmap image processing program
 * @bugs None
 */

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/types.h>
#include <sys/uio.h>

#include "apue.h"

int hex2dec(unsigned char *arr, int bytes);
int hexchar2dec(unsigned char hexchar);
void print3BytesPixelNoColorTable(unsigned char *buf, int rowsize, int row);
void print1BytesPixelNoColorTable(unsigned char *buf, int rowsize, int row);
void changePixelColor(unsigned char *buf, int rowsize, int row, int red, int green, int blue);

int main(int argc, char **argv) {

	int input = 0;
	int ifd;
	int ofd;
	char *infile = NULL;
	char *outfile = NULL;
	int i = 0;
	unsigned char arr[10];
	unsigned char fileheader[14];
	unsigned char *dibheader = NULL;
	unsigned char *colortable = NULL;
	int opti = 0;
	int headercheck = 0;
	int opto = 0;
	int optd = 0;
	int optR = 0;
	int optr = -1;
	int optg = -1;
	int optb = -1;
	int numofcolor = 0;
	int compressionMethod = 0;
	int imagewidth = 0;
	int bitperpixel = 0;
	int imageheight = 0;
	int n;

	while ((input = getopt(argc, argv, "hidRo:r:g:b:")) != -1) {
		switch (input) {
		case 'o':
			opto = 1;
			//printf("%s\n", optarg);
			outfile = optarg;

			if ((ofd = open(outfile, O_CREAT | O_WRONLY, 0660)) < 0) {

				err_exit(2, "ERROR: output file: %s can not be open\n",
						outfile);
			}
			break;
		case 'i':
			opti = 1;
			break;
		case 'd':
			optd = 1;
			opti = 1;
			break;
		case 'R':
			optR = 1;
			break;
		case 'r':
			optr = atoi(optarg);
			if (optr < 0 || optr > 255)
				err_quit(
						"ERROR: color value %d for -r is not in 0 and 255, try again!\n",
						optr);
			break;
		case 'g':
			optg = atoi(optarg);
			if (optg < 0 || optg > 255)
				err_quit(
						"ERROR: color value %d for -g is not in 0 and 255, try again!\n",
						optg);
			break;
		case 'b':
			optb = atoi(optarg);
			if (optb < 0 || optb > 255)
				err_quit(
						"ERROR: color value %d for -b is not in 0 and 255, try again!\n",
						optb);
			break;
		case 'h':
			printf(
					"usage: bmp FILE [-o FILE] [-i] [-d] [-R] [-r N] [-g N] [-b N]");
			printf("\nFILE : the input file (required)\n");
			printf("\n-o FILE : the output file (required if options -r, -g, -b are set)\n");
			printf("\n-i : display the bitmap header and DIB header information\n");
			printf("\n-d : dump of all headers, color table if available and pixel values of\n");
			printf("\neach cell, one line per pixel\n");
			printf("\n-R : reverse the image of an 8bpp (grayscale) image only (black to white vice versa)\n");
			printf("\nWrite to an output file when using the -R option by including (-o FILE)\n");
			printf("\n-r : change value of all red pixels to value N in a 24bpp bitmap (N between 0 and 255)\n");
			printf("\n-g : change value of all green pixels to value N in a 24bpp bitmap (N between 0 and 255)\n");
			printf("\n-b : change value of all blue pixels to value N in a 24bpp bitmap (N between 0 and 255)\n\n");
			exit(1);
			break;

		default:
			break;
		}

	}

	// Done with getopt parsing, let's see what were set.
	if (optR || (optr > -1) || (optg > -1) || (optb > -1)) {

		if (!opto) { // output file supposted to be set
			err_quit("ERROR: option -o not set, try again!\n");
		}
	}

	// without option, commandline argument must be read from here
	infile = argv[optind];
	if (infile == NULL) {
		err_exit(2, "ERROR: %s not found\n", infile);
	}

	umask(0);
	if ((ifd = open(infile, O_RDONLY, 0660)) == -1) {

		err_exit(2, "ERROR: cannot find or open file: %s ", infile);
	}


	int pagesize = sysconf(_SC_PAGESIZE);
	unsigned char *buf = (unsigned char *) malloc(
			sizeof(unsigned char) * pagesize);

	if (buf == NULL) {
		err_quit("ERROR: Creating buffer\n");
	}

	int nbytes = 0;
	nbytes = read_all(ifd, buf, pagesize);

	if (nbytes == -1) {
		err_quit("ERROR: Reading from input file\n");
	}
	// check error condition for read_all here

	unsigned char *copybuf = (unsigned char *) memcpy(fileheader, buf, 14); // File Header copied

	if (copybuf == NULL) {

		err_quit("Error: File header memcpy failed\n");
	}

	//
	// Reading Fileheader fields
	if (opti)
		printf("\nheader: %c%c\n", fileheader[0], fileheader[1]);

	sprintf(arr, "%.2x%.2x%.2x%.2x", fileheader[5], fileheader[4],
			fileheader[3], fileheader[2]);

	if (opti)
		printf("size of bitmap (bytes): %d\n", hex2dec(arr, 8));

	sprintf(arr, "%.2x%.2x%.2x%.2x", fileheader[13], fileheader[12],
			fileheader[11], fileheader[10]);
	int startofbmp = hex2dec(arr, 8);
	if (opti)
		printf("offset (start of image data): %d\n", startofbmp);

	sprintf(arr, "%.2x%.2x%.2x%.2x", buf[17], buf[16], buf[15], buf[14]);
	int dibsize = hex2dec(arr, 8);
	if (opti)
		printf("\nsize of dib: %d\n", dibsize);

	//
	// Reading DIB fields
	if (dibsize == 40) {
		dibheader = (unsigned char *) malloc(sizeof(unsigned char) * dibsize);
		if (dibheader == NULL) {
			err_quit("ERROR: Creating dib header\n");
		}

		copybuf = memcpy(dibheader, &buf[14], dibsize); //Copied DIB header
		if (copybuf == NULL) {
			err_exit(1, "Error: memcpy failed\n");
		}

		sprintf(arr, "%.2x%.2x%.2x%.2x", dibheader[7], dibheader[6],
				dibheader[5], dibheader[4]);
		imagewidth = hex2dec(arr, 8);
		if (opti)
			printf("bitmap width in pixels: %d\n", imagewidth);

		sprintf(arr, "%.2x%.2x%.2x%.2x", dibheader[11], dibheader[10],
				dibheader[9], dibheader[8]);
		imageheight = hex2dec(arr, 8);
		if (opti)
			printf("bitmap height in pixels: %d\n", imageheight);

		sprintf(arr, "%.2x%.2x", dibheader[13], dibheader[12]);
		if (opti)
			printf("number of color planes: %d\n", hex2dec(arr, 4));

		sprintf(arr, "%.2x%.2x", dibheader[15], dibheader[14]);
		bitperpixel = hex2dec(arr, 4);
		if (opti)
			printf("number of bits per pixel: %d\n", bitperpixel);
		if (optR && (bitperpixel != 8)) { //Check for reverse and 8 bits/pixel, if not then error.
			err_quit(
					"ERROR: option -R but wrong image bit/pixels: %d != 8, try again!\n",
					bitperpixel);
		}

		sprintf(arr, "%.2x%.2x%.2x%.2x", dibheader[19], dibheader[18],
				dibheader[17], dibheader[16]);
		compressionMethod = hex2dec(arr, 8);
		if (opti)
			printf("compression method: %d\n", compressionMethod);

		sprintf(arr, "%.2x%.2x%.2x%.2x", dibheader[23], dibheader[22],
				dibheader[21], dibheader[20]);
		int imagesize = hex2dec(arr, 8);
		if (opti)
			printf("image size: %d\n", imagesize);

		sprintf(arr, "%.2x%.2x%.2x%.2x", dibheader[27], dibheader[26],
				dibheader[25], dibheader[24]);
		if (opti)
			printf("horizontal resolution (pixel per meter): %d\n",
					hex2dec(arr, 8));

		sprintf(arr, "%.2x%.2x%.2x%.2x", dibheader[31], dibheader[30],
				dibheader[29], dibheader[28]);
		if (opti)
			printf("vertical resolution (pizel per meter): %d\n",
					hex2dec(arr, 8));

		sprintf(arr, "%.2x%.2x%.2x%.2x", dibheader[35], dibheader[34],
				dibheader[33], dibheader[32]);
		numofcolor = hex2dec(arr, 8);
		if (opti)
			printf("number of colors:  %d\n", numofcolor);

		sprintf(arr, "%.2x%.2x%.2x%.2x", dibheader[39], dibheader[38],
				dibheader[37], dibheader[36]);
		if (opti)
			printf("number of important colors: %d\n", hex2dec(arr, 8));
	}
	//
	// Print Color Table If exists.
	//
	if (numofcolor == 0)
		printf("no color table\n");

	else {
		printf("\n");
		int colorTableOS = 0;

		if (compressionMethod == 3) {
			colorTableOS = 14 + dibsize + 12;
		} else {
			colorTableOS = 14 + dibsize;
		}

		colortable = (unsigned char *) malloc(
				sizeof(unsigned char) * (256 * 4));
		if (colortable == NULL) {
			err_quit("ERROR: Creating color table\n");
		}
		copybuf = (unsigned char *) memcpy(colortable, &buf[colorTableOS],
				(256 * 4));
		if (copybuf == NULL) {
			err_exit(1, "Error: memcpy failed\n");
		}

		if (optd) {
			printf("Color Table\nindex\tred\tgreen\tblue\talpha\n");
			printf("-------------------------------------\n");

			int row = 0;
			int startAd = 0;

			for (i = 0; i < numofcolor; i++) {
				printf("%d\t%d\t%d\t%d\t%d\n", row++,

						hexchar2dec(colortable[startAd + i]),
						hexchar2dec(colortable[startAd + i + 1]),
						hexchar2dec(colortable[startAd + i + 2]),
						hexchar2dec(colortable[startAd + i + 3]));
				startAd += 3;
			}
		}
	}

	if (optR && (bitperpixel==8)) {
		for (int i = 0; i < 255 * 4; i++)
			colortable[i] = 255 - colortable[i];
	}

	int rowsize = floor((bitperpixel * imagewidth + 31) / 32) * 4;
	int bytesperpixel = bitperpixel / 8;
	if (optd) {
		if (bytesperpixel == 3) {
			printf("\nPixel Data\n");
			printf("(row, col)\tred\tgreen\tblue\n");
			printf("------------------------------------\n");
		}

		if (bytesperpixel == 1) {
			printf("\nPixel Data\n");
			printf("(row, col)\tindex\n");
			printf("------------------------\n");
		}

		for (i = 1; i <= imageheight; i++) {
			int startaddress = startofbmp + rowsize * (imageheight - i);

			n = lseek(ifd, startaddress, SEEK_SET);
			nbytes = read_all(ifd, buf, rowsize);

			switch (bytesperpixel) {
			case 1:
				print1BytesPixelNoColorTable(buf, rowsize, i);
				break;
			case 2:
				break;
			case 3: // 3 bytes per pixel, i.e. 24 bits per
				if (numofcolor == 0) {
					print3BytesPixelNoColorTable(buf, rowsize, i);
				}
				break;
			case 4:
				break;
			default:
				break;
			}
		}
	}

	if (optR && (bitperpixel==8)) {
		struct iovec iov[3];
		iov[0].iov_base = fileheader;
		iov[0].iov_len = 14;
		iov[1].iov_base = dibheader;
		iov[1].iov_len = dibsize;
		iov[2].iov_base = colortable;
		iov[2].iov_len = 256 * 4;

		int iovcnt = sizeof(iov) / sizeof(struct iovec);

		ssize_t bytes_written = writev(ofd, iov, iovcnt);
		printf("%d: %d: %d, %d, %d\n", bytes_written, iovcnt, iov[0].iov_len,
				iov[1].iov_len, iov[2].iov_len);

		for (i = 1; i <= imageheight; i++) {
			int startaddress = startofbmp + rowsize * (imageheight - i);
			n = lseek(ifd, startaddress, SEEK_SET);
			lseek(ofd, startaddress, SEEK_SET);
			nbytes = read_all(ifd, buf, rowsize);
			write(ofd, buf, rowsize);
		}
	}

	if(((optr > -1) || (optg > -1) || (optb> -1)) && (bitperpixel == 24)){
		struct iovec iov[2];
				iov[0].iov_base = fileheader;
				iov[0].iov_len = 14;
				iov[1].iov_base = dibheader;
				iov[1].iov_len = dibsize;
				int iovcnt = sizeof(iov) / sizeof(struct iovec);
				ssize_t bytes_written = writev(ofd, iov, iovcnt);

		for (i = 1; i <= imageheight; i++) {
					int startaddress = startofbmp + rowsize * (imageheight - i);

					n = lseek(ifd, startaddress, SEEK_SET);
					lseek(ofd, startaddress, SEEK_SET);
					nbytes = read_all(ifd, buf, rowsize);
					changePixelColor(buf, rowsize, i, optr, optg, optb);
					write(ofd, buf, rowsize);
				}
	}


	// Clean up resources
	close(ofd);
	close(ifd);
	if (buf != NULL) {

		free(buf);
	}
	if (dibheader != NULL) {
		free(dibheader);

	}

	if (colortable != NULL) {
		free(colortable);

	}

	return 0;
}

/**
 * hex to decimal conversion
 * @param hexarr
 * @param bytes
 * @return the converted decimal
 */
int hex2dec(unsigned char *hexarr, int bytes) {

	int dec = 0;
	int i = 0;
	int base = 1;

	for (i = (bytes - 1); i >= 0; i--) {

		if (hexarr[i] >= '0' && hexarr[i] <= '9') {

			dec += base * (hexarr[i] - 48);
			base *= 16;

		} else if (hexarr[i] >= 'A' && hexarr[i] <= 'F') {

			dec += base * (hexarr[i] - 55);
			base *= 16;

		} else if (hexarr[i] >= 'a' && hexarr[i] <= 'f') {

			dec += base * (hexarr[i] - 87);
			base *= 16;

		}

	}

	return dec;
}

/**
 * changes hex to decimal value
 * @param hexchar
 * @return the converted decimal
 */
int hexchar2dec(unsigned char hexchar) {

	unsigned char arr[3];

	sprintf(arr, "%.2x", hexchar);
	return hex2dec(arr, 2);

}

/**
 * prints red, green, and blue value of each pixel
 * @param *buf
 * @param rowsize
 * @param row
 * @return nothing (void)
 */
void print3BytesPixelNoColorTable(unsigned char *buf, int rowsize, int row) {

	int count = 0;
	int column = 0;

	if (rowsize % 3 == 0) {

		while (count < rowsize) {

			printf("(%d, %d)\t\t%d\t%d\t%d\n", row - 1, column,
					hexchar2dec(buf[2 + count]), hexchar2dec(buf[1 + count]),
					hexchar2dec(buf[count]));
			column++;
			count += 3;
		}

	} else {
		printf("need algin\n");
	}

}

/**
 * changes the pixel color based on red, green, and blue color value
 * @param *buf
 * @param rowsize
 * @param row
 * @param red
 * @param green
 * @param blue
 * @return nothing (void)
 */
void changePixelColor(unsigned char *buf, int rowsize, int row, int red, int green, int blue) {

	int count = 0;
	int column = 0;

	if (rowsize % 3 == 0) {

		while (count < rowsize) {

			if(red != -1)
				buf[2 + count] = red;
			if(green != -1)
				buf[1 + count] = green;
			if(blue != -1)
				buf[count] = blue;

			column++;
			count += 3;
		}

	} else {
		printf("need align\n");
	}

}

/**
 * prints bitmap with using color table index
 * @param *buf
 * @param rowsize
 * @param row
 * @return nothing (void)
 */
void print1BytesPixelNoColorTable(unsigned char *buf, int rowsize, int row) {

	int count = 0;
	int column = 0;

	if (rowsize % 4 == 0) {
		while (count < rowsize) {
			printf("(%d, %d)\t\t%d\t%.2x\n", row - 1, column,
					hexchar2dec(buf[count]), buf[count]);
			column++;
			count += 1;
		}
	} else {
		printf("need align\n");
	}

}
