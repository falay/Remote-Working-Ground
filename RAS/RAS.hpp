#ifndef RAS_HPP
#define RAS_HPP


#include "Pipe.hpp"
#include <map>

using std::string ;
using std::vector ;
using std::map ;

#define PERM 	0666
#define BUFSIZE 1024
#define QLEN    5


struct User
{
	int Socket ;
	string IP ;
	int port ;
	string Name ;
	string PATH ;
	PipePool pool ;
	
	User() {} ;
	User(int fd, string ip, int p, string n) : Socket(fd), IP(ip), port(p), Name(n), PATH("bin:.") {}
} ;





class RAS
{
	public:
		RAS(map<int,User>& users) : usersPool(&users) {} ;
		
		/* shell related subroutines */
		bool shell(int) ;
		void processor(int,string) ;
		char** formatChanger(string) ;
		void cmdExecutor(int,string) ;
		int redirectHandler(string&,int) ;
		vector<string> parser(string) ;
		bool socketCloser(int, int, int) ;
		bool binFinder(string) ;
		
		/* built-in command related subroutines */
		int builtInAnalyser(string) ;
		void builtInHandler(int,int,string) ;
		void printenvHandler(int,string) ;
		void setenvHandler(int,string) ;
		void whoHandler(int,string) ;
		void nameHandler(int, string) ;
		void yellHandler(int, string) ;
		void tellHandler(int, string) ;		
		void broadcaster(string) ;

		/* public pipe handling subroutines */
		bool inPublicPipeAnalyser(string) ;
		void inPublicPipeHandler(int,string&,int&,bool&) ;
		bool outPublicPipeAnalyser(string) ;
		void outPublicPipeHandler(int,string&,int&,bool&) ;		
		int userFinder(int) ;
		
	private:
		map<int,User>* usersPool ;	
		map<int,bool> publicPipeFlag ; /* ID->T/F: false:not exists, true:exists */
} ;


void writer(int,string) ;
string cmdModifier(const char*, int) ;
void foolProofer(int) ;
void spaceEraser(string&) ;
void err_mesg(const char*) ;



#endif