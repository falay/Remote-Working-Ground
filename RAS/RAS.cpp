#include <iostream>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/wait.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h> 
#include <fcntl.h>
#include <dirent.h>
#include <signal.h>
#include <sstream>
#include <vector>
#include "RAS.hpp"




using namespace std ;





bool RAS::shell(int ID)
{
	char buffer[BUFSIZE] ;
	int revLen, cmdType ; 
	User* client = &(*usersPool)[ID] ;
				
	setenv( "PATH", (*client).PATH.c_str(), 1 ) ;	
	int Socket = (*client).Socket ;	

	
	if( (revLen = read( Socket, buffer, BUFSIZE )) < 0 )
		err_mesg("read error") ;
	else if( revLen == 0 )
		return false ;	
	else
	{
		string revComd = cmdModifier( buffer, revLen ) ;
		
		if( revComd == "exit" )
			return false ;
			
		else if( (cmdType = builtInAnalyser( revComd )) >= 0 )
			builtInHandler( ID, cmdType, revComd ) ;
			
		else 
		{	
			if( !revComd.empty() )
			{		
				/* Unknown command */
				if( !binFinder( revComd ) )
				{
					string invalidCmd = "Unknown command: [" + revComd.substr(0, revComd.find_first_of(" ")) + "].\n" ;
					writer( Socket, invalidCmd ) ;	
				}	
				else	
				{	
					processor( Socket, revComd ) ;
					(*client).pool.pipeCountDowner() ;
				}
			}	
		}		
	}	
		
	writer( Socket, "% " ) ;	
	
	return true ;
}






void RAS::cmdExecutor(int Socket, string Cmd)
{
	int pid, pos, inSock, OutSock, stderrSock = Socket ;
	Pipe outPipe(0), stderrPipe(-1) ; 
	bool validPublicPipeFlag = true ;

	User* client =  &((*usersPool)[ userFinder( Socket ) ]) ;
	
	/* stdin from public pipe or number pipe */
	if( inPublicPipeAnalyser( Cmd ) )
		inPublicPipeHandler( Socket, Cmd, inSock, validPublicPipeFlag ) ;
	else if( (inSock = (*client).pool.pipeReader()) < 0 ) 
		err_mesg("Invalid usage of pipe") ;
		
	/* stderr */
	if( (pos = Cmd.find_first_of("!")) != string::npos )
	{	
		stderrPipe = (*client).pool.pipeWriter( Cmd, pos ) ;
		stderrSock = stderrPipe.Socket[1] ;
	}	
		
	/* stdout to redirect file or public pipe or number pipe */
	if( outPublicPipeAnalyser( Cmd ) )
		outPublicPipeHandler( Socket, Cmd, OutSock, validPublicPipeFlag ) ;
	else
	{
		if( (pos = Cmd.find_first_of(">") ) != string::npos )
			OutSock = redirectHandler( Cmd, pos ) ;
		
		else if( (pos = Cmd.find_first_of("|") ) != string::npos )
		{	
			outPipe = (*client).pool.pipeWriter( Cmd, pos ) ;
			OutSock = outPipe.Socket[1] ;
		}	
				
		else
			OutSock = Socket ;
	}
	
	if( !validPublicPipeFlag )
		return ;	

	spaceEraser( Cmd ) ;
	
	if( (pid = fork()) < 0 )
		err_mesg("fork error") ;
	
	else if( pid == 0 )
	{			
		dup2( inSock, STDIN ) ;
		dup2( OutSock, STDOUT ) ;
		dup2( stderrSock, STDERR ) ;
		
		char** cmd = formatChanger( Cmd ) ;	
		
		if( execvp( cmd[0], cmd ) < 0 )
		{	
			string invalidCmd = "Unknown command: [" + Cmd + "].\n" ;
			writer( Socket, invalidCmd ) ;	
			exit(0) ;
		}
		
		exit(EXIT_SUCCESS);
	}
		
	else
	{
		wait(0) ;
				
		/* Closing read pipe */
		if( inSock != STDIN )		
			close( inSock ) ;
	
		/* Not number pipe */
		if( outPipe.counter == 0 )
		{
			if( socketCloser(OutSock, STDOUT, Socket) )
				close( OutSock ) ;
		}
		else 	
			(*client).pool.destroyWritePipe() ;
			
		
		/* stderr */
		if( socketCloser(stderrSock, STDERR, Socket) )
			close( stderrSock ) ;

	}
}


bool RAS::socketCloser(int outSock, int typeFD, int clientSock)
{
	return outSock != typeFD && outSock != clientSock ;
}



void RAS::processor(int Socket, string cmd)
{
	vector<string> parsedCmd = parser( cmd ) ;
		
	for(string cmd : parsedCmd)
		cmdExecutor( Socket, cmd ) ;
}


/*
	The commands are parsed as follows:
	
	cat | cat | cat |5
	=> [cat |0][cat |0][cat |5]
	to indicate it's a number pipe with 0
		
	cat | ls
	=> [cat |0][ls]
*/
vector<string> RAS::parser(string cmd)
{
	if( cmd.find_first_of("|") == string::npos )
		return { cmd } ;

	stringstream SS(cmd) ;
	string token ;
	vector<string> parsedToken ;
	
	while( getline(SS, token, '|') )
	{	
		if( !parsedToken.empty() )
		{
			if( isdigit(token[0]) )
			{	
				parsedToken.back() += token ;
				break ;
			}	
			else
				parsedToken.back() += "0" ;
		}
		parsedToken.push_back( token + "|" ) ;
	}
	
	if( parsedToken.back().back() == '|' )
		parsedToken.back().pop_back() ;
	
	return parsedToken ;	
}


/* Handle for the ">" redirect case */
int RAS::redirectHandler(string& cmd, int pos)
{
	int startPos = cmd.find_first_not_of(" ", pos+1) ;
	string file = cmd.substr(startPos, string::npos) ;
	
	int outputSocket ; 
	if( (outputSocket = open( file.c_str(), O_WRONLY|O_CREAT, S_IRWXU | S_IRWXG )) < 0 )
		err_mesg("open error") ;
	
	cmd = cmd.substr(0, pos) ;
	while( cmd.back() == ' ' )
		cmd.pop_back() ;
	
	return outputSocket ;
}


// "cat test.html" -> ["cat"]["test.html"]
char** RAS::formatChanger(string Cmd)
{
	stringstream SS(Cmd) ;
	string token ;
	vector<string> parseCmd ;
	
	while( getline(SS, token, ' ') )
		parseCmd.push_back( token ) ;

	char** cmd = new char* [ parseCmd.size() ] ;	
	for(int i=0; i<parseCmd.size(); i++)
	{
		cmd[i] = new char [parseCmd[i].length()] ;
		strcpy( cmd[i], parseCmd[i].c_str() ) ;
	}	
	
	cmd[ parseCmd.size() ] = NULL ;
	
	return cmd ;
} 



bool RAS::binFinder(string cmd)
{
	DIR* dirPointer ;
	struct dirent* entry ;
	bool isFound = false ;
	
	if( (dirPointer = opendir("./bin")) == NULL )
		err_mesg("Open bin error") ;

	while( (entry = readdir(dirPointer)) != NULL )
	{
		if( cmd.find( entry->d_name ) != string::npos )
		{	
			isFound = true ;
			break ;
		}	
	}		

	closedir( dirPointer ) ;
	
	return isFound ;
}





int RAS::builtInAnalyser(string cmd) 
{
	string cmdSubStr = cmd.substr(0, cmd.find_first_of(" ")) ;
	
	if( cmdSubStr == "printenv" )
		return 0 ;
	
	else if( cmdSubStr == "setenv" )
		return 1 ;
	
	else if( cmdSubStr == "who" )
		return 2 ;
	
	else if( cmdSubStr == "name" )
		return 3 ;

	else if( cmdSubStr == "yell" )
		return 4 ;
	
	else if( cmdSubStr == "tell" )
		return 5 ;	
	else
		return -1 ;
	
}


void RAS::builtInHandler(int ID, int cmdType, string cmd) 
{
	switch( cmdType )
	{
		case 0:
			printenvHandler( ID, cmd ) ;
			break ;
		
		case 1:
			setenvHandler( ID, cmd ) ;
			break ;
			
		case 2:
			whoHandler( ID, cmd ) ;
			break ;
		
		
		case 3:
			nameHandler( ID, cmd ) ;
			break ;
		
		case 4:
			yellHandler( ID, cmd ) ;
			break ;
		
		default:	
			tellHandler( ID, cmd ) ;
			break ;	
	} ;
}	


void RAS::printenvHandler(int ID, string cmd)	
{	
	User client = (*usersPool)[ID] ;
	int Socket = client.Socket ;	
	string PATH =  client.PATH ;

	int pos ;
	if( (pos = cmd.find_first_not_of(" ", 8)) != string::npos )
	{
		string envVar = cmd.substr(pos, string::npos) ;
			
		if( envVar != "PATH" )
		{	
			foolProofer( Socket ) ;
			return ;
		}	
			
		writer( Socket, PATH+"\n" ) ;
	}
	else
		foolProofer( Socket ) ;		
}



void RAS::setenvHandler(int ID, string cmd)
{
	int Socket = (*usersPool)[ID].Socket ;		
	
	stringstream SS(cmd) ;
	string token, paramter ;
	int counter = 1 ;
		
	while( getline(SS, token, ' ') )
	{
		if( counter == 2 )
		{	
			if( token != "PATH" )
			{
				foolProofer( Socket ) ;		
				return ;
			}	
		}
		else if( counter == 3 )
			paramter = token ;
			
		counter ++ ;
	}	
		
	setenv( "PATH", paramter.c_str(), 1 ) ;
	(*usersPool)[ID].PATH = paramter ;		
}




void RAS::whoHandler(int ID, string cmd)
{
	int Socket = (*usersPool)[ID].Socket ;
	
	if( cmd != "who" )
		writer( Socket, "Error: invalid use of who\n" ) ;
	else
	{	
		string mesg = "<ID>\t<nickname>\t<IP/port>\t<indicate me>\n" ;
		
		for(int id=1; id<=(*usersPool).size(); id++)
		{	
			if( (*usersPool).count(id) )
			{
				User client = (*usersPool)[id] ;
				mesg += to_string(id) + "\t" + client.Name + "\t" + client.IP + "/" + to_string(client.port) ;
				
				if( id == ID )
					mesg += "\t<-me\n" ;
				else
					mesg += "\n" ;
			}	
		}
		
		writer( Socket, mesg ) ;
	}	
		
}



void RAS::tellHandler(int ID, string cmd) 
{
	User client = (*usersPool)[ID] ;

	string id, mesg ;
	int pos, pos2 ;
	
	/* Parsing */
	if( (pos = cmd.find_first_of(" ")) == string::npos || (pos2 = cmd.find_first_of(" ",pos+1)) == string::npos )
	{	
		writer(client.Socket, "Invalid use of name\n") ;
		return ;
	}	
	else
	{
		id = cmd.substr( pos+1, pos2-pos-1 ) ;	
		mesg = cmd.substr( pos2+1, string::npos ) ;
		
		if( !isdigit( id[0] ) )
		{
			writer(client.Socket, "Invalid user ID\n") ;
			return ;
		}	
	}	
	
	/* Checking existence */
	if( !(*usersPool).count( stoi(id) ) )
		writer(client.Socket, "*** Error: user #(" + id + ") does not exist yet. ***\n") ;
	else
		writer((*usersPool)[stoi(id)].Socket, "*** " + client.Name + " told you ***: " + mesg + "\n") ;		
	
}


void RAS::nameHandler(int ID, string cmd) 
{
	User client = (*usersPool)[ID] ;
	string newName ;
	int pos ;
	
	/* parsing */
	if( (pos = cmd.find_first_of(" ")) != string::npos )
		newName = cmd.substr( pos+1, string::npos ) ;	
	else
	{
		writer(client.Socket, "Invalid use of name\n") ;
		return ;
	}	
	
	/* checking duplcated */
	for(auto it=(*usersPool).begin(); it!=(*usersPool).end(); ++it)
	{
		if( (it->second).Name == newName )
		{
			writer(client.Socket, "*** User '" + newName + "' already exists. ***\n") ;
			return ;
		}	
	}
	
	/* updating name */
	(*usersPool)[ID].Name = newName ;
	
	/* broadcasting message */
	broadcaster("*** User from " + client.IP + "/" + to_string(client.port) + " is named '" + newName + "'. ***\n") ;
}



void RAS::yellHandler(int ID, string cmd) 
{
	User client = (*usersPool)[ID] ;
	int pos ;
	string mesg ;
	
	/* parsing */
	if( (pos = cmd.find_first_of(" ")) != string::npos )
		mesg = cmd.substr( pos+1, string::npos ) ;	
	else
	{
		writer(client.Socket, "Invalid use of yell\n") ;
		return ;
	}	

	/* broadcasting message */
	broadcaster("*** " + client.Name + " yelled ***: " + mesg) ;
}





void RAS::broadcaster(string mesg)
{
	mesg += "\n" ;
	for(auto it=(*usersPool).begin(); it!=(*usersPool).end(); ++it)
		writer( (it->second).Socket, mesg ) ;	
}	



bool RAS::inPublicPipeAnalyser(string Cmd)
{
	return Cmd.find_first_of("<") != string::npos ;
}


void RAS::inPublicPipeHandler(int Socket, string& Cmd, int& inSock, bool& flag)
{
	/* parsing */
	int start = Cmd.find_first_of("<") ;
	int end = Cmd.find_first_of(" ", start+1) ;
	int length = (end == string::npos)? end : end - start - 1 ;
	int pipeID = stoi( Cmd.substr(start+1, length) ) ;
	
	/* The public pipe does not exist */
	if( !publicPipeFlag[ pipeID ] )
	{
		flag = false ;
		writer( Socket, "*** Error: the pipe #" + to_string(pipeID) + " does not exist yet. ***\n" ) ;
		return ;
	}	
	
	/* exists */
	publicPipeFlag[ pipeID ] = false ;
	string fifoName = "/tmp/fifo" + to_string(pipeID) ;
	mkfifo( fifoName.c_str(), PERM ) ;
	
	if( (inSock = open( fifoName.c_str(), O_RDONLY|O_NONBLOCK, S_IRWXU | S_IRWXG )) < 0 )
		err_mesg("fifo open error") ;
	
	unlink( fifoName.c_str() );
	
	/* Message */
	int clientID = userFinder( Socket ) ;
	writer( Socket, "*** " + (*usersPool)[clientID].Name + "(#" + to_string(clientID) + ") just received via '" + Cmd + "' ***\n") ;

	/* modifying cmd */
	length = (length == string::npos)? length : length + 1 ;
	Cmd.replace( start, length, "" ) ;		
}


bool RAS::outPublicPipeAnalyser(string Cmd)
{
	int pos = Cmd.find_first_of(">") ;
	return pos != string::npos && Cmd[pos+1] != ' ' ;
}


void RAS::outPublicPipeHandler(int Socket, string& Cmd, int& outSock, bool& flag)
{	
	/* parsing */
	int start = Cmd.find_first_of(">") ;
	int end = Cmd.find_first_of(" ", start+1) ;
	int length = (end == string::npos)? end : end - start - 1 ;
	int pipeID = stoi( Cmd.substr(start+1, length) ) ;
	
	/* The public pipe has already existed */
	if( publicPipeFlag[ pipeID ] )
	{
		flag = false ;
		writer( Socket, "*** Error: the pipe #" + to_string(pipeID) + " already exists. ***\n") ;
		return ;
	}	
	
	/* exists */
	publicPipeFlag[ pipeID ] = true ;
	string fifoName = "/tmp/fifo" + to_string(pipeID) ;
	mkfifo( fifoName.c_str(), S_IRUSR | S_IWUSR ) ;
	open(fifoName.c_str(), O_RDONLY | O_NONBLOCK);
	
	if( (outSock = open( fifoName.c_str(), O_WRONLY | O_NONBLOCK, S_IRWXU )) < 0 )
		err_mesg("fifo open error") ;
	
	/* Message */
	int clientID = userFinder( Socket ) ;
	writer( Socket, "*** " + (*usersPool)[clientID].Name + "(#" + to_string(clientID) + ") just piped '" + Cmd + "' ***\n") ;

	/* modifying cmd */
	length = (length == string::npos)? length : length + 1 ;
	Cmd.replace( start, length, "" ) ;	
}


int RAS::userFinder(int Socket)
{
	for(auto it=(*usersPool).begin(); it!=(*usersPool).end(); ++it)
		if( (it->second).Socket == Socket )
			return it->first ;
	return -1 ;	
}


void writer(int Socket, string mesg) 
{
	if( write(Socket, mesg.c_str(), mesg.length()) < 0 )
		perror("write error") ;
}

string cmdModifier(const char* buffer, int revLen)
{
	string revComd(buffer, revLen) ;
	revComd = revComd.substr(0, revComd.find_first_of("\r")) ;
	
	while( revComd.back() == '\n' ) 
		revComd.pop_back() ;

	return revComd ;
}

void spaceEraser(string& str)
{
	int pos = str.find_first_not_of(" ") ;
	str = str.substr(pos, string::npos) ;
	
	while( str.back() == ' ' )
		str.pop_back() ;
}


void foolProofer(int Socket)
{
	string errorMesg = "Error: support environment variable PATH only\n" ;
	writer( Socket, errorMesg ) ;	
}


void err_mesg(const char* mesg)
{
	fprintf(stderr, "%s\n", mesg) ;
	exit(0) ;
}
