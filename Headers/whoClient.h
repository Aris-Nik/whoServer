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

using namespace std;

typedef struct queryNode{
	string query;
	queryNode *next;
}queryNode;


class queryList{
	private:
		queryNode* head;
		queryNode* tail;
	public:
		queryList();
		~queryList();
		void insert_query(string query);
		void print_list();
		int length();
		string pop();
};
string readMessage(int fd);
void sendMessage(int fd,string to_send);