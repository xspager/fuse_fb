all: driver get_fb_info_test

driver: driver.c
	$(CC) -g -o driver driver.c -lfuse `sdl2-config --cflags --libs`

get_fb_info_test: get_fb_info_test.c
	$(CC) -g -o get_fb_info_test get_fb_info_test.c

build_docker_image:
	docker build -t fuse_fb_build_image .

docker_shell:
	docker run -v.:/root/code -it fuse_fb_build_image

build_inside_docker:
	docker run -v.:/root/code -it fuse_fb_build_image make
