all: PICOFoxweb proxy

clean:
	@rm -rf *.o 
	@rm -rf PICOFoxweb 
	@rm -rf proxy

PICOFoxweb: main.o httpd.o
	gcc -o PICOFoxweb main.o httpd.o

main.o: main.c httpd.h
	gcc -c -o main.o main.c

httpd.o: httpd.c httpd.h
	gcc -c -o httpd.o httpd.c

sqli_filter.o: sqli_filter.c sqli_filter.h
	gcc -c -o sqli_filter.o sqli_filter.c

proxy: proxy.o sqli_filter.o
	gcc -o proxy proxy.o sqli_filter.o

proxy.o: proxy.c sqli_filter.h
	gcc -c -o proxy.o proxy.c