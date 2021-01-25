#include <stdint.h>



struct fileItem {
        const char *fileName;
        uint32_t fileSize;
};

struct mfs_file {
	const struct fileItem *fileItem;
	uint32_t offset;
	uint8_t * data;
};

int mfs_open(const char *fileName, struct mfs_file *file);
int mfs_read(struct mfs_file *file, uint8_t *buffer, int n);
