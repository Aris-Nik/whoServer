#include "../Headers/whoClient.h"
#define BUFFERSIZE 50


pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t cond_mutex = PTHREAD_MUTEX_INITIALIZER;
//pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;


pthread_t *tid;

int serverPort;
string serverIP;

int threads_left;

queryList* list;

string readMessage(int fd){
	string out ="";
	char buffer[BUFFERSIZE + 1] = {0};
	int valread;
	while ((valread = read(fd,buffer,BUFFERSIZE)) > 0){
		
		buffer[valread] = '\0';
		string temp123 = buffer;
		if (temp123 == "$"){
			break;
		}
		out = out + buffer;  
	}
	return out;
}

void sendMessage(int fd,string to_send){
    char buf[BUFFERSIZE + 1] = {0};
    int message_length = to_send.length();
    int times = message_length/BUFFERSIZE;
    if (message_length % BUFFERSIZE != 0 || times == 0)
        times++;
    string temp;
    for (int j = 0;j<times;j++){
        if (BUFFERSIZE < to_send.length()){
            temp = to_send.substr(0,BUFFERSIZE);
            to_send = to_send.substr(BUFFERSIZE,to_send.length() - BUFFERSIZE);
        }
        else{
            temp = to_send.substr(0,to_send.length());
            to_send = "";
        }
        
        strcpy(buf,temp.c_str());
        write(fd,buf,BUFFERSIZE);
    }
    string dolla = "$";
    strcpy(buf,dolla.c_str());
    write(fd,buf,1);
}

void* thread_create(void *arg)
{
	
	pthread_mutex_lock(&cond_mutex);

	
	
	queryList* thread_list = new queryList();
    unsigned long i = 0;
    pthread_t id = pthread_self();

	int numQueries = list->length();
							//if this thread has no queries to send then we dont need it
	
	for (int i = 0;( i < numQueries / threads_left); i++){
		string q = list->pop();
		if (q != "EOF")
			thread_list->insert_query(q);
	}
	if (numQueries % threads_left > 0){
		string q = list->pop();
		if (q != "EOF")
			thread_list->insert_query(q);
	}
	threads_left--;

	if (threads_left == 0){										//if we are on the last thread then broadcast all threads to start 
		pthread_cond_broadcast(&cond);
	}
	else{
		pthread_cond_wait(&cond,&cond_mutex);
	}														//else wait until all threads gather their queries and when the "last" one gathers them they will start all together
	
    while (thread_list->length() > 0){
    
		string query = thread_list->pop();
		
		int sock = 0;
	   	struct sockaddr_in serv_addr; 
	    
	   
	    if ((sock = socket(AF_INET, SOCK_STREAM,0)) < 0){
	    	cout << "Socket creation error" << endl;
	    	return  NULL;
	    }

	    serv_addr.sin_family = AF_INET;
	    serv_addr.sin_port = htons(serverPort);

	    if (inet_pton(AF_INET,serverIP.c_str(),&serv_addr.sin_addr) <= 0){
	    	cout << "Invalid address or not support" << endl;
	    	return NULL;
	    }
		
		if (connect(sock, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
			cout << "Connection failed" << endl;
			return NULL;
		}   
		string dollar = "$";
		//cout << "prin apo to send tha steilw to " << query << endl;
		sendMessage(sock,query);
		//send(sock,query.c_str(),query.length(),0);
		//send(sock,dollar.c_str(),1,0);
		int valread;
		string out = readMessage(sock);
		char buffer[2] = {0};
		
		cout << "\n" + out + "\n";
		//cout << "Got feedback from server : " + out << endl;

	    close(sock);

		pthread_mutex_unlock(&cond_mutex);


	}
	delete thread_list;
	thread_list = NULL;
    return NULL;
}



int main(int argc, char *argv[]){

	pthread_mutex_lock(&cond_mutex);


	char queryFile[30];
	strcpy(queryFile,argv[2]);

	int numThreads = atoi(argv[4]);
	
	tid = new pthread_t[numThreads];
	list = new queryList();
	
	serverPort = atoi(argv[6]);
	serverIP = argv[8];

	int err;
	

	//Gathering queries on a list
	ifstream textfile(queryFile);
	int i = 0;
	int count_of_queries = 0;
	if (textfile.is_open() && textfile.good()){
        string line = "";
        while( getline(textfile, line)){   
        	i++;
        	count_of_queries++;
        	string temp123 = line;
        	istringstream ss2(temp123);
        	ss2 >> temp123;		
        	if (temp123.length() > 1)	//Ignore new line lines (if we have a lot of new lines at the end of the txt )
        		list->insert_query(line);
        }
        textfile.close();
        
    }
    else{
        cout << "Failed to open file" << endl;
    }

    if (numThreads > count_of_queries)						//we dont need more threads that queries
    	numThreads = count_of_queries;
   
   	threads_left = numThreads;
    
    for (int i = 0; i < numThreads; i++){
		//*arg = i;
		
		err = pthread_create(&(tid[i]),NULL, &thread_create, NULL);
		if (err != 0)
			cout << "Cannot create thread : [" << strerror(err) << "]" << endl;
		else
			cout << "Thread created successfully with index " << i << " and id " << tid[i]  << endl;
	}

    pthread_mutex_unlock(&cond_mutex);


    for (int i = 0; i < numThreads; i++){
    	pthread_join(tid[i],NULL);
    }
    
    delete[] tid;

    delete list;

    return 0;
    
}



