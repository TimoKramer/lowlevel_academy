#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "common.h"
#include "parse.h"

int add_employee(struct dbheader_t *dbhdr, struct employee_t **employees, char *addstring) {

    if (NULL == dbhdr) return STATUS_ERROR;
    if (NULL == employees) return STATUS_ERROR;
    if (NULL == *employees) return STATUS_ERROR;
    if (NULL == addstring) return STATUS_ERROR;

    char *name = strtok(addstring, ",");
    if (NULL == name) return STATUS_ERROR;

    char *address = strtok(NULL, ",");
    if (NULL == address) return STATUS_ERROR;

    char *hours = strtok(NULL, ",");
    if (NULL == hours) return STATUS_ERROR;

    struct employee_t *e = *employees;
    struct employee_t *temp = realloc(e, sizeof(struct employee_t) * (dbhdr->count + 1));
    if (temp == NULL) {
        printf("Realloc failed\n");
        return STATUS_ERROR;
    }
    e = temp;

    dbhdr->count++;

    strncpy(e[dbhdr->count-1].name, name, sizeof(e[dbhdr->count-1].name)-1);
    e[dbhdr->count-1].name[sizeof(e[dbhdr->count-1].name)-1] = '\0';
    
    strncpy(e[dbhdr->count-1].address, address, sizeof(e[dbhdr->count-1].address)-1);
    e[dbhdr->count-1].address[sizeof(e[dbhdr->count-1].address)-1] = '\0';
    
    e[dbhdr->count-1].hours = atoi(hours);

    *employees = e;

    return STATUS_SUCCESS;
}

int list_employees(struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (dbhdr == NULL) return STATUS_ERROR;
    if (employees == NULL) return STATUS_ERROR;

    int i = 0;
    for (; i < dbhdr->count; i++) {
        printf("Employee %d\n", i);
        printf("\tName: %s\n", employees[i].name);
        printf("\tAddress: %s\n", employees[i].address);
        printf("\tHours: %d\n", employees[i].hours);
    }
    return STATUS_SUCCESS;
}

int read_employees(int fd, struct dbheader_t *dbhdr, struct employee_t **employeesOut) {
    if (fd < 0) {
        printf("Got a bad FD from the user\n");
    }

    int count = dbhdr->count;

    struct employee_t *employees = calloc(count, sizeof(struct employee_t));
    if (employees == NULL) {
        printf("Malloc failed\n");
        return STATUS_ERROR;
    }

    ssize_t bytes_read = read(fd, employees, count*sizeof(struct employee_t));
    if (bytes_read != count*sizeof(struct employee_t)) {
        printf("Failed to read employee data\n");
        free(employees);
        return STATUS_ERROR;
    }

    int i = 0;
    for (; i < count; i++) {
        employees[i].hours = ntohl(employees[i].hours);
    }

    *employeesOut = employees;
    return STATUS_SUCCESS;
}

int output_file(int fd, struct dbheader_t *dbhdr, struct employee_t *employees) {
    if (fd < 0) {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    int realcount = dbhdr->count;

    dbhdr->magic = htonl(dbhdr->magic);
    dbhdr->filesize = htonl(sizeof(struct dbheader_t) + (sizeof(struct employee_t) * realcount));
    dbhdr->count = htons(dbhdr->count);
    dbhdr->version = htons(dbhdr->version);

    if (lseek(fd, 0, SEEK_SET) == -1) {
        printf("Failed to seek in file\n");
        return STATUS_ERROR;
    }

    if (write(fd, dbhdr, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        printf("Failed to write database header\n");
        return STATUS_ERROR;
    }

    int i = 0;
    for (; i < realcount; i++) {
        employees[i].hours = htonl(employees[i].hours);
        if (write(fd, &employees[i], sizeof(struct employee_t)) != sizeof(struct employee_t)) {
            printf("Failed to write employee %d\n", i);
            return STATUS_ERROR;
        }
    }

    return STATUS_SUCCESS;
}

int validate_db_header(int fd, struct dbheader_t **headerOut) {
    if (fd < 0) {
        printf("Got a bad FD from the user\n");
        return STATUS_ERROR;
    }

    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        printf("Malloc failed to create a db header\n");
        return STATUS_ERROR;
    }

    if (read(fd, header, sizeof(struct dbheader_t)) != sizeof(struct dbheader_t)) {
        perror("read");
        free(header);
        return STATUS_ERROR;
    }

    header->version = ntohs(header->version);
    header->count = ntohs(header->count);
    header->magic = ntohl(header->magic);
    header->filesize = ntohl(header->filesize);

    if (header->magic != HEADER_MAGIC) {
        printf("Improper header magic\n");
        free(header);
        return -1;
    }

    if (header->version != 1) {
        printf("Improper header version\n");
        free(header);
        return -1;
    }

    struct stat dbstat = {0};
    fstat(fd, &dbstat);
    if (header->filesize != dbstat.st_size) {
        printf("Corrupted database\n");
        free(header);
        return -1;
    }

    *headerOut = header;
    return STATUS_SUCCESS;
}

int create_db_header(struct dbheader_t **headerOut) {
    if (headerOut == NULL) {
        printf("Invalid db header\n");
        return STATUS_ERROR;
    }
    struct dbheader_t *header = calloc(1, sizeof(struct dbheader_t));
    if (header == NULL) {
        printf("Malloc failed to create db header\n");
        return STATUS_ERROR;
    }
    header->version = 0x1;
    header->count = 0;
    header->magic = HEADER_MAGIC;
    header->filesize = sizeof(struct dbheader_t);

    *headerOut = header;

    return STATUS_SUCCESS;
}
