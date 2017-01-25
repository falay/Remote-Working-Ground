#include "RWG.hpp"
#include <sys/select.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/wait.h>

using std::map ;



RWG::RWG(int port) : rasShell(usersPool) 
{
	fd_set activeFDs, readableFDs ;

	int masterSocket = passiveTCP( port ) ;
	int clientSocket, maxID = 0 ;
	struct sockaddr_in clientAddr ;

	FD_ZERO( &activeFDs ) ;
	FD_SET( masterSocket, &activeFDs ) ;

	if( chdir( "./ras" ) < 0 )
		err_mesg("chdir error") ;
	
	while( true )
	{
		memcpy( &readableFDs, &activeFDs, sizeof(readableFDs) ) ;
		
		if( select( getdtablesize(), &readableFDs, NULL, NULL, NULL ) < 0 )	
			err_mesg("select error") ;

		if( FD_ISSET( masterSocket, &readableFDs ) )
		{
			socklen_t clientLen = sizeof( clientAddr ) ;
			if( (clientSocket = accept( masterSocket, (struct sockaddr*)&clientAddr, &clientLen )) < 0 ) 
				err_mesg("accept error") ;
		
			FD_SET( clientSocket, &activeFDs ) ;	
			
			string welcome = "****************************************\n** Welcome to the information server. **\n****************************************\n% " ;	
			writer( clientSocket, welcome ) ;
			
			int clientID = clientConstructor( clientSocket, clientAddr ) ;
			maxID = max( maxID, clientID ) ;
			welcomeMesger( clientID ) ;
		}	
		
		
		for(int id=1; id<=maxID; id++)
		{
			if( usersPool.count( id ) )
			{
				int curSocket = usersPool[id].Socket ;
				if( curSocket!=masterSocket && FD_ISSET(curSocket, &readableFDs) )
				{				
					if( !rasShell.shell( id ) )
					{
						rasShell.broadcaster("*** User '" + usersPool[id].Name + "' left. ***") ;
						close( curSocket ) ;
						FD_CLR( curSocket, &activeFDs ) ;
						usersPool.erase( id ) ;	
					}	
				}	
			}		
		}			
	}	
}


void RWG::welcomeMesger(int ID)
{
	User thisClient = usersPool[ ID ] ;
	string welcomeMesg = "*** User '(no name)' entered from " + thisClient.IP + "/" + to_string( thisClient.port ) + ". ***" ;
	rasShell.broadcaster( welcomeMesg ) ;	
}



int RWG::clientConstructor(int clientSocket, struct sockaddr_in clientAddr)
{
	int mapSize = usersPool.size() ;
	int newID = mapSize + 1 ;
	
	for(int id=1; id<=mapSize; id++)
	{
		if( !usersPool.count(id) )
		{
			newID = id ;
			break ;
		}	
	}	
	
	char IPstr[ INET_ADDRSTRLEN ] ;
	struct sockaddr_in* IPV4 = (struct sockaddr_in*)& clientAddr ;
	inet_ntop( AF_INET, &IPV4->sin_addr, IPstr, INET_ADDRSTRLEN ) ;

	string IP = string( IPstr, strlen(IPstr) ) ;
	int port = ntohs( IPV4->sin_port ) ;
	
	usersPool[ newID ] = User( clientSocket, IP, port, "(no name)" ) ;

	return newID ;
}


int RWG::passiveTCP(int port)
{
	struct protoent* ppe ;
	struct sockaddr_in serverAddr ;
	int masterSocket ;
	
	if( (ppe = getprotobyname("tcp")) == NULL )
		err_mesg("getprotobyname error") ;
	
	if( (masterSocket = socket(AF_INET, SOCK_STREAM, ppe->p_proto)) < 0 )
		err_mesg("socket error") ;
	
	bzero( (char*)&serverAddr, sizeof(serverAddr) ) ;
	serverAddr.sin_family 		= AF_INET ;
	serverAddr.sin_addr.s_addr 	= htonl( INADDR_ANY ) ;
	serverAddr.sin_port			= htons( port ) ;	
	
	if( bind(masterSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0 )
		err_mesg("bind error") ;
	
	listen(masterSocket, QLEN) ;
	
	return masterSocket ;
}


void err_mesg(string error)
{
	cerr << error << endl ;
	exit(0) ;
}