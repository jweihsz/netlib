
all:
	gcc -o  test_bin -I./utils ./test/test.c ./utils/common.c  -lpthread

01_server:
	gcc -o  01_tcp_server -I./utils ./test/01_tcp_server.c ./utils/common.c  -lpthread

01_client:
	gcc -o  01_tcp_client -I./utils ./test/01_tcp_client.c ./utils/common.c  -lpthread
	
02_server:
	gcc -o  02_tcp_server -I./utils ./test/02_tcp_server.c ./utils/common.c  -lpthread

01_udp_cli:
	gcc -o  01_udp_client -I./utils ./test/01_udp_client.c ./utils/common.c  -lpthread

01_udp_server:
	gcc -o  01_udp_server -I./utils ./test/01_udp_server.c ./utils/common.c  -lpthread