all: tmp/sshexec.o tmp/pass.o tmp/encoder.o
	rm -f sshexec
	gcc -g -o sshexec tmp/sshexec.o tmp/encoder.o -Llib -lssh2
	rm -f pass
	gcc -g -o pass tmp/pass.o tmp/encoder.o

tmp/sshexec.o: src/sshexec.c tmp/encoder.o
	rm -f tmp/sshexec.o
	gcc -g -c src/sshexec.c -o tmp/sshexec.o -Iinclude

tmp/pass.o: src/pass.c
	rm -f tmp/pass.o
	gcc -g -c src/pass.c -o tmp/pass.o
	
tmp/encoder.o: src/encoder.h src/encoder.c
	rm -f tmp/encoder.o
	gcc -g -c src/encoder.c -o tmp/encoder.o

clean: 
	rm -f tmp/*.o sshexec pass
