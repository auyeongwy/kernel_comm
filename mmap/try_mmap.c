/** @file try_mmap.c 
 *  Illustrates the mmap operation that uses mmap to write to a file "FILE".
 * 
 *  Usage:
 *  Edit the file "FILE" with some text. Make sure there is at least 10 chars inside or an error could occur.
 *  run try_mmap. The characrters "mama" will be written from the 5th position in the text file.
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>


#define FILE "FILE"


static void process_perror(const int p_err);


int main(int argc, char *argv[])
{
	int fd = -1;
	struct stat s_stat;
	unsigned char *buf = NULL;
	
	/* Opens the file. */
	if((fd = open(FILE, O_RDWR)) == -1) {
		process_perror(errno);
		goto do_exit;
	}
	
	/* Find size of the file. */
	if((fstat(fd, &s_stat)) == 0) {
		printf("File size: %zd bytes\n", s_stat.st_size);
		if(s_stat.st_size < 10) {
			printf("File too small for demo\n");
			goto do_exit;
		}
	} else {
		process_perror(errno);
		goto do_exit;	
	}
	
	
	/* Get pagesize. Not required in this example but done as an exercise. */
	printf("Pagesize: %ld\n", sysconf(_SC_PAGE_SIZE));
	
	
	/* Perform mmap. */
	buf = mmap(NULL, s_stat.st_size, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
	if((void*)buf == MAP_FAILED) {
		process_perror(errno);
		buf = NULL;
		goto do_exit;
	}

	/* Perform write */
	memcpy(buf+5, "mama", 4);
	
do_exit:
	if(buf != NULL) {
		if(munmap(buf, s_stat.st_size) == -1)
			process_perror(errno);
	}
	if(fd != -1)
		close(fd);
	return 0;
}



static void process_perror(const int p_err)
{
	printf("%s\n", strerror(p_err));
}
