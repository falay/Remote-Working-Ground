#include <stdlib.h>
#include <stdio.h>
#include "RWG.hpp"



int main(int argc, char* argv[])
{
	if( argc == 2 )
		RWG( atoi(argv[1]) ) ;
	else
		fprintf( stderr, "Usage: %s [port number]\n", argv[0] ) ;		
}