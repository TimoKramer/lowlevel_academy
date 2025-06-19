#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

struct database_header_t {
	unsigned short version;
	unsigned short employees;
	unsigned int filesize;
	unsigned int uid;
};

int main(int argc, char *argv[]) {
	struct database_header_t head = {0};
	struct stat dbStat = {0};

	if (argc != 2) {
		printf("Usage: %s <filename>\n", argv[0]);
		return 0;
	}

	int fd = open(argv[1], O_RDWR | O_CREAT, 0644);
	if (fd == -1) {
		perror("open");
		return -1;
	}

	if (read(fd, &head, sizeof(head)) != sizeof(head)) {
		perror("read");
		close(fd);
		return -1;
	}

	printf("DB Version: %u\n", head.version);
	printf("DB Number of Employees: %u\n", head.employees);
	printf("DB File Length: %u\n", head.filesize);
	printf("DB UID: %u\n", head.uid);

	if (fstat(fd, &dbStat) < 0) {
		perror("stat");
		close(fd);
		return -1;
	}

	close(fd);

	printf("DB File Length, reportet by stat: %ld\n", dbStat.st_size);
	printf("DB File Length, reportet by stat: %u\n", dbStat.st_uid);

	if (dbStat.st_size != head.filesize | dbStat.st_uid != head.uid) {
		perror("DB ERROR");
		return -1;
	}

	return 0;
}
