#include "mfs.h"
#include "mfs_data.h"
#include <string.h>

//při nalezeném souboru vrací velikost, jinak -1
int mfs_open(const char *fileName, struct mfs_file *file)
{
	int i = 0, offset = 0,found = 0;

	while(fileSystem[i].fileSize != 0)
	{
		if(strcmp(fileSystem[i].fileName, fileName)==0)
		{
			//printk("nalezen index %d, offset %d\n",i,offset);
			found = 1;
			break;
		}
		else
		{
			offset += fileSystem[i].fileSize;
		}

		i++;
	}

	if(found == 1)
	{
		file->fileItem = &fileSystem[i];
		file->offset = 0;
		file->data = (uint8_t *)((int)fsData + offset);
		return fileSystem[i].fileSize;
	}
	else
	{
		return -1;
	}

}


int mfs_read(struct mfs_file *file, uint8_t *buffer, int n)
{
	if((file->fileItem->fileSize - file->offset) >= n)
	{
		memcpy(buffer,file->data + file->offset, n);
		file->offset = file->offset + n;
		return n;
	}
	else
	{
		return 0;
	}
}
