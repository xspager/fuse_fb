# FuseFB

__FuseFB__ uses [FUSE](https://en.wikipedia.org/wiki/Filesystem_in_Userspace) low level block API __CUSE__ to simulate a [Linux Framebuffer](https://en.wikipedia.org/wiki/Linux_framebuffer) (like /dev/fb0)

Fisrt thing you need to make sure you have is the kernel module cuse, you can check running lsmod | grep cuse

For now you can start the "Driver" and use the test program 'get_fb_info_test' like this:

````bash
$ make
$ sudo ./driver &
$ sudo ./get_fb_info_test /dev/fb_fuse_buffer
````

The same program also works with the native framebuffer:

````bash
$ sudo ./get_fb_info_test /dev/fb0
````
