#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

char cmdList[10][10][30];


int interpret_input(char* input) {
    int cmdNum = 0;
    int instOrder = 0;
    char* ptr = strtok(input, " ");
    while (1) {
        strcpy(cmdList[cmdNum][instOrder++], ptr);
        ptr = strtok(NULL, " ");
        if (ptr == NULL) {
            strcpy(cmdList[cmdNum++][instOrder], "");
            break;
        }
        if (ptr[0] == '|') {
            strcpy(cmdList[cmdNum++][instOrder], "");
            ptr = strtok(NULL, " ");
            instOrder = 0;
        }
    }
    return cmdNum;
}
int globalpgid=0;

void type1func(int ord, int type) {
    char path[20];
    char* arg[20] = {NULL, };
    int i=0;
   int pid=0;
    while(1){
      if(!strcmp(cmdList[ord][i], "") || !strcmp(cmdList[ord][i], ">") || !strcmp(cmdList[ord][i], "<") || !strcmp(cmdList[ord][i], ">>")) {
         break;
      }
       arg[i] = cmdList[ord][i];
      i++;
    }
    if(type==0){
       sprintf(path, "/bin/%s", arg[0]);
    }
    else if(type==1){
        sprintf(path, "/usr/bin/%s", arg[0]);
    }
    else if(type==2){
       sprintf(path, "%s", arg[0]);
    }
   if (execv(path, arg) == -1) {
            //printf("type1: %s error\n", arg[0]);
    }
}

//cmd type2
void head(int k, int fd) {
   char ch;
   int line = 0;
   if(k==-1) k=10;
   while(read(fd, &ch, 1) > 0){
      write(STDOUT_FILENO, &ch, 1);
      if(ch=='\n') line++;
      if(line >= k) break;
   }
}

void tail(int k, int fd) {
   char ch;
   char text[1024][1024];
   int line = 0;
   int i=0;
   if(k==-1) k=10;

   int dummyfp = open("dummyszx", O_RDWR | O_CREAT, 0644);
   while(read(fd, &ch, 1) > 0) {
             if(ch=='\n') line++;
         write(dummyfp, &ch, 1);
   }
   lseek(dummyfp, 0, SEEK_SET);
   while(i<(line-k)){
      read(dummyfp, &ch, 1);
      if(ch=='\n') i++;
   }
   while(read(dummyfp, &ch, 1)>0){
      write(STDOUT_FILENO, &ch, 1);
   }
   close(dummyfp);
   unlink("dummyszx");
}

void cat(int fdi, int fdo) {
   char ch;
   while(read(fdi, &ch, 1)>0){
      write(fdo, &ch, 1);
   }
}

int isEnd(char* str){
   if(!strcmp(str, "") || !strcmp(str, ">") || !strcmp(str, "<") || !strcmp(str, ">>")){
      return 1;
   }
   return 0;
}

void sigint_handler(int sig){

}
void sigtstp_handler(int sig){
   int status;
   kill(-1*globalpgid, SIGTSTP);
   waitpid(-1, &status, WNOHANG | WUNTRACED);
   kill(-1*globalpgid, SIGKILL);
   exit(1);
   if(WIFSTOPPED(status)){
      
   }
}

int main() {
    char input[201];
    int cmdNum;
   int cmdOrd;
   int type3err =0;
   char* arg1;
   char* arg2;
   char* cmdtype;
   int tmpFd;
   int arguFd;
   int pipeFdList[20][2];
    int pgroupid;
   int status;

   int ogOut = dup(STDOUT_FILENO);
   int ogIn  = dup(STDIN_FILENO);

   //signal(SIGINT, sigint_handler);
   
   while (1) {
      dup2(ogIn, STDIN_FILENO);
      dup2(ogOut, STDOUT_FILENO);

      fgets(input, 200, stdin);

      input[strlen(input)-1] = '\0';
        cmdNum = interpret_input(input);

      type3err = 0;

      if(!strcmp(cmdList[0][0], "exit")){
         write(STDERR_FILENO, "exit\n", 5);
         exit(atoi(cmdList[0][1]));
      }
      else if(!strcmp(cmdList[0][0], "rm")){
               type3err = unlink(cmdList[0][1]);
            }
      else if(!strcmp(cmdList[0][0], "cd")){
         type3err = chdir(cmdList[0][1]);
      }
      else if(!strcmp(cmdList[0][0], "mv")){
         char newname[100];
         sprintf(newname, "%s%s", cmdList[0][2], cmdList[0][1]);
         type3err = rename(cmdList[0][1], newname);
      }
      else if(!strcmp(cmdList[0][0], "cp")){
         int fdi = open(cmdList[0][1], O_RDONLY);
         int fdo = open(cmdList[0][2], O_RDWR | O_CREAT, 0644);
         cat(fdi, fdo);
         close(fdi);
         close(fdo);
      }
      else if(!strcmp(cmdList[0][0], "pwd")){
         char pwdir[255];
         getcwd(pwdir, 255);
         strcat(pwdir, "\n");
         write(STDOUT_FILENO, pwdir, strlen(pwdir)); 
      }
      else if(!strcmp(cmdList[0][0], "man" )&& cmdNum == 1){
         int i =0;
         char *arg [50] = {NULL,};
         char path[50];
         int fff = 0;
         for (int i = 0; strcmp(cmdList[cmdOrd][i],""); i++) {
            if(!strcmp(cmdList[cmdOrd][i], ">") || !strcmp(cmdList[cmdOrd][i], ">>")){
               fff=1;
               break;
            }
         }
         if(!fff){
         while(1){
            if(!strcmp(cmdList[0][i], "") || !strcmp(cmdList[0][i], ">") || !strcmp(cmdList[0][i], "<") || !strcmp(cmdList[0][i], ">>")) {
               break;
            }
            arg[i] = cmdList[0][i];
            i++;
         }
           sprintf(path, "/usr/bin/%s", arg[0]);
    
         if (fork() == 0) {
                 if (execv(path, arg) == -1) {
              }
         }
          else wait(NULL);
         }
      }
      else{
         for(int cmdOrd=0; cmdOrd<cmdNum; cmdOrd++){
            //redirection check
            if(cmdOrd != cmdNum-1) pipe(pipeFdList[cmdOrd]);
            if((globalpgid=fork())==0){
               if(cmdOrd==0){
                  setpgid(0, 0);
                  pgroupid = getpid();
               }
               setpgid(0, pgroupid);
               if(cmdNum>1){
                  if(cmdOrd==0){
                     dup2(pipeFdList[cmdOrd][1], STDOUT_FILENO);
                  }
                  else if(cmdOrd==cmdNum-1){
                     dup2(pipeFdList[cmdOrd-1][0], STDIN_FILENO);
                     dup2(ogOut, STDOUT_FILENO);
                  }
                  else {
                     dup2(pipeFdList[cmdOrd-1][0], STDIN_FILENO );
                     dup2(pipeFdList[cmdOrd  ][1], STDOUT_FILENO);
                  }
               }
               for (int i = 0; strcmp(cmdList[cmdOrd][i],""); i++) {
                  if(!strcmp(cmdList[cmdOrd][i], "<")){
                     tmpFd = open(cmdList[cmdOrd][i+1], O_RDONLY);
                     if(tmpFd < 0){
                        printf("swsh: No such file\n");
                        return 0;
                     }
                     dup2(tmpFd, STDIN_FILENO);
                     close(tmpFd);
                  }
                  else if(!strcmp(cmdList[cmdOrd][i], ">")){
                     tmpFd = open(cmdList[cmdOrd][i+1], O_RDWR | O_CREAT, 0644);
                     dup2(tmpFd, STDOUT_FILENO);
                     close(tmpFd);
                  }
                  else if(!strcmp(cmdList[cmdOrd][i], ">>")){
                     tmpFd = open(cmdList[cmdOrd][i+1], O_APPEND | O_CREAT | O_WRONLY, 0644);
                     dup2(tmpFd, STDOUT_FILENO);
                     close(tmpFd);
                  }
               }

               cmdtype = cmdList[cmdOrd][0];
               // type1
               if(!strcmp(cmdtype, "ls") || !strcmp(cmdtype, "grep")){
                  type1func(cmdOrd, 0);
               }
               else if(!strcmp(cmdtype, "sort") || !strcmp(cmdtype, "man") || !strcmp(cmdtype, "awk") || !strcmp(cmdtype, "bc")){
                  type1func(cmdOrd, 1);
               }
               else if(cmdtype[0]=='.' && cmdtype[1]=='/'){
                  type1func(cmdOrd, 2);
               }
               //type2
               else if(!strcmp(cmdtype, "head")){
                  if(!strcmp(cmdList[cmdOrd][1], "-n")){
                     if(!isEnd(cmdList[cmdOrd][3])){
                        arguFd = open(cmdList[cmdOrd][3], O_RDONLY);
                        head(atoi(cmdList[cmdOrd][2]), arguFd);
                        close(arguFd);
                     }
                     else {
                        head(atoi(cmdList[cmdOrd][2]), STDIN_FILENO);
                     }               
                  }
                  else{
                     if(!isEnd(cmdList[cmdOrd][1])){
                        arguFd = open(cmdList[cmdOrd][1], O_RDONLY);
                        head(-1, arguFd);
                        close(arguFd);
                     }
                     else{
                        head(-1, STDIN_FILENO);
                     }
                     
                  }   
               }
               else if(!strcmp(cmdtype, "tail")){
                  if(!strcmp(cmdList[cmdOrd][1], "-n")){
                     if(!isEnd(cmdList[cmdOrd][3])){
                        arguFd = open(cmdList[cmdOrd][3], O_RDONLY);
                        tail(atoi(cmdList[cmdOrd][2]), arguFd);
                        close(arguFd);
                     }
                     else {
                        tail(atoi(cmdList[cmdOrd][2]), STDIN_FILENO);
                     }               
                  }
                  else{
                     if(!isEnd(cmdList[cmdOrd][1])){
                        arguFd = open(cmdList[cmdOrd][1], O_RDONLY);
                        tail(-1, arguFd);
                        close(arguFd);
                     }
                     else{
                        tail(-1, STDIN_FILENO);
                     }
                     
                  }    
               }
               else if(!strcmp(cmdtype, "cat")){
                  if(!isEnd(cmdList[cmdOrd][1])){
                     arguFd = open(cmdList[cmdOrd][1], O_RDONLY);
                     cat(arguFd, STDOUT_FILENO);
                     close(arguFd);
                  }
                  else{
                     cat(STDIN_FILENO, STDOUT_FILENO);
                  }
                  
               }
               else {
                  write(STDERR_FILENO, "swsh: Command not found\n", 24);
               }
               
               exit(0);
            }else{
               //parent
               signal(SIGTSTP, sigtstp_handler);
               
            }
            if(cmdNum>1){
               if(cmdOrd==0){
                  close(pipeFdList[cmdOrd][1]);
               }
               else if(cmdOrd==cmdNum-1){
                  close(pipeFdList[cmdOrd-1][0]);
               }
               else {
                  close(pipeFdList[cmdOrd-1][0]);
                  close(pipeFdList[cmdOrd  ][1]);
               }
            }
         }
      }
      if(type3err < 0){
         char errmsg[255];
         char err[255];
         char name[50];
         
         strcpy(name, cmdList[0][0]);

         if(errno==EACCES) strcpy(err, "Permission denied");
         else if(errno==EISDIR) strcpy(err, "Is a directory");
         else if(errno==ENOENT) strcpy(err, "No such file or directory");
         else if(errno==ENOTDIR) strcpy(err, "Not a directory");
         else if(errno==EPERM) strcpy(err, "Permission denied");
         else sprintf(err, "Error occurred: <%d>", errno);
         sprintf(errmsg, "%s: %s\n", name, err);
         write(STDERR_FILENO, errmsg, strlen(errmsg));
         type3err = 0;
      }
      
      while(wait(NULL)>0){}
    }
}
