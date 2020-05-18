/**
	Name: Shameen Shetty
	ID: 1001429743
*/

/**
	Description: In this program we are trying to familiarize ourselves with the 
	FAT32 file system. We implement commands such as 'get' or 'open' or 'cd' which
	will access our image.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>


FILE *fp;

struct  DirStruct
{
    char dir_name[11];
    uint8_t dir_attribute;
    uint8_t notused[8];
    uint16_t DIR_FirstClusterHigh;
    uint8_t unused[4];
    uint16_t dir_first_cluster_low;
    uint32_t dir_filesize;
};
struct DirStruct dir[16]; //Creation of the directory

char Oem_name[8];
int16_t Bytes_Per_Sector; // bytes per sector
int8_t Sector_Per_Cluster; //sectors per cluster
int16_t Reserved_Sector_Count; //reserved sectors Amount
int8_t Numbeer_FAT;
int16_t Root_Entry_Count; // count of Rootentry
int32_t FATSz32;
int32_t BPB_Root_Cluster_Location; //location of Rootcluster

int32_t RootDirSectors = 0; //root dir sector count
int32_t FirstDataSector = 0; //location of  first data sector .
int32_t FirstSectorofCluster = 0;

int32_t currDir;
char formattedDirectory[12];



int LBAToOffset(int32_t sector);
void printDirectory();
int openFile(char *buf);
void info();
void stat(char *dirname);
void cd(char *dir);
void ls();
void get(char *dirname);
void readImg();


int32_t getCluster(char *dirname);
int32_t clusterSize(int32_t cluster);
void fmtDir(char *dirname);
void readFile(char *dirname, int position, int numOfBytes);
void CD(int32_t cluster);


char *buf;
char *tok0;
char *tok1;
char *tok2;
char *tok3;


int main()
{



  while(1)
   {
    printf("mfs> ");
    buf = (char *)malloc(256);
    tok0 =(char *)malloc(256);
    tok1 =(char *)malloc(256);
    tok2 =(char *)malloc(256);
    tok3 =(char *)malloc(256);
    int tok_count = 0;

    memset(buf,'\0',256);
    fgets(buf, 256, stdin) ;
    char *string,*found;

        string = strdup(buf);


        while( (found = strsep(&string," ")) != NULL )
        {

            if(tok_count==0)
            {
              strcpy(tok0,found);
            }
            if(tok_count==1)
            {
              strcpy(tok1,found);
              int len = strlen(tok1);
              char c = tok1[len-1];
              if(c=='\n')
                 tok1[len-1]='\0';
            }
            if(tok_count==2)
            {
              strcpy(tok2,found);
            }
            if(tok_count==3)
            {
              strcpy(tok3,found);
              int len =strlen(tok3);
              tok3[len-1]='\0';
            }

            tok_count++;


        }
        
        // Get the first token, token 0
        // and if it is "open", then we will
        // open our image file.
        if( strcmp(tok0, "open") == 0)
        {
          int ret = openFile(tok1);
          if(ret!=-1)
          {
              fseek(fp, 3, SEEK_SET);
              fread(&Oem_name, 8, 1, fp);
              fseek(fp, 11, SEEK_SET);
              fread(&Bytes_Per_Sector, 2, 1, fp);
              fread(&Sector_Per_Cluster, 1, 1, fp);
              fread(&Reserved_Sector_Count, 2, 1, fp);
              fread(&Numbeer_FAT, 1, 1, fp);
              fread(&Root_Entry_Count, 2, 1, fp);
          }

        }
        if(strcmp(tok0, "info\n") == 0)
        {
          info();
        }
        if(strcmp(tok0, "stat\n") == 0)
        {
          printf("Please give filename  or directory name\n");
          continue;
        }
        if(strcmp(tok0, "stat") == 0)
        {
          if(tok1 == NULL)
          {
              continue;
          }

          stat(tok1);
        }
        if(strcmp(tok0, "ls\n") == 0)
        {
          printDirectory();
        }
        if(strcmp(tok0, "cd") == 0)
        {

          if (tok1 == NULL)
          {
              printf("Error: Please provide which directory you would like to open\n");
              return -1;
          }
          CD(getCluster(tok1));

        }

        if(strcmp(tok0, "read") == 0)
        {
          int begin=atoi(tok2);
          int end = atoi(tok3);
          printf("begin=%d..%d\n",begin,end);
          readFile(tok1,begin ,end );
        }

        if(strcmp(tok0, "close\n") ==0 || strcmp(tok0, "close") == 0)
        {

          if (fp == NULL)
          {
              printf("Error: Image File not open.\n");
              continue;
          }

          int ret = fclose(fp);
          if (ret == 0)
          {
              printf("Fat32 Image file system closed.\n");
              fp=NULL;
          }

        }
        if(strcmp(tok0, "get\n") == 0)
        {
          printf("ERR: Please provide which file you would like to download\n");
          continue;
        }
        if(strcmp(tok0, "get") == 0)
        {
          get(tok1);
        }


    }
    free(buf);
    return 0;
}



int openFile(char *file)
{

  if (fp != NULL)
  {
      printf("Error: File system image already open.\n");
      return -1;
  }


  fp = fopen(file, "r");
  if (fp == NULL)
  {
      printf("Error: File system not found.\n");
      return -1;
  }


    printf("%s opened.\n", file);

    fseek(fp, 3, SEEK_SET);
    fread(&Oem_name, 8, 1, fp);

    fseek(fp, 11, SEEK_SET);
    fread(&Bytes_Per_Sector, 2, 1, fp);
    fread(&Sector_Per_Cluster, 1, 1, fp);
    fread(&Reserved_Sector_Count, 2, 1, fp);
    fread(&Numbeer_FAT, 1, 1, fp);
    fread(&Root_Entry_Count, 2, 1, fp);

    fseek(fp, 36, SEEK_SET);
    fread(&FATSz32, 4, 1, fp);

    fseek(fp, 44, SEEK_SET);
    fread(&BPB_Root_Cluster_Location, 4, 1, fp);
    currDir = BPB_Root_Cluster_Location;

    return 0;
}



void info()
{
  printf("Bytes_Per_Sector: %d  (0x%x)\n ", Bytes_Per_Sector,Bytes_Per_Sector);
  printf("Sector_Per_Cluster: %d  (0x%x)\n", Sector_Per_Cluster,Sector_Per_Cluster);
  printf("Reserved_Sector_Count: %d  (0x%x)\n", Reserved_Sector_Count,Reserved_Sector_Count);
  printf("Numbeer_FAT: %d  (0x%x)\n", Numbeer_FAT,Numbeer_FAT);
  printf("FATSz32: %d  (0x%x)\n", FATSz32,FATSz32);
}


void stat(char *dirname)
{
  int cluster = getCluster(dirname);
  int size = clusterSize(cluster);
  if(size==-1)
  {
    // printf("%s not found\n",dirname);
    printf("Error: File not found.\n");
    return;
  }
  printf("Size: %d\n", size);
  int i;
  for (i = 0; i < 16; i++)
  {
      if (cluster == dir[i].dir_first_cluster_low)
      {
          printf("Attr: %d\n", dir[i].dir_attribute);
          printf("Starting Cluster: %d\n", cluster);
          printf("Cluster High: %d\n", dir[i].DIR_FirstClusterHigh);
      }
  }

}



int LBAToOffset(int32_t sector)
{
    if (sector == 0)
        sector = 2;
    return ((sector - 2) * Bytes_Per_Sector) + (Bytes_Per_Sector * Reserved_Sector_Count) + (Numbeer_FAT * FATSz32 * Bytes_Per_Sector);
}


void printDirectory()
{
    if (fp == NULL)
    {
        printf("No image is opened\n");
        return;
    }

    int offset = LBAToOffset(currDir);
    fseek(fp, offset, SEEK_SET);

    int i;
    for (i = 0; i < 16; i++)
    {
        fread(&dir[i], 32, 1, fp);

        if ((dir[i].dir_name[0] != (char)0xe5) &&
            (dir[i].dir_attribute == 0x1 || dir[i].dir_attribute == 0x10 || dir[i].dir_attribute == 0x20))
        {
            char *directory = (char *)malloc(11);
            memset(directory, '\0', 11);
            memcpy(directory, dir[i].dir_name, 11);
            printf("%s\n", directory);
        }
    }
}


void get(char *dirname)
{
  char *dirstring = (char *)malloc(strlen(dirname));
  printf("dirname=%s\n",dirname);
  strncpy(dirstring, dirname, strlen(dirname));
  printf("dirstring=%s\n",dirstring);
  int cluster = getCluster(dirstring);
  int size = clusterSize(cluster);
  FILE *newfp = fopen(tok1, "w");
  fseek(fp, LBAToOffset(cluster), SEEK_SET);
  unsigned char *ptr = (unsigned char *)malloc(size);
  fread(ptr, size, 1, fp);
  fwrite(ptr, size, 1, newfp);
  fclose(newfp);
}


int32_t getCluster(char *dirname)
{
    //Compare dirname to directory name (attribute), if same, cd into FirstClusterLow
    fmtDir(dirname);

    int i;
    for (i = 0; i < 16; i++)
    {
        char *directory = (char *)malloc(11);
        memset(directory, '\0', 11);
        memcpy(directory, dir[i].dir_name, 11);

        if (strncmp(directory, formattedDirectory, 11) == 0)
        {
            int cluster = dir[i].dir_first_cluster_low;
            return cluster;
        }
    }

    return -1;
}

int32_t clusterSize(int32_t cluster)
{
    int i;
    for (i = 0; i < 16; i++)
    {
        if (cluster == dir[i].dir_first_cluster_low)
        {
            int size = dir[i].dir_filesize;
            return size;
        }
    }
    return -1;
}


void fmtDir(char *dirname)
{
    char expanded_name[12];
    memset(expanded_name, ' ', 12);
    char *token = strtok(dirname, ".");
    if (token)
    {
        strncpy(expanded_name, token, strlen(token));

        token = strtok(NULL, ".");

        if (token)
        {
            strncpy((char *)(expanded_name + 8), token, strlen(token));
        }

        expanded_name[11] = '\0';

        int i;
        for (i = 0; i < 11; i++)
        {
            expanded_name[i] = toupper(expanded_name[i]);
        }
    }
    else
    {
        strncpy(expanded_name, dirname, strlen(dirname));
        expanded_name[11] = '\0';
    }
    strncpy(formattedDirectory, expanded_name, 12);
}

void readFile(char *dirname, int position, int numOfBytes)
{
    printf("dirname=%s\n",dirname);
    int cluster = getCluster(dirname);
    printf("cluster=%d\n",cluster);
    int offset = LBAToOffset(cluster);
    printf("offset=%d\n",offset);
    fseek(fp, offset + position, SEEK_SET);
    char *bytes = (char *) malloc(numOfBytes);
    fread(bytes, numOfBytes, 1, fp);
    printf("%s\n", bytes);
}

void CD(int32_t cluster)
{


  if (strcmp(tok1, "..") == 0)
  {
      int i;
      for (i = 0; i < 16; i++)
      {
          if (strncmp(dir[i].dir_name, "..", 2) == 0)
          {
              int offset = LBAToOffset(dir[i].dir_first_cluster_low);
              currDir = dir[i].dir_first_cluster_low;
              fseek(fp, offset, SEEK_SET);
              fread(&dir[0], 32, 16, fp);
              return;
          }
      }

    int offset = LBAToOffset(cluster);
    currDir = cluster;
    fseek(fp, offset, SEEK_SET);
    fread(&dir[0], 32, 16, fp);
  }

}
