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
	int fileout = 0;
	int key = 0;
	pid_t pid;
	int oflags = O_RDWR | O_CREAT | O_TRUNC;

	if (argc != 7) {
		strcpy(str, "Wrong number of arguments\n");
		strcat(str, "Usage: <prog_src> <file_src> <file_out> ");
		strcat(str, "<prog_key> <file_key> <file_key_out>\n");
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
		i = execlp(argv[1], argv[1], argv[2], NULL);
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

	pid = fork();
	if (pid == -1) {
		strcpy(str, "Fork error\n");
		write(2, str, strlen(str));
		exit(1);
	} else if (pid == 0) {
		close(1);
		dup(pipes[KEYFILE][1]);
		close(pipes[KEYFILE][1]);
		i = execlp(argv[4], argv[4], argv[5], NULL);
		if (i == -1) {
			strcpy(str, "Exec error\n");
			write(2, str, strlen(str));
			exit(2);
		}
	}


	fileout = open(argv[3], oflags, S_IWUSR | S_IRUSR);
	key = open(argv[6], oflags, S_IWUSR | S_IRUSR);

	if (fileout == -1 || key == -1) {
		strcpy(str, "File open error\n");
		write(2, str, strlen(str));
		exit(3);
	}
	while (nread = read(0, buff, LENGTH)) {
		uchar crypted;
		uchar randbuff[LENGTH];
		uchar randbyte;
		int nreadkey;

		nreadkey = read(pipes[KEYFILE][0], randbuff, nread);
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
	return 0;
}
