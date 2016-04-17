/* LISTAR FICHEIROS REGULARES E SUB-DIRECTÓRIOS DE UM DIRECTÓRIO */
/* USO: lstdir dirname */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>

#define FILE_LIST_NAME "files.txt"
#define FILE_NAME_SIZE 63
#define PATH_SIZE 255

void writeInfoToFile(char path[], struct stat* statBuf, FILE* fileList, char* dirName){
  char* name;
  name = strrchr(path, '/');
  name++;
  fprintf(fileList, "\"%s\" %zu %3o %lld \"%s\"\n", name, statBuf->st_size, statBuf->st_mode & 0777, (long long) statBuf->st_mtim.tv_sec, path);
}

void createProccess(char path[]){
  if(fork() == 0){
    execlp("./lsdir", "lsdir", path, NULL);
    printf("lsdir to %s failed!\n", path);
    exit(1);
  }
}

void fixFilePath(char* givenPath, char newPath[], struct dirent* direntp){
  char buf[PATH_SIZE];
  if(givenPath[strlen(givenPath) - 1] == '/')
    givenPath[strlen(givenPath) - 1] = '\0';
  if(givenPath[0]=='/')
    sprintf(newPath,"%s/%s", givenPath, direntp->d_name);
  else if(strcmp(givenPath, ".") == 0)
    sprintf(newPath, "%s/%s", getcwd(buf, 256), direntp->d_name);
  else
    sprintf(newPath,"%s/%s/%s", getcwd(buf, 256), givenPath,direntp->d_name);
}

int main(int argc, char *argv[])
{
  DIR *dirp;
  FILE *fileList;
  struct dirent *direntp;
  struct stat stat_buf;
  char name[200];
  int proccessCount = 0;
  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s dir_name\n", argv[0]);
    exit(1);
  }
  if ((dirp = opendir(argv[1])) == NULL)
  {
    perror(argv[1]);
    exit(2);
  }
  if((fileList = fopen(FILE_LIST_NAME, "a")) == NULL){
    perror(FILE_LIST_NAME);
    exit(3);
  }
  
  int noIter;
  for(noIter = 0; noIter < 2; noIter++){
      while ((direntp = readdir(dirp)) != NULL)
      {
    
        fixFilePath(argv[1], name, direntp);
    
        if (lstat(name, &stat_buf)==-1)
        {
          perror("lstat ERROR");
          exit(3);
        }
        if (S_ISDIR(stat_buf.st_mode) && strcmp(direntp->d_name, ".") != 0 && strcmp(direntp->d_name, "..") != 0 && noIter == 0){
          createProccess(name);
          proccessCount++;
        }
        else if(S_ISREG(stat_buf.st_mode) && noIter == 1){
          writeInfoToFile(name, &stat_buf, fileList, argv[1]);
        }
        int pid=waitpid(-1, NULL,WNOHANG);
        if (pid != -1 && pid != 0){
          proccessCount--;
        }
      }
     rewinddir(dirp);
}
  for(; proccessCount > 0; proccessCount--){
    wait(NULL);
  }
  closedir(dirp);
  fclose(fileList);
  exit(0);
}
