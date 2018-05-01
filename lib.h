
typedef struct MemoryRegion {
	void *startAddr;
	void *endAddr;
	size_t size;
	int isReadable;
	int isWriteable;
	int isExecutable;
} MR;

int ckpt_getline(int fd, char buffer[], int len);
int indexOf(char *line, char c);

int
ckpt_getline(int fd, char buffer[], int len) 
{	
	int i = 0;
	while (i < len) {
		if (buffer[i] == '\0') {
			break;
		}
		buffer[i++] = '\0';
	}

	char c;	
	if (read(fd, &c, 1) == -1) {
		printf("Error: Cannot read line, %s", strerror(errno));
		return -1;
	}
	int size = 0;
	while (c != '\0' && c!='\n') {
		buffer[size++] = c;
		if(read(fd, &c, 1) == -1) {
			printf("Error: Cannot read line, %s", strerror(errno));
			return -1;
		};

	}
	buffer[size] = c;
	return size;
}

int 
indexOf(char *line, char c) 
{	
	int i = 0;
	while (i < strlen(line)) {
		if (line[i] == c) {
			return i;
		}
		i++;
	}
	return -1;
}