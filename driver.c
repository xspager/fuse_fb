#define FUSE_USE_VERSION 30
#define _FILE_OFFSET_BITS 64
#include <fuse/cuse_lowlevel.h>
#include <fuse/fuse_opt.h>

#include <linux/fb.h>

#include <stdio.h>
#include <string.h>

#define LOG(...) do { fprintf(stderr, __VA_ARGS__); puts(""); } while (0)

static void
cusetest_open(fuse_req_t req, struct fuse_file_info *fi)
{
    LOG("open");
    fuse_reply_open(req, fi);
}

static void
cusetest_read(fuse_req_t req, size_t size, off_t off,
	      struct fuse_file_info *fi)
{
    LOG("read");
    fuse_reply_buf(req, "Hello", size > 5 ? 5 : size);
}

static void
cusetest_write(fuse_req_t req, const char *buf, size_t size,
	       off_t off, struct fuse_file_info *fi)
{
    LOG("write (%zu bytes)", size);
    fuse_reply_write(req, size);
}

static void
cusetest_ioctl(fuse_req_t req, int cmd, void *arg,
	       struct fuse_file_info *fi, unsigned flags,
	       const void *in_buf, size_t in_bufsz, size_t out_bufsz)
{

    LOG("ioctl %d: insize: %zu outsize: %zu", cmd, in_bufsz, out_bufsz);
    switch (cmd) {
    case FBIOGET_FSCREENINFO:
	LOG("Yay!");
	fuse_reply_ioctl(req, 0, NULL, 0);
	break;
    case FBIOGET_VSCREENINFO:
	if (in_bufsz == 0){
		struct iovec iov = {arg, sizeof(struct fb_var_screeninfo)};
		fuse_reply_ioctl_retry(req, &iov, 1, &iov, 1);
	} else { 
    		struct fb_var_screeninfo fb_var;
		fb_var.xres_virtual = 800;
    		fb_var.yres_virtual = 640;
    		fb_var.bits_per_pixel = 1;
		fuse_reply_ioctl(req, 0, &fb_var, sizeof(struct fb_var_screeninfo));
	}
	break;
    case 23:
	if (in_bufsz == 0) {
	    struct iovec    iov = { arg, sizeof(int) };
	    fuse_reply_ioctl_retry(req, &iov, 1, NULL, 0);
	} else {
	    LOG("  got value: %d", *((int *) in_buf));
	    fuse_reply_ioctl(req, 0, NULL, 0);
	}
	break;
    case 42:
	if (out_bufsz == 0) {
	    struct iovec    iov = { arg, sizeof(int) };
	    fuse_reply_ioctl_retry(req, NULL, 0, &iov, 1);
	} else {
	    LOG("  write back value");
	    int             v = 42;
	    fuse_reply_ioctl(req, 0, &v, sizeof(int));
	}
	break;
    }
}

static const struct cuse_lowlevel_ops cusetest_clop = {
    .open = cusetest_open,
    .read = cusetest_read,
    .write = cusetest_write,
    .ioctl = cusetest_ioctl,
};

int
main(int argc, char **argv)
{
    // -f: run in foreground, -d: debug ouput
    // Compile official example and use -h
    const char     *cusearg[] = { "test", "-f" };	// -d for debug -s for single thread
    const char     *devarg[] = { "DEVNAME=fb_fuse_buffer" };
    struct cuse_info ci;
    memset(&ci, 0x00, sizeof(ci));
    ci.flags = CUSE_UNRESTRICTED_IOCTL;
    ci.dev_info_argc = 1;
    ci.dev_info_argv = devarg;

    return cuse_lowlevel_main(2, (char **) &cusearg, &ci, &cusetest_clop,
			      NULL);
}
