#ifndef RWG_HPP
#define RWG_HPP

#include "RAS/RAS.hpp"
#include <iostream>
#include <string>
#include <map>
using namespace std ;

#define QLEN 5
#define BUFSIZE 1024



class RWG
{
	public:
		RWG(int);
		int clientConstructor(int, struct sockaddr_in) ;
		int passiveTCP(int) ;
		void welcomeMesger(int) ;
		
	private:	
		map<int,User> usersPool ; // ID -> User data
		RAS rasShell ;
} ;


void err_mesg(string) ;


#endif