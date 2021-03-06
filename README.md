Project Name: Remote Working Ground (rwg)


It is designed as a chat-like systems, 
calledremote working systems (server only).  
In this system, users can meet with/work with/talk to/make friends.  
Basically, this system supports all functions in ras.  
In addition, all clients can see what all on-line users are working.

/////////////////////////
///// Detailed Spec /////
/////////////////////////

1. All commands in project ras, especially...

* Initial setting:  you have to do this when new client connects.

    a. Environment variables: remove all the environment variables except PATH
        e.g.,
    PATH=bin:.

    !!!!! remove all the environment variables except PATH !!!!!
    
    Notice that every client will have its own environment variables settings
    e.g.,
    [client A]
    % setenv PATH .
    % printenv PATH
    PATH=.
    
    [client B]
    % printenv PATH
    PATH=bin:.

    b. Executable files in "bin/"
        "ls", "cat": you should copy them from "/usr/bin/" or "/bin/"
        "noop", "removetag", "removetag0", "number": TA provides source code for you to build them
        There are 6 executable files in it. Nothing else.
    
    c. There are only 2 objects in the directory: "bin" and "test.html".
        that means if I command "ls", it will show...
        e.g.,
        % ls
        bin
        test.html
        %

* If a command is not found in environment variable "PATH", show message "Unknown command: [command]."
    e.g.
    % echo "kerker."
    Unknown command: [echo].                # It contains "[]."

* You don't need to write client by yourself, just use telnet.

* When you write something to file, like "ls > test1.txt", 
  if there is something in the file, please remove all of it.
  
  !!!! DO NOT append to the file !!!!
  
  that means 
  e.g.,
  % ls > test.html
  % ls > test.html
  % cat test.html
  bin
  test.html
  %
* You MUST use "exec" to run "ls", etc.  You MUST NOT use functions
  like "system()" or some other functions (in lib) to do the job.
  That is, you cannot use a function which will include "exec".
  
----------------------------------------------------------------------------------------------------
2. Commands [who], [tell], [yell], [name] are single line commands, which means there will be no
   pipe connected to these commands, just like [printenv] or [setenv].
   
   eg.
   % ls | yell   //It is illegal. This will not appear in testcases.
   % who |2      //It is illegal. This will not appear in testcases.
----------------------------------------------------------------------------------------------------
3. The output format of [who]:
   You have to print a tab between each of tags. 
   Notice that the first column does not print socket fd but client id.
   
   <ID>[Tab]<nickname>[Tab]<IP/port>[Tab]<indicate me>
   (1st id)[Tab](1st name)[Tab](1st IP/port)([Tab](<-me))
   (2nd id)[Tab](2nd name)[Tab](2nd IP/port)([Tab](<-me))
   (3rd id)[Tab](3rd name)[Tab](3rd IP/port)([Tab](<-me))
   ...   

   For example:
   % who 
   <ID>	<nickname>	<IP/port>	<indicate me>
   1	IamStudent	140.113.215.62/1201	<-me
   2	(no name)	140.113.215.63/1013
   3	student3	140.113.215.64/1302
   
   Notice that
   The client's id should be assigned in the range of number 1~30. 
   The server should always assign a smallest unused id to new connected client.
   
   eg.
   <new client login> // server assigns this client id = 1
   <new client login> // server assigns this client id = 2
   <client 1 logout>
   <new client login> // server assigns this client id = 1, not 3
----------------------------------------------------------------------------------------------------
4. Format of command [tell]: 
   % tell (client id) (message)
   And the client will get the message with following format:
   *** (sender's name) told you ***: (message)
   
   e.g., Assume my name is 'IamUser'.
   [terminal of mine]
   % tell 3 Hello World.
   
   [terminal of client id 3]
   % *** IamUser told you ***: Hello World.

   If the client you want to send message to does not exist, print the following message:
   *** Error: user #(client id) does not exist yet. *** 
   
   e.g.,
   [terminal of mine]
   % tell 3 Hello World.
   *** Error: user #3 does not exist yet. *** 
   % 
----------------------------------------------------------------------------------------------------
5. Format of command [yell]: 
   % yell (message)
   All the clients will get the message with following format:
   *** (sender's name) yelled ***: (message)
   
   e.g., Assume my name is 'IamUser'.
   [terminal of mine]
   % yell Hi everybody
   
   [terminal of all clients]
   % *** IamUser yelled ***: Hi everybody
----------------------------------------------------------------------------------------------------
6. Format of command [name]: 
   % name (name)
   All the clients will get the message with following format:
   *** User from (IP/port) is named '(name)'. ***
   
   eg.
   [terminal of mine]
   % name IamUser
   
   [terminal of all clients]
   % *** User from 140.113.215.62/1201 is named 'IamUser'. ***
   
   Notice that the name CAN NOT be the same as the name which on-line users have,
   or you will get the following message:
   *** User '(name)' already exists. ***
   
   e.g.,
   Mike is on-line, and I want to have the name "Mike" too.
   
   [terminal of mine]
   % name Mike
   *** User 'Mike' already exists. ***
   % 

----------------------------------------------------------------------------------------------------
7. When a new client connects to the server, the client does not have a name.
   So the broadcast message will be as follows. 
   *** User '(no name)' entered from (IP/port). ***
   The new client can also receive this message.
   See Notice 7 below for detail.
   
   e.g.
   [terminal of all clients]
   % *** User '(no name)' entered from 140.113.215.63/1013. ***
----------------------------------------------------------------------------------------------------
8. When a client disconnects from the server, broadcast as follows. 
   *** User '(name)' left. ***
   
   e.g.
   [terminal of all clients]
   % *** User 'student5' left. ***   
   
   You will receive left messeges after exit command
   e.g.
   My name is student.
   [terminal of mine]
   % exit
   *** User 'student' left. ***
   
----------------------------------------------------------------------------------------------------
9. Any client can use '>(id)' to pipe the standard output/error into a public pipe.
   When a client pipes something to others, broadcast the following message:
   *** (name) (#<client id>) just piped '(command line)' ***

   e.g. Assume my name is 'IamUser' and client id = 7.
   [terminal of mine]
   % cat test.html | cat >3
   
   [terminal of all clients]
   % *** IamUser (#7) just piped 'cat test.html | cat >3' *** 
----------------------------------------------------------------------------------------------------
10. If the public pipe exists already, show the following error message.
   
   e.g. 
   [terminal of mine]
   % cat test.html | cat >3
   *** Error: public pipe 3 already exists. ***
   % 
----------------------------------------------------------------------------------------------------
11. Any client can use '<(id)' to receive something from a public pipe.
   When a client received from a public pipe, broadcast the following message:
   *** (my name) (#<my client id>) just received via '(command line)' *** 

   e.g. Assume my name is 'IamUser' and client id = 4.
   [terminal of mine]
   % cat <3
   
   [terminal of all clients]
   *** IamUser (#4) just received via 'cat <3' ***
----------------------------------------------------------------------------------------------------
12. After closed by both sender/receiver, the public pipe is gone (does not exist).
	If public pipe does not exists, show the following error message.
	
	e.g. If public pipe 3 does not exists.
	[terminal of mine]
	% cat <3
    *** Error: public pipe 3 does not exist yet. ***
	%
----------------------------------------------------------------------------------------------------
	
[Notice]

1. When doing broadcast, all clients should receive the message, including me.

   eg. Assume my name is 'IamUser'.
   [terminal of mine]
   % yell Hi...
   *** IamUser yelled ***:  Hi...
   % 
---------------------------------------------------------------------------------------------------- 
2. The length of a client's name is at most 20 characters. 
   Also, there are only alphabet and digits in names.
----------------------------------------------------------------------------------------------------
3. If an user wants to receive a pipe and the pipe does not exist, he/she will receive the
   following error message:
   *** Error: the pipe #(id) does not exist yet. *** 
   
   A pipe can be taken only one time. After the pipe is taken, it is distroyed.
   If he/she tries to take it again, he/she will receive the error message above.

   e.g. Assume there are only 2 clients connect to server with client id 1, and 2.
   
   [terminal of client id = 1]
   % cat <5
   *** Error: the pipe #5 does not exist yet. *** 
   % ls >1
   
   [terminal of client id = 2]
   % cat <1

   [terminal of client id = 2]
   % cat <1
   *** Error: the pipe #1 does not exist yet. *** 
   % 
----------------------------------------------------------------------------------------------------
4. Before the pipe is taken, client cannot send to its pipe again.
   Otherwise, he/she will receive the following error message. 
   *** Error: the pipe #(id) already exists. *** 
   
   e.g.
   [terminal of client id = 2]
   % ls >1
   % ls >1
   *** Error: the pipe #1 already exists. *** 
   % 
----------------------------------------------------------------------------------------------------
5. When a client receive message from other client's pipe and then redirect the stdout into its pipe using ">n",
   Server will broadcast two messages to all clients.
   Server shows message about receiving pipe first, and then shows message about sending pipe.
   For example:
   
   eg. Assume my name is 'IamUser' and client id = 3.
       Assume there's something in public pipe 7.
   
   % cat <7 >1
   *** IamUser (#3) just received via 'cat <7 >1' ***
   *** IamUser (#3) just piped 'cat <7 >1' ***
   % 
   
   or
   
   % cat >1 <7
   *** IamUser (#3) just received via 'cat >1 <7' ***
   *** IamUser (#3) just piped 'cat >1 <7' *** 
   % 
----------------------------------------------------------------------------------------------------
6. Do not worry about synchronization problem.
----------------------------------------------------------------------------------------------------
7. Server sends "% " only when the client connected to it or current command finished.

   e.g. A possible situation
   ****************************************
   ** Welcome to the information server. **
   ****************************************
   *** User '(no name)' entered from 140.113.215.63/1013. ***
   % *** User '(no name)' entered from 140.113.215.64/1014. ***
   *** User '(no name)' entered from 140.113.215.65/1015. ***
   *** User '(no name)' entered from 140.113.215.66/1016. ***
   who
   <ID>	<nickname>	<IP/port>	<indicate me>
   1	(no name)	140.113.215.63/1013	<-me
   2	(no name)	140.113.215.64/1014
   3	(no name)	140.113.215.65/1015
   4	(no name)	140.113.215.66/1016
   % 
   
   The user connected to the server, showing his/her online message and "% ".
   3 other users also connected to the server before the user send first command, showing 3 online messages.
   The user entered 'who', showing online user information.
   After command 'who' finished, server send "% " to client.
   
   Notice the "% " position.
----------------------------------------------------------------------------------------------------

/////////////////////////
///// Requirements  /////
/////////////////////////

* Write two different server programs. 
  (1) Use the single-process concurrent paradigm.
  (2) Use the concurrent connection-oriented paradigm with shared memory.  

* For each client, all behaviors required by project 1 are still required
  in this project.  

Hints: 

* You can assume that the # of users is <= 30.

* You can assume that the # of public pipes is <= 100 

* ">3" or "<4" has no space between them.  So, you can distinct them 
  from "> filename" easily.  

* When you handle the pipe n , and you got public pipe error messege, you should consider that command
  as unknowned command (but don't need to show unknowned command messege).

e.g.
If public pipe 2 is not exists:
	
Case 1 :
	
	% ls |2
	% cat <2 |1		//error when receive public pipe, so this line will not be counted by previous ls.
	*** Error: the pipe #2 does not exist yet. *** 
	% ls
	% cat	// will receive stdin by ls in first line
	
Case 2 :
	
	% ls |2
	% ls | cat >2 	//error when receive public pipe, but first command ls is valid, so this line will be counted by previous ls
	*** Error: the pipe #2 does not exist yet. *** 
	% cat	// will receive by ls in first line


* The following commands will not appear in test cases:
(and you don't need to handle the error messeges)

1. 
	% ls |1
	% cat <5 |1
	% ...

	or 
	
	% ls | cat <5 | cat
	
	/*Every single command will not receive from previous pipe(ls) and public pipe(5) at the same time */

2.
	% cat <2 | cat | number >2
	
	/*In same line, the public pipe number of receive/send command won't be the same*/
			
	
3.
	% cat <1 <2
		or
	% cat >5 >6
	
	/*Invalid*/


* But the following commands are valid:

1.	
	If public pipe 2 is exists, but 3 is not:
	
	% cat <2 | cat | number >3
	*** C1 (#1) just received via 'cat <2 | cat | number >3' ***
	*** C1 (#1) just piped 'cat <2 | cat | number >3' ***
	%
	
2.	
	If public pipe 2 and 3 both exist:
	
	% cat <2 | cat | number >3
	*** C1 (#1) just received via 'cat <2 | cat | number >3' ***
	*** Error: the pipe #3 already exists. *** 
	%	
	
3.
	% ls |2 
	% cat <2 |1
	% number
	
	
* For the second program (2), 
  * One chat buffer has at most 10 unread messages, each of which has
    at most 1024 bytes. 
  
  * For each public channel, need to support a lock on it to prevent from others to access it.
  
  * For each pipe for ">n", use FIFO instead of pipe.
  
  * If a message has more than 1024 bytes, simply truncate it to 1024 bytes.
  
  * You can use signal "SIGUSR1" or "SIGUSR2" to help.
  
  * For "who", the master process maintain an id for each forked process. 
    Since there are no more than 30 processes, id <= 30 and let the id be client id.