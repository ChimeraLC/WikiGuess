all:
	cc -I -std=gnu11 ws2_32.dll -Wall -Wextra -Werror -O2 server.c -o server -lmingw32
	cc -I -std=gnu11 ws2_32.dll -Wall -Wextra -Werror -O2 client.c -o client -lmingw32
