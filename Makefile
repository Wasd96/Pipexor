all:
	gcc lab.c -o olab4
	gcc lab.c -o olab4san -fsanitize=address
	cppcheck --enable=all --inconclusive --std=posix lab.c
	/usr/src/linux-source-4.4.0/scripts/checkpatch.pl -f lab.c
