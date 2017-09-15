/** @file say_hello.c
 * Simple test application to read and write to the character device /dev/khello. Used for testing with the khello kernel module.
 * 
 * Usage:
 * After loading the kernel module.
 * To read from the device: "./say_hello read"
 * To write to the device: ./say_hello write something"
 * 
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>


#define DEVICE "/dev/khello" /**< The character device. */


/** Check the arguments on the command line.
 *  @param p_num Number of arguments
 *  @param p_args List of arguments
 *  @return 0 if arguments are OK. Else -1.
 */
static int check_args(const int p_num, char *p_args[]);

/** Process the value set in erno.
 *  @param The errno itself.
 */
static void process_errnum(const int p_errnum);

/** Perform a read operation on the device.
  */
static void do_read();

/** Performs a write operation on the device.
 *  @param p_msg Message to write. Only the 1st 32 bytes will be writen to the device. The device also replaces the last byte in its 32-byte buffer with a '0'.
  */

static void do_write(const char *const p_msg);



int main(int argc, char *argv[])
{
	int operation; /* Operation to execute. 1=read. 2=write. */

	/* Check input arguments and decide on operation to carry out. */
	if((operation = check_args(argc, argv)) == -1) {
		printf("Incorrect args. Abort.\n");
		return 0;
	}
	
	
	switch(operation) {
		case 1:
 			do_read();
			break;
		case 2:
			do_write(argv[2]);
			break;
		default:
			break;
	}
	
	return 0;
}



static int check_args(const int p_num, char *p_args[])
{
	if(p_num < 2) 
		return -1;
	
	if(strcmp(p_args[1], "read")==0)
		return 1;
	
	if((strcmp(p_args[1], "write")==0) && (p_num >= 3))
		return 2;
	
	return -1;
}



static void do_read()
{
	int count, fd = -1;
	unsigned char buf[32];
	
	if((fd = open(DEVICE, O_RDONLY)) == -1) {
		process_errnum(errno);
		return;
	}
	if((count = read(fd, buf, 32)) != -1) {
		buf[count -1] = 0;
		printf("READ from %s: %s\n", DEVICE, buf);
	} else
		process_errnum(errno);
	
	close(fd);
}



static void do_write(const char *const p_msg)
{
	int count, fd = -1;
	
	if((count = strlen(p_msg)) > 32)
		count = 32;
	
	if((fd = open(DEVICE, O_WRONLY)) == -1) {
		process_errnum(errno);
		return;
	}
	if(write(fd, p_msg, count) != -1)
		printf("WRITE to %s: %s\n", DEVICE, p_msg);
	else 
		process_errnum(errno);
	
	
	close(fd);
}



static void process_errnum(const int p_errnum)
{
	printf("%s\n", strerror(p_errnum));
}
