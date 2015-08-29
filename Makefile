all: driver get_fb_info_test

driver: driver.c
	$(CC) -g -o driver driver.c -lfuse

get_fb_info_test: get_fb_info_test.c
	$(CC) -g -o get_fb_info_test get_fb_info_test.c

