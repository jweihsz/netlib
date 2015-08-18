
all:
	gcc -o  test_bin -I./utils ./test/test.c ./utils/common.c  -lpthread