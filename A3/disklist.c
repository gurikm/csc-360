
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <errno.h>

int sub_dir = 0;
char *data;
#define timeOffset 14 //offset of creation time in directory entry
#define dateOffset 16 //offset of creation date in directory entry

/*
 *returns the creation date and time of a specific file.
 */
void print_date_time(char * directory_entry_startPos){
    
    int time, date;
    int hours, minutes, day, month, year;
    
    time = *(unsigned short *)(directory_entry_startPos + timeOffset);
    date = *(unsigned short *)(directory_entry_startPos + dateOffset);
    
    //the year is stored as a value since 1980
    //the year is stored in the high seven bits
    year = ((date & 0xFE00) >> 9) + 1980;
    //the month is stored in the middle four bits
    month = (date & 0x1E0) >> 5;
    //the day is stored in the low five bits
    day = (date & 0x1F);
    
    printf("%d-%02d-%02d ", year, month, day);
    //the hours are stored in the high five bits
    hours = (time & 0xF800) >> 11;
    //the minutes are stored in the middle 6 bits
    minutes = (time & 0x7E0) >> 5;
    
    printf("%02d:%02d\n", hours, minutes);
    
    return ;    
}
/*
 *list is used to output the root directory
 *and all of its subdirectories form the IMA.
 */
int list(char*file,char* root,int dir,int pos){
    int i = 0;
     if(file[0] == 0x00){
    return 0;
  }
    if(dir > sub_dir){
    printf("%s\n==================\n",root);
    sub_dir++;
    }
    char filename[15];
    char file_type;
    int cluster = (file[26] & 0xFF) + (file[27] << 8);
  if(cluster == 1 || cluster == 0){
    file = file + 32;
    return list(file,root,dir,pos);
}else{
    while(pos < 33*512){
            if((file[11] & 0x10) == 0x10){
            file_type = 'D';
        } else{
            file_type = 'F';
        }
        int file_size = (file[28] & 0xFF) + ((file[29] & 0xFF) << 8) + ((file[30] & 0xFF) << 16) + ((file[31] & 0xFF) << 24);
        int n_size = 0;
        int size=0;
        //outputs the name of the file or directory.
        for(i = 0; i < 8; i++){
            if(file[i] == ' '){
                continue;
            }
            filename[i] = file[i];
            n_size++;
            size++;
        }
        if(file_type == 'F'){
        filename[n_size] = '.';
        n_size++;
        size++;
        for(i=0;i<3;i++){
            if(file[i+8] == ' '){
                break;
            }
            filename[i+n_size] = file[i+8];
            size++;
        }
    }
       filename[size] = '\0';
        if(file[11] == 0x0F || filename[0] == 0x00 || filename[0] == 0xE5 || (file[11] & 0x08) == 0x08){
            pos += 32;
            continue;
        }
        //if the file is a directory it enters the if statement and goes until it find the files.
        if(file_type == 'D'){
            int sub_directory_pos = 512 * (cluster + 31)+64;
            char *sub_dir_status = data - 512 * 19 + sub_directory_pos;
            int sub_directory_number = dir + 1;
            printf("%c %10d %20s ", file_type, file_size, filename);
            print_date_time(&file[pos]);
            pos += 32;
            file = file + 32;
            return list(file,root,dir,pos) + list(sub_dir_status,filename,sub_directory_number,pos);
        }
        else{
        printf("%c %10d %20s ", file_type, file_size, filename);
        print_date_time(&file[pos]);
        pos += 32;
        file = file + 32;
        return list(file,root,dir,pos);
            }
        }
    }
    return 0;
}

/*
 *Returns the disk image after opening the IMA file.
 */
char* disk_image(struct stat status,char *items,int fd){
    fd = open(items, O_RDWR);
    fstat(fd,&status);
    data = mmap(NULL, status.st_size, PROT_READ, MAP_SHARED, fd, 0);
    close(fd);
    return data;
}

/**************************************************************Main function****************************************************************************************************/
int main(int argc, char* argv[]) {
    if (argc < 2) {
        perror("Error: Did not enter disk image.\n");
        exit(1);
    }
    struct stat sb;
    int fd;
    data = disk_image(sb,argv[1],fd);

    char* root = malloc(sizeof(char));
    char* root_data = data;
    int i;
  for(i=0; i<11; i++){
    root[i] = root_data[i+43];
  }
  if(root[0] == ' '){
    root_data = root_data + 512 *19;
    while(root_data[0] != 0x00){
      if(root_data[11] == 8){
        for(i=0; i<8; i++){
          root[i] = root_data[i];
        }
      }
    root_data = root_data + 32;
    }
  }
  printf("%s\n==================\n",root);
    int pos = 0x2600;
    data = data + 512 *19;
    list(data,root,0,pos);

    munmap(data, sb.st_size);
    return 0;
}