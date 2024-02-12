#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(int argc, char ** argv)
{
    // Check if minimum number of arguments was met, if not informs user and returns error code
    if(argc < 3)
    {
        printf("Not enough arguments!\n");

        return 1;
    }

    char zipProgram[40] = "/usr/bin/";
    int argumentNum = 2, status = 0;
    pid_t childPid, waitPid;

    strcat(zipProgram, argv[1]); 

    for (int i = 0; i < argc - 2; i++)
    {
        childPid = fork();

        // Check if child process creation failed
        if (childPid < 0)
        {
            printf("Fork creation failed!");

            exit(-1);
        }
        // If child process was succesfully created, execute file compression with desired program
        else if(childPid == 0)
        {
            int check = execl(zipProgram, zipProgram , argv[argumentNum], NULL);
            
            // Check if compression ended succesfully if not inform user 
            if(check == -1)
            {
                printf("Could not exececute %s, program might not exist.\n", argv[1]);

                exit(-1);
            }
        }

        // Parent process iterates over arguments to compress all files
        else
            argumentNum++;
        
    }

    //Parent waits to prevent zombie processes
    while ((waitPid = wait(&status)) > 0)
    


    return 0;
}