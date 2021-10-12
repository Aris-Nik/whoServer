#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <errno.h>
#include <sstream>
#include <sys/types.h>
#include <stdio.h>
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h> 
#include <string.h>
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 
#include <pthread.h>
#include <semaphore.h>
#include <csignal>

using namespace std;

typedef struct worker_info{
	int port;
}worker_info;

typedef struct pool_data{
	int fd;
	char type;
}pool_data;

typedef struct pool_t{
	pool_data* data;
	int start;
	int end;
	int count;

}pool_t;

typedef struct portNode{
	int port;
	portNode *next;
}portNode;


class portList{
	private:
		portNode* head;
		portNode* tail;
	public:
		portList();
		~portList();
		void insert_port(int port);
		void print_list();
		string readAnswers(string query); 
		int length();
		int pop();
		string sendquery(string query);
};


void initialize(pool_t* pool);
void place(pool_t* pool, int data,char type);
pool_data obtain(pool_t* pool);
void* thread_create(void *arg);
string readMessage(int fd);
void signalHandler( int signum );
void sendMessage(int fd,string to_send);