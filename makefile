AF_TEST:
	gcc -g -o ./AudioFormat/test/test -IAudioFormat ./AudioFormat/test/main.c
AF_CLEAN:
	rm ./AudioFormat/test/test