#include "../Headers/whoServer.h"
#define BUFFERSIZE 50


pthread_mutex_t mtx=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mtx2=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_nonempty;
pthread_cond_t cond_nonfull;
pool_t* pool;
int POOL_SIZE;

portList* port_list;

struct sockaddr_in test_address;

pthread_t *tid;
int workers;

volatile sig_atomic_t sig_int = 0;

void signalHandler( int signum ) {
    if (signum == SIGINT || signum == SIGQUIT){
    	cout << "Mphka sto signal ahdner " << endl;
        sig_int = 1; 
    } 
}

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


void* thread_create(void *arg){
	pool_data temp;
	string ports ="";
	while(1){
		if (sig_int == 1){
			break;
		}
		pthread_mutex_lock(&mtx2);
		if (pool->count > 0){
			temp = obtain(pool);						

			pthread_cond_signal(&cond_nonfull);
			if (temp.type == 's'){											//s means statistics from worker and we also get the port from the statistics
				string out = readMessage(temp.fd);
				istringstream ss(out);
				string port_temp;		
				ss >> port_temp;											//first string on the statistics is the port we need for the socket creation and connection with the worker
				struct sockaddr_in worker_addr; 
				int sock_worker;
			    if ((sock_worker = socket(AF_INET, SOCK_STREAM,0)) < 0){
			    	cout << "Socket creation error" << endl;
			    	return  NULL;
			    }
			    test_address.sin_family = AF_INET;
			  
			    test_address.sin_port = stoul(port_temp.c_str());					

			    int err;
			    if (err = connect(sock_worker, (struct sockaddr *)&test_address,sizeof(struct sockaddr_in)) < 0){
			    	cout << "Cannot connect thread : [" << strerror(err) << "]" << endl;
					cout << "Connection failed" << endl;
					return NULL;
				} 
			    port_list->insert_port(sock_worker);						//save the connection with worker to a list so we gather them all together to send them the queries							
			}
			else if (temp.type == 'q'){									//q means got a query 
				string out = readMessage(temp.fd);
				
				string answer = port_list->sendquery(out);
			    

				string answer1 = port_list->readAnswers(out);
				cout << "\n" + answer1 + "\n";
				sendMessage(temp.fd,answer1);
				//send(temp.fd,answer1.c_str(),answer1.length(),0);
				//string dolla = "$";	
				//send(temp.fd,dolla.c_str(),1,0);
				
			}
		}
		pthread_mutex_unlock(&mtx2);
	}
}

int main(int argc, char *argv[]){
	signal(SIGINT,signalHandler);
	signal(SIGQUIT,signalHandler);
	int stat_fd, new_socket, valread;
	int queries_fd;
	fd_set readfds;


	int queryPortNum = atoi(argv[2]);
	int statisticsPortNum = atoi(argv[4]);
	int numThreads = atoi(argv[6]);
	int bufferSize = atoi(argv[8]);
	POOL_SIZE = bufferSize;
	pool = new pool_t();
	port_list = new portList();
	initialize(pool);
	
	pool->data = new pool_data[bufferSize];
	
	int err;

	tid = new pthread_t[numThreads];
	for (int i = 0; i < numThreads; i++){
		err = pthread_create(&(tid[i]),NULL, &thread_create, NULL);
		if (err != 0)
			cout << "Cannot create thread : [" << strerror(err) << "]" << endl;
	}

	struct sockaddr_in statistics_addr;
	struct sockaddr_in queries_addr;
	int opt = 1;
	int stat_addr_len = sizeof(statistics_addr);
	int quer_addr_len = sizeof(queries_addr);
	char buffer[2];
	memset(buffer,0,1);

	string hello = "Hello from server";

	if ((stat_fd = socket(AF_INET, SOCK_STREAM,0)) == 0){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	statistics_addr.sin_family = AF_INET;
	statistics_addr.sin_addr.s_addr = INADDR_ANY;
	statistics_addr.sin_port = htons(statisticsPortNum);

	//test_address = statistics_addr;

	if (bind(stat_fd,(struct sockaddr *)&statistics_addr,sizeof(statistics_addr)) < 0){
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(stat_fd,3) < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}

	if ((queries_fd = socket(AF_INET,SOCK_STREAM,0)) == 0){
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	queries_addr.sin_family = AF_INET;
	queries_addr.sin_addr.s_addr = INADDR_ANY;
	queries_addr.sin_port = htons(queryPortNum);


	if (bind(queries_fd,(struct sockaddr *)&queries_addr,sizeof(queries_addr)) < 0){
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	if (listen(queries_fd,3) < 0){
		perror("listen");
		exit(EXIT_FAILURE);
	}
	

	worker_info* worker_ports;
	//worker_ports = new 
	workers = 0;
	string ports="";
	int status;
	int maxfd;
	if (stat_fd > queries_fd)
		maxfd = stat_fd;
	else 
		maxfd = queries_fd;
	string all_queries="";
	int exitflag = 0;
	while(1){
		if (sig_int == 1)
			break;
		FD_ZERO(&readfds);
		FD_SET(stat_fd,&readfds);
		FD_SET(queries_fd,&readfds);
		status = select(maxfd + 1, &readfds, NULL, NULL, NULL);
		if (status < 0){
			break;
		}
		else if (status > 0){
			if (FD_ISSET(stat_fd,&readfds)){
				if ((new_socket = accept(stat_fd,(struct sockaddr *)&statistics_addr,(socklen_t*)&stat_addr_len)) < 0){
					perror("accept");
					exit(EXIT_FAILURE);
				}
				test_address = statistics_addr;								//global struct sockaddr_in so we threads can see it too
				place(pool,new_socket,'s');									//s stands for stats
				pthread_cond_signal(&cond_nonempty);
				usleep(0);	
			}
			if(FD_ISSET(queries_fd,&readfds)){
				if ((new_socket = accept(queries_fd,(struct sockaddr *)&queries_addr,(socklen_t*)&quer_addr_len)) < 0){
					perror("accept");
					exit(EXIT_FAILURE);
				}
				place(pool,new_socket,'q');									//q stands for query
				pthread_cond_signal(&cond_nonempty);
				usleep(0);
			}
		}
	}
	for (int i = 0; i < numThreads; i++){
    	pthread_join(tid[i],NULL);
    }
    close(stat_fd);
    close(queries_fd);
    delete[] tid;
	delete[] pool->data;
	delete pool;
	delete port_list;
	cout << "exiting..." << endl;
	return 0;
}

void initialize(pool_t* pool){
	pool->start = 0;
	pool->end = -1;
	pool->count = 0;
}

void place(pool_t* pool, int fd, char type){
	pthread_mutex_lock(&mtx);
	while (pool->count >= POOL_SIZE){
		cout << "Buffer is full" << endl;
		pthread_cond_wait(&cond_nonfull, &mtx);
	}
	pool->end = (pool->end + 1) % POOL_SIZE;
	pool->data[pool->end].fd = fd;
	pool->data[pool->end].type = type;
	pool->count = pool->count + 1;
	pthread_mutex_unlock(&mtx);
}

pool_data obtain(pool_t* pool){
	pool_data ret;
	pthread_mutex_lock(&mtx);
	while (pool->count <= 0){
		cout << "Buffer is Empty" << endl;
		pthread_cond_wait(&cond_nonempty, &mtx);
	}
	//data = pool->data[pool->start];
	ret = pool->data[pool->start];
	pool->start = (pool->start + 1) % POOL_SIZE;
	pool->count--;
	pthread_mutex_unlock(&mtx);
	return ret;
}


portList::portList(){
	head = NULL;
	tail = NULL;
}

portList::~portList(){
	
	portNode* temp = this->head;
	while (temp != NULL){
		close(temp->port);
		portNode* temp1 = temp;
		temp = temp->next;
		delete temp1;
	}
}

void portList::insert_port(int port){
	portNode* temp = new portNode;
	temp->port = port;
	temp->next = NULL;

	if (head == NULL){
		head = temp;
		tail = temp;
	}
	else{
		tail->next = temp;
		tail = tail->next;
	}
}

void portList::print_list(){
	portNode* temp = this->head;
	int i = 0;
	while (temp != NULL){
		cout << "Position " << i << " has port : " << temp->port << endl;
		i++;
		temp = temp->next;
	}
}

int portList::length(){
	portNode* temp = this->head;
	int i = 0;
	while (temp != NULL){
		i++;
		temp=temp->next;
	}
	return i;
}

int portList::pop(){
	if (this->head == NULL)
		return -1;
	portNode* temp = this->head;
	this->head = this->head->next;
	int ret = temp->port;
	delete temp;
	return ret;
}

string portList::sendquery(string query){
	char dolla_buf[2] = {0};
	strcpy(dolla_buf,"$");
	portNode* temp = this->head;
	while(temp != NULL){

		//send(temp->port,query.c_str(),query.length(),0);
		//send(temp->port,dolla_buf,1,0);
		sendMessage(temp->port,query);
		temp = temp->next;
	}
	return "kati";
}

string portList::readAnswers(string query){
	string answer = "";
	portNode* temp = this->head;
	istringstream ss(query);
    string answer1;
    ss >> answer1;
    if (answer1 == "/diseaseFrequency" || answer1 == "/numPatientAdmissions" || answer1 == "/numPatientDischarges"){
        
        int sum = 0;
        while (temp != NULL){
			string answer1;
			answer1 = readMessage(temp->port);
    		//cout << "to answer einai " << answer1 << endl;
			sum = sum + atoi(answer1.c_str());
			//cout << "diavasa apo worker " << answer1;
			temp = temp->next;
		}
		answer = to_string(sum);
    }
   // else if (answer1 == "/numPatientAdmissions" || answer1 == "/numPatientDischarges"){

    //}
    else{
    	while (temp != NULL){
			string answer1;
			answer1 = readMessage(temp->port);
			if (answer1 != "")
				answer = answer1;
    		//cout << "to answer einai " << answer1 << endl;
			temp = temp->next;
		}
    	//string answer1;
		//answer = readMessage(temp->port);
    }
    if (answer1 == "/searchPatientRecord" && answer == "")
    	answer = "Record id not found";
	//cout << "THA EPISTREPSW " << answer << endl;
	answer = query + "\n" + answer;
	return answer;
} 