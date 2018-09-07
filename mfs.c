// The MIT License (MIT)
// 
// Copyright (c) 2016, 2017 Trevor Bakker 
// 
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#define _GNU_SOURCE

#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>

#define WHITESPACE " \t\n"      // We want to split our command line up into tokens
                                // so we need to define what delimits our tokens.
                                // In this case  white space
                                // will separate the tokens on our command line

#define MAX_COMMAND_SIZE 255    // The maximum command-line size

// MY CODE STARTS HERE

#define MAX_NUM_ARGUMENTS 10     // Mav shell only supports five arguments

#define MAX_HISTORY 15 // Mav shell will only store history of last 10 entries

struct cmd
{
  char *token[MAX_NUM_ARGUMENTS];
  pid_t pid; 
};

struct cmd history[MAX_HISTORY];
int counter = 0;

int first_null( char *array[MAX_NUM_ARGUMENTS] )
{
  int first_null = 0;
  for(int i = 0; i < MAX_NUM_ARGUMENTS; i++)
  {
    if( array[i] == NULL )
    {
      break;
    }
    first_null++;
  }
  return first_null;
}

char * concat_str( char * str1, char * str2 )
{
  // +1 is added for null space
  char *final = malloc( strlen(str1) + strlen(str2) + 1);
  strcpy( final, str1 );
  strcat( final, str2 );
  return final;
}

static void handle_signal ( int sig )
{
    /*
   Determine which of the two signals were caught and 
   print an appropriate message.
  */

  switch( sig )
  {
    case SIGINT: 
      //printf("Caught a SIGINT\n");
    break;

    case SIGTSTP: 
      //printf("Caught a SIGTSTP\n");
    break;

    default: 
      printf("Unable to determine the signal\n");
    break;

  }
}

// MY CODE ENDS HERE

int main()
{
  // MY CODE STARTS HERE
  // SET UP SIGNALS

  struct sigaction act;
 
  /*
    Zero out the sigaction struct
  */ 
  memset (&act, '\0', sizeof(act));
 
  /*
    Set the handler to use the function handle_signal()
  */ 
  act.sa_handler = &handle_signal;
 
  /* 
    Install the handler for SIGINT and SIGTSTP and check the 
    return value.
  */ 
  if (sigaction(SIGINT , &act, NULL) < 0) 
  {
    perror ("sigaction: ");
    return 1;
  }

  if (sigaction(SIGTSTP , &act, NULL) < 0) 
  {
    perror ("sigaction: ");
    return 1;
  }

  // MY CODE ENDS HERE

  char * cmd_str = (char*) malloc( MAX_COMMAND_SIZE );

  while( 1 )
  {
    // Print out the msh prompt
    printf ("msh> ");

    // Read the command from the commandline.  The
    // maximum command that will be read is MAX_COMMAND_SIZE
    // This while command will wait here until the user
    // inputs something since fgets returns NULL when there
    // is no input
    while( !fgets (cmd_str, MAX_COMMAND_SIZE, stdin) );

    /* Parse input */
    char *token[MAX_NUM_ARGUMENTS];

    int   token_count = 0;                                 
                                                           
    // Pointer to point to the token
    // parsed by strsep
    char *arg_ptr;                                         
                                                           
    char *working_str  = strdup( cmd_str );                

    // we are going to move the working_str pointer so
    // keep track of its original value so we can deallocate
    // the correct amount at the end
    char *working_root = working_str;

    // Tokenize the input stringswith whitespace used as the delimiter
    while ( ( (arg_ptr = strsep(&working_str, WHITESPACE ) ) != NULL) && 
              (token_count<MAX_NUM_ARGUMENTS))
    {
      token[token_count] = strndup( arg_ptr, MAX_COMMAND_SIZE );
      if( strlen( token[token_count] ) == 0 )
      {
        token[token_count] = NULL;
      }
        token_count++;
    }

    // Now print the tokenized input as a debug check
    // MY CODE STARTS HERE

    // if there are token then the program should not try to change
    // directories or execute any program
    if( first_null(token) != 0 )
    {
      // check whether the inputed string is cd, because if
      // the command is cd we do not need fork.
      if( strcmp(token[0], "cd" ) == 0 )
      {
        chdir( token[1] );
      }
      else if( (strcmp(token[0], "exit") == 0 ) ||  (strcmp(token[0], "quit") == 0 ) )
      {
        printf("exit\n");
        return 0;
      }
      // for every other command we need to call, we need to fork
      // this is because it is an independed process that we are
      // are calling
      else
      {
        pid_t pid = fork();

        if( pid == -1 )
        {
          // When fork() returns -1, an error happened.
          perror("fork failed: ");
        }

        // When fork() returns 0, we are in the child process.
        // in child we exec the user input
        else if ( pid == 0 )
        {
          // record so we can kill or get history

          // start process in order of folders
          char *path = concat_str( "/usr/local/bin/", token[0] );
          ssize_t err = execv( path, token );
          // check if process is sucessfully running. If not
          // look for executable in next directory
          if ( err < 0 )
          {
            char *path = concat_str( "/usr/bin/", token[0] );
            ssize_t err = execv( path, token );
            if ( err < 0 )
            {
              char *path = concat_str( "/bin/", token[0] );
              ssize_t err = execv( path, token );
              if ( err < 0 )
              {
                printf("%s: Command not found.\n\n", token[0]);
                kill(getpid(),6);
              }
            }
          }

          free( path );
          fflush(NULL);
        }
        else
        {
          // When fork() returns a positive number, we are in the parent
          // process and the return value is the PID of the newly created
          // child process.
          int status;

          // Force the parent process to wait until the child process 
          // exits
          pid_t completed = wait(&status);
          //printf("Hello from the parent process\n\tPID: %d\n\tPPID: %d\n", getpid(),getppid());
          fflush( NULL );
        }
      }
    }

    // MY CODE ENDS HERE

    int token_index  = 0;
    /*for( token_index = 0; token_index < token_count; token_index ++ ) 
    {
      printf("token[%d] = %s\n", token_index, token[token_index] );  
    }*/

    free( working_root );

  }
  return 0;
}
