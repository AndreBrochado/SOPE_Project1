#include <stdio.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>

void runlsdir(char dirName[]){
  if(fork() == 0){
    execlp("./lsdir", "lsdir", dirName, NULL);
    printf("lsdir to %s failed!\n", dirName);
    exit(1);
  }
  else{
    wait(NULL);
  }
}

int areFilesEqual(char path1[], char path2[]){
  int status;
  if(fork() == 0){
    execlp("diff", "diff", path1, path2, NULL);
    printf("diff failed! \n");
    exit(1);
  }
  else{
    wait(&status);
    return status;
  }
}

//CONTEXT: Files closer to be equal are closer on the list.
//STRATEGY: TAKE 2 ADJACENT FILES, COMPARE THEM
// IF THEY ARE EQUAL, HARDLINK THE 2ND TO THE 1ST AND REPROCCESS WITH 1ST AND 3RD
// IF THEY ARE DIFFERENT, REPROCCESS WITH 2ND AND 3RD
void parseSortedFile(){
  int i = 0;
  char buffer[1024];
  char name1[64], path1[256], name2[64], path2[256];
  int size1, permission1, date1, size2, permission2, date2;
  FILE* file = fopen("files.txt", "r");
  if(fgets(buffer, 1024, file) == NULL){
    printf("I cant read\n");
    return;
  }
  //printf("%s\n", buffer);
  sscanf(buffer, "\"%[^\"]\" %d %d %d \"%[^\"]\"", name1, &size1, &permission1, &date1, path1);
  while(fgets(buffer, 1024, file) != NULL){
    sscanf(buffer, "\"%[^\"]\" %d %d %d \"%[^\"]\"", name2, &size2, &permission2, &date2, path2);
    printf("%s == %s %d == %d %d == %d ?\n", name1, name2, size1, size2, permission1, permission2);
    if(strcmp(name1, name2) == 0 && size1 == size2 && permission1 == permission2){
      if(areFilesEqual(path1, path2) == 0){
      unlink(path2);
      if(link(path1, path2) != 0){
        perror(path1);
        exit(42);
      }
      FILE* hlinks = fopen("hlinks.txt", "a");
      fprintf(hlinks, "%s linked to ---> %s\n", path2, path1);
      fclose(hlinks);
     }
    }
    else{
      printf("One but not the same %d\n", i);
      i++;
      strcpy(name1, name2);
      strcpy(path1, path2);
      size1 = size2;
      permission1 = permission2;
      date1 = date2;
    }
  }
  fclose(file);
}

int main(int argc, char* argv[]){

  if (argc != 2)
  {
    fprintf(stderr, "Usage: %s dir_name\n", argv[0]);
    exit(1);
  }

  FILE* f = fopen("files.txt", "w");
  fclose(f);
  f = fopen("hlinks.txt", "w");
  fclose(f);

  runlsdir(argv[1]);

  int sortedFiles = open("files.txt", O_RDWR);
  int save = 0;
  if(fork() == 0){
    save = dup(STDOUT_FILENO);
    dup2(sortedFiles, STDOUT_FILENO);
    execlp("sort", "sort", "-d", "files.txt", NULL);
  }
  else{
    wait(NULL);
    dup2(save, STDOUT_FILENO);
    close(sortedFiles);
    parseSortedFile();
  }
  return 0;
}
