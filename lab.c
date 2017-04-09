#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/types.h>

#define LENGTH 128
#define uchar unsigned char
#define INPUTFILE 0
#define KEYFILE 1

int main(int argc, char *argv[])
{
	int nread = 0;
	char str[LENGTH];
	char buff[LENGTH];
	int pipes[2][2];
	int i = 0;
	int fileout;
	int key;
	pid_t pid;
	int oflags = O_RDWR | O_CREAT | O_TRUNC;

	if (argc < 3 || argc > 3) {
		strcpy(str, "Wrong number of arguments\n");
		strcat(str, "Usage: crypt/encrypt <file>\n");
		write(2, str, strlen(str));
		exit(1);
	}
	i = pipe(pipes[INPUTFILE]);
	if (i == 0)
		i = pipe(pipes[KEYFILE]);
	if (i != 0) {
		strcpy(str, "Piping error\n");
		write(2, str, strlen(str));
		exit(1);
	}

	pid = fork();
	if (pid == -1) {
		strcpy(str, "Fork error\n");
		write(2, str, strlen(str));
		exit(1);
	} else if (pid == 0) {
		close(1);
		dup(pipes[INPUTFILE][1]);
		close(pipes[INPUTFILE][0]);
		close(pipes[INPUTFILE][1]);
		i = execlp("cat", "cat", argv[2], NULL);
		if (i == -1) {
			strcpy(str, "Exec error\n");
			write(2, str, strlen(str));
			exit(2);
		}
	}
	close(0);
	dup(pipes[INPUTFILE][0]);
	close(pipes[INPUTFILE][0]);
	close(pipes[INPUTFILE][1]);
	if (strcmp(argv[1], "crypt") == 0) {
		int entropy;

		fileout = open("fileout", oflags, S_IWUSR | S_IRUSR);
		key = open("key", oflags, S_IWUSR | S_IRUSR);
		entropy = open("/dev/urandom", O_RDONLY);
		if (fileout == -1 || key == -1 || entropy == -1) {
			strcpy(str, "File open error\n");
			write(2, str, strlen(str));
			exit(3);
		}
		while (nread = read(0, buff, LENGTH)) {
			uchar crypted;
			uchar randbuff[LENGTH];
			uchar randbyte;
			int nreadkey;

			nreadkey = read(entropy, randbuff, nread);
			if (nreadkey < nread) {
				strcpy(str, "Random read error\n");
				write(2, str, strlen(str));
				exit(4);
			}
			for (i = 0; i < nread; i++) {
				randbyte = randbuff[i] & 0xFF;
				crypted = buff[i] ^ randbyte;
				write(fileout, &crypted, 1); 
				write(key, &randbyte, 1);
			}
		}
		close(fileout);
		close(key);
		close(entropy);
	} else if (strcmp(argv[1], "encrypt") == 0) {
		key = open("key", O_RDONLY);
		if (key == -1) {
			strcpy(str, "key missing error\n");
			write(2, str, strlen(str));
			exit(3);
		}
		close(key);

		pid = fork();
		if (pid == -1) {
			strcpy(str, "Fork error\n");
			write(2, str, strlen(str));
			exit(1);
		} else if (pid == 0) {
			close(1);
			dup(pipes[KEYFILE][1]);
			close(pipes[KEYFILE][1]);
			i = execlp("cat", "cat", "key", NULL);
			if (i == -1) {
				strcpy(str, "Exec error\n");
				write(2, str, strlen(str));
				exit(2);
			}
		}

		fileout = open("filesrc", oflags, S_IRUSR | S_IWUSR);
		if (fileout == -1) {
			strcpy(str, "File open error\n");
			write(2, str, strlen(str));
			exit(3);
		}
		while (nread = read(0, buff, LENGTH)) {
			uchar encrypted;
			uchar randbuff[LENGTH];
			int nreadkey;

			nreadkey = read(pipes[KEYFILE][0], randbuff, nread);
			if (nreadkey < nread) {
				strcpy(str, "Key read error\n");
				write(2, str, strlen(str));
				exit(4);
			}
			for (i = 0; i < nread; i++) {
				encrypted = buff[i] ^ randbuff[i];
				write(fileout, &encrypted, 1);
			}
		}
		close(fileout);
	}
	return 0;
}
