#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <termios.h>

#define FB "/dev/fbe_buffer"

static int fb;
static struct fb_fix_screeninfo fix;
static struct fb_var_screeninfo var;

int main(int argc, char** argv)
{
	unsigned int *fp;
	int res;

	fb = open(FB, O_RDWR);
	if (fb == -1) {
		perror("Unable to open fb " FB);
		return 1;
	}
	res = ioctl(fb, FBIOGET_FSCREENINFO, &fix);
	if (res != 0) {
		perror("FSCREENINFO failed");
		close(fb);
		return 1;
	}


	res = ioctl(fb, FBIOGET_VSCREENINFO, &var);
	if (res != 0) {
		perror("VSCREENINFO failed");
		close(fb);
		return 1;
	}

	printf("size %dx%d @ %d bits per pixel\n", var.xres_virtual, var.yres_virtual, var.bits_per_pixel);
}

