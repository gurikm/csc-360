
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>
#include <ctype.h>

char *data;

/*
 *Functions returns the label of the disk image.
 */
void* get_label(char *data, char *label){
	for(int i = 0; i < 8; i++){
		label[i] = data[i+43];
	}
	label[11] = '\0';
	if(label[0] == ' '){
        data += 512 * 19;
        while (data[0] != 0x00) {
            if (data[11] == 8) {
                for (int i = 0; i < 8; i++) {
                    label[i] = data[i];
                }
            }
            data += 32;
        }
	}
}

/*
 *Returns the free size of the 
 *disk image.
 */
int get_free_size(int totalSectorCount,char *data){
    int free_size = 0;
    int result;
    for(int i = 2; i < (totalSectorCount-31); i++){
        if ((i % 2) == 0) {
        result = ((data[512 + ((3*i) / 2) + 1] & 0x0F) << 8) + data[512 + ((3*i) / 2)] & 0xFF;
    } else {
        result = ((data[512 + (int)((3*i) / 2)] & 0xF0) >> 4) + ((data[512 + (int)((3*i) / 2) + 1] & 0xFF) << 4);
    }
        if(result == 0x00){
            free_size++;
        }
    }
    return free_size *512;
}

/*
 *Returns the number of root and sub directory
 * files in the IMA disk image.
 */
int get_num_files(char *my_data){
	int count = 0;
  if(my_data[0] == 0x00){
    return count;
  }
  int start = my_data[26] + (my_data[27] << 8);
  if(start == 1 || start == 0 || my_data[11] == 0x0f || (my_data[11] & 0x08) == 0x08 || (my_data[0] & 0xFF) == 0xE5){
    my_data = my_data + 32;
    return count + get_num_files(my_data);
  //determine if it is a subdirectory
  }else if((my_data[11] & 0x10) == 0x10){
    //Finds to location of the sudirectory
    int subdirectory_locations = 512 * (start + 31) + 64; 
    char *subdirectory = data - 512 * 19 + subdirectory_locations;
    my_data = my_data + 32;
    //below goes to the end of directories
    return count + get_num_files(subdirectory) + get_num_files(my_data);
  }else{
    count++;
    my_data = my_data + 32;
    return count + get_num_files(my_data);
  }
}
//*********************************************MAIN FUNCTION*****************************************************/
int main(int argc, char *argv[]) {
    int fd = open(argv[1], O_RDWR);
    struct stat sb; 
	if(argv[1] == NULL){
        printf("Error: Did not enter disk image.\n");
        exit(1);
    }
    data = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (data == MAP_FAILED) {
        perror("Error: failed to map memory\n");
        exit(1);
    }
    //Items below are used to get the info of the IMA file.
	char* os_name = malloc(sizeof(char));
    //used to get the name of the OS.
    for(int i = 0; i < 8; i++){
        os_name[i] = data[i+3];
	}
    int bytesPerSector = data[11] + (data[12] << 8);
    int totalSectorCount = data[19] + (data[20] << 8);
	int total_size = bytesPerSector * totalSectorCount;
    char* label = malloc(sizeof(char));
	get_label(data, label);
    data = data + 512 * 19;
    int num_files = get_num_files(data);
    data = data - 512 * 19;
	char num_fats = data[16];
    int sectors = data[22] + (data[23] << 8);
    int free_size = get_free_size(totalSectorCount, data);

	printf("OS Name: %s\n", os_name);
	printf("Label of the disk: %s\n", label);
	printf("Total size of the disk: %d bytes\n", total_size);
	printf("Free size of the disk: %d bytes\n", free_size);
	printf("==============\n");
	printf("The number of files in the disk (including all files in the root directory and files in all subdirectories) %d\n\n", num_files);
	printf("=============\n");
	printf("Number of FAT copies: %d\n", num_fats);
	printf("Sectors per FAT: %d\n", sectors);
    
	munmap(data, sb.st_size);
	return 0;
}