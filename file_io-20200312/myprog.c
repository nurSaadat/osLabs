#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int insert_at(int fd, off_t offset, char *s, int n)
{
	// count how many bites we need to allocate
	off_t end_of_file = lseek(fd, 0, SEEK_END);
	printf("End of file %ld. \n", end_of_file);
	
	off_t start_of_replace = lseek(fd, offset, SEEK_SET);
	off_t store_buf_size = end_of_file - start_of_replace;

	// create buffer to save string from offset to the end of the file
	char *save_buf = (char*) malloc(store_buf_size);

	// get save string
	lseek(fd, offset, SEEK_SET);
	read(fd, save_buf, store_buf_size);
	
	// write insert string
	lseek(fd, offset, SEEK_SET);
	write(fd, s, n);

	// insert string from save_buf
	write(fd, save_buf, store_buf_size);
}

int main(int argc, char *argv[])
{
	// arguments validation
	if (argc != 4)
	{
		printf("ERROR! Please write in form \"insert dst_f offset string\".\n");
		return -1;
	}
		
	// open source file
	int dstf = open(argv[1], O_RDWR);

	if (dstf == -1)
	{
		printf("%s file cannot be opened.\n", argv[1]);
		return -1;
	}

	printf("File descriptor is %d.\n", dstf);

	// insert in destination file
	insert_at(dstf, atoi(argv[2]), argv[3], strlen(argv[3]));
	close(dstf);
	return 0;

}

