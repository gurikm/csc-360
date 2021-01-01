

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

char * data;
int found_filesize = 0;

/*
 *Returns the value of the fat.
 */
int value_of_fat(int index, char* data){
    int value;
   if(index % 2 == 0){
        int low = data[512 + 1+(3*index/2)] & 0x0F;
        value = (low << 8) + (data[512 + (3*index/2)] & 0xFF); 
    } else{
        int high = data[512 + (3*index/2)] & 0xF0;
        value = (high >> 4) + ((data[512 + 1+(3*index/2)] & 0xFF) << 4);
    }
    return value;
}
/*
 *List is used to check if the file exists
 *returns the cluster number if the file is found
 *else it will return 0;
 */
int list(char *file, char *copyfile_name){
 if(file[0] == 0x00){
    return 0;
  }
  int curname_length = 0;
  char curr_fname[10];
  for(int i=0; i<8; i++){
     if(file[i] == ' '){
      curr_fname[i] = '\0';
      break;
    }
    curr_fname[i] = file[i];
    curname_length++;
  }
  int cluster_number = (file[26] & 0xFF) + (file[27] << 8);
 if(strncmp(curr_fname, copyfile_name, strlen(copyfile_name)) == 0){
    found_filesize = (file[28] & 0xFF) + ((file[29] & 0xFF) << 8) + ((file[30] & 0xFF) << 16) + ((file[31] & 0xFF) << 24);
    return cluster_number;
  }else{
    file = file + 32;
    return list(file, copyfile_name);
  }
}
/*
 *copies over the contents of the IMA file to 
 *new file.
 */
void copy_to_newfile(char * data, char * updated_data, int sector){
  int new_size = found_filesize;
  int counter;
  while(value_of_fat(sector, data) != 0xFFF){
    if(new_size == found_filesize){
      counter = (31 + sector) * 512;
    }else{
      sector = value_of_fat(sector, data);
      counter = (31 + sector) * 512;
    }
    for(int i=0; i<512; i++){
      if(new_size == 0) return;
      updated_data[found_filesize - new_size] = data[i + counter];
      new_size--;
    }
  }
}


/*
 *USed to open the IMA file disk image.
 */
char* disk_image(struct stat status,char *items,int file){
    file = open(items, O_RDWR);
    fstat(file,&status);
    data = mmap(NULL, status.st_size, PROT_READ, MAP_SHARED, file, 0);
    close(file);
    return data;
}
/*
 *converts name of the file to upper case 
 *in order to find the file type by user.
 */
void convert_to_uppercase(char* filename,char* file){
 int name_length = 0;
for(int i=0; file[i] != '\0'; i++){
      filename[name_length] = toupper(file[i]); //convert to upper
      name_length++;
  }
  filename[name_length] = '\0';

}
 
/************************************************************Main Function***************************************************************/
int main(int argc, char *argv[]){
  int fd;
  struct stat sb;
  if(fd < 0){
    perror("ERROR: Failed to open file image.\n");
    exit(1);
  }
  data = disk_image(sb,argv[1],fd);
  char *file = argv[2];
  char filename[10];
  int name_length = 0;
  convert_to_uppercase(filename,file);
  for(int i=0; file[i] != '\0'; i++){
    if(file[i] == '.'){
      filename[name_length] = '\0';
      break;
    }
      filename[name_length] = filename[i];
      name_length++;
    }

  data = data + 512 * 19;
  int find = list(data,filename);
  data = data - 512 * 19; 
  if(find == 0){
   perror("File not found.\n");
    exit(0);
  }else{
  int new_file = open(file, O_RDWR | O_CREAT, 0644);
  int bus_error = lseek(new_file,found_filesize-1, SEEK_SET);
  bus_error = write(new_file, "", 1);
  char * new_map = mmap(NULL, found_filesize, PROT_WRITE, MAP_SHARED, new_file, 0);
  copy_to_newfile(data, new_map, find);
}
  munmap(data, sb.st_size);
  return 0;
}
