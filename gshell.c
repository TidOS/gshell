/*
 *  jshell.c
 *  shell
 *
 *  This is a shell written in c that supports pipes
 *  A line of input is read from the user and it's
 *  then parsed into tokens.  Then, it's further parsed
 *  and broken into sections between pipes.  Each part
 *  is run, piping its output to the next command and 
 *  reading its input from the pipe before it.
 *  
 *  Created by Jordan McLastName on 9/2/07.
 *
 */
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <tcl.h>
#include <sys/wait.h>


/*******************************************
//  Prototypes for extra functions
*******************************************/
/////////////////////////////////////////////
char** parse(char* parseMe, char** parsed);
char** getNextCommand(char** parsed);
int getNumTokens(char** parsed);
void getNumPipes(char** parsed);
////////////////////////////////////////////


int numTokens;
int numPipes;
int gotThisFar = 0;
int pipesPassed = 0;

int main(int argc, char** argv)
{

	char commandtoParse [2049];	//the command entered by user
	
	char* cwd;			//current working directory, for fun
	char cwtemp[2049];	//temp variable for getting cwd
	cwd = cwtemp;		//set cwd variable properly
	getcwd(cwtemp,2049);	//put cwd into cwd...know what I mean?
	
	char** parsedCommand;	//command after parsing
	char** runMe;		//the current command to be run
	
	//is it first / last command?
	//and how many times we've been through main loop
	int firstOne,lastOne,count;
	
	//for waiting on children
	int status = 0;
	
	//used in keeping track of pids of children
	int pid;
	int anArray[20][20];
	int** pipeArray = (int**)anArray;
	int pidArray[100];
	int numProcs = 0;
	
	//loop back and forth...forever
	while(1)
	{
		count = 0;
		printf("\nWorking in %s \n$ ",cwd);
		fflush(stdin);
		fgets(commandtoParse,2049,stdin);
		parsedCommand = parse(commandtoParse,parsedCommand);
		numTokens = getNumTokens(parsedCommand);
		getNumPipes(parsedCommand);
		gotThisFar = 0;
		
		//  User wants to quit the shell.  We must comply
		//  Maybe in future versions we should insult the user
		//  for leaving this excellent shell...or not
		if (strcmp(parsedCommand[0],"exit") == 0)
		{
			return 0;
			exit(0);
		}
		
		//may or may not be last, definitely first
		lastOne = 0;
		firstOne = 1;
		
		//keep looping until we've executed all our commands.
		while(!lastOne)
		{
			//get the next command from parsedCommand and put it into an array.
			//we'll call this array runMe.
			runMe = getNextCommand(parsedCommand);
			
			if(count >= numPipes + 1|| gotThisFar >= numTokens)
			{
				lastOne = 1;
			}
			
			if(!lastOne)
			{
				//create a pipe P and put it in the array of pipes
				pipeArray[count] = (int*)malloc(sizeof(int) * 2);
				pipeArray[count][0] = (int)malloc(sizeof(int));
				pipeArray[count][1] = (int)malloc(sizeof(int));
				pipe(pipeArray[count]);
			}
			
			//fork a child and keep its pid in our pidArray
			pid = fork();
			pidArray[count] = pid;
			if(pid != 0)
			{
				//Greetings, I am the parent.
				pidArray[count] = pid;
				numProcs++;
				
				//note that we must close my pipe ends or the children will never get 
				//end of file
				if(!lastOne)
				{
					//close write end of pipe
					close(pipeArray[count][1]);	
				}
				if(!firstOne)
				{
					//close read end of pipe P created in last loop iteration
					close(pipeArray[count - 1][0]);
				}
			}
			else if(pid == 0)
			{
				// I am the child.
				if(firstOne && lastOne)
				{
					//run our only program.
					numProcs++;
					execvp(runMe[0],runMe);
				}
				if(firstOne)
				{
					//put P's write end on stdout
					dup2(pipeArray[count][1],fileno(stdout));
					numProcs++;
					execvp(runMe[0],runMe);
				}
				if(lastOne)
				{
					//put old pipe's read end on stdin
					dup2(pipeArray[count - 1][0],fileno(stdin));
					numProcs++;
					execvp(runMe[0],runMe);
				}
				else
				{
					//read end of previous pipe onto stdin
					dup2(pipeArray[count -1][0],fileno(stdin));
					//write end of P onto stdout
					dup2(pipeArray[count][1],fileno(stdout));
					numProcs++;
					execvp(runMe[0],runMe);
				}
			}
			firstOne = 0;
			count++;
		}
		//wait for all children to finish.
		int j = 0;
		for(j = 0; j < numProcs; j++)
		{
			waitpid(pidArray[j], &status, 0);
		}
		numProcs = 0;
		pipesPassed = 0;
	}
	return 0;
}


/********************************************
*  Parse accepts a pointer to a string and
*  separates it into tokens, with a space
*  being the separator.  It uses the tcl
*  library's Tcl_Dstring and Tcl_Splitlist
*  to achieve this simply.
*
*  It returns a pointer to an array of the
*  newly parsed words from parseMe
***********************************************/
char** parse(char* parseMe, char** parsed)
{
	Tcl_DString dsCmdLine;
	int numArgs, len;
	Tcl_Interp* interp;
	// Create a Tcl interpreter object.  This will allow
	// use to call Tcl functions.
	interp = Tcl_CreateInterp();
	// Create a dynamic string that we can grow as we
	// read the command line.  This is easy using C++
	// strings too.
	Tcl_DStringInit(&dsCmdLine);
	len = strlen(parseMe);
	// put our entered command into a DString
	Tcl_DStringAppend(&dsCmdLine, parseMe, len);
	// now, we split our string into its parts and then return a pointer to an array
	// consisting of these parts
	Tcl_SplitList(interp,dsCmdLine.string,&numArgs, (const char***) &parsed);
	return parsed;
}


/********************************************************
//	This should return an array of character pointers 
//	(strings) in which the last element is NULL.  It 
//	gets this array by starting at the programNumberth
//	pipe and going until it finds either the end of the
//	parsed list or the next pipe
*********************************************************/
char** getNextCommand(char** parsed)
{
	int start = gotThisFar;
	//so if it's first command start is 0
	//now to get the end.  The end of a command can come
	//either when we come across a pipe or if we exhaust
	//all our tokens
	int end = 0;
	int i = gotThisFar;
	if(gotThisFar != 0)
		i++;
	for(i = i; i < numTokens; i++)
	{
		if(strcmp(parsed[i],"|") == 0)
		{
			//we have found a pipe
			end = i;
			pipesPassed++;
			break;
		}
	}
	if(i == numTokens)
	{
		end = numTokens;
	}
	//prepare our array to hold our command for execvp
	char** returnMe = (char**)malloc(sizeof(char*) * end - start + 1);
	int j = 0;
	for(i = start; i < end; i++)
	{
		returnMe[j] = parsed[i];
		j++;
	}
	returnMe[j] = NULL;
	gotThisFar = end + 1;
	return returnMe;
}

int getNumTokens(char** parsed)
{
	
	int i;
	for(i = 0; i < 2049; i++)
	{
		if(parsed[i] == NULL)
		{
			return i;
		}
	}
	return -1;
}

/*********************************
//  Before running this, getNumTokens
//  should have been run at least once
//  Having this number lets us know how
//  many times we will need to run 
//  a program.  Programs run is 
//  numPipes + 1
*******************************/
void getNumPipes(char** command)
{
	numPipes = 0;
	int i = 0;
	for(i = 0; i < numTokens; i++)
	{
		if(strcmp(command[i],"|") == 0)
		{
			numPipes++;
		}
	}
}
