
all:
	gcc -o  test_bin -I./utils ./test/test.c ./utils/common.c  -lpthread

01_server:
	gcc -o  01_server -I./utils ./test/01_server.c ./utils/common.c  -lpthread

01_client:
	gcc -o  01_client -I./utils ./test/01_client.c ./utils/common.c  -lpthread
	
02_server:
	gcc -o  02_server -I./utils ./test/02_server.c ./utils/common.c  -lpthread