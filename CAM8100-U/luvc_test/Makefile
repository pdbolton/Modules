CROSS=arm-none-linux-gnueabi-

all: luvc_test

luvc_test: luvc_test.c
	$(CROSS)gcc -O2 -o luvc_test luvc_test.c -ljpeg

clean:
	@rm -vf luvc_test *.o *~
