#include "../Headers/ex3.h"



int global_numWorkers = 0;
string* countries_parent;
int* pids;
int glob_buff_size;
char* in_dir;
int* country_pids;
int number_of_countries;
int *fd1;
pid_t global_pid;


volatile sig_atomic_t sig_child_death = 0;
volatile sig_atomic_t sig_flag = 0;

void signalHandler( int signum ) {


	if (signum == SIGINT ){
		
		close(0);													//close stdin because it will stuck on get line and we dont want that 
		signal(SIGINT,SIG_IGN);
		sig_flag = 1;
	}
	if (signum == SIGQUIT){
		
		close(0);													//close stdin because it will stuck on get line and we dont want that 
		signal(SIGQUIT,SIG_IGN);
		sig_flag = 1;
	}

}

void child_death(int signum){

	pid_t p;
    int status;

    while ((p=waitpid(-1, &status, WNOHANG)) != -1)
    { 
    	if (p == 0)
    		return;
    	else if (p == -1)
    		return;
    	else{

    		sig_child_death = 1;
    		global_pid = p;
    	}
    }
}


void new_worker(pid_t p){
	int target_index;
	for (int i = 0;i < global_numWorkers; i++){
		if (p == pids[i]){
			target_index = i;
			break;
		}
	}
	string str1="pipe" + to_string(target_index);
	
	close(fd1[target_index]);
	unlink(str1.c_str());
	
	if (mkfifo(str1.c_str(), 0666) < 0)
        perror("mkfifo1");
    
   	int temp_index = target_index;
	pids[target_index] = fork();
	char buf[glob_buff_size];

	if (pids[target_index] == 0){									//child
		worker(temp_index,str1,glob_buff_size,in_dir);
		exit(0);
	} 					
	else{															//parent process
		if ((fd1[target_index] = open(str1.c_str(), O_WRONLY)) < 0)
	        	perror("parent - open");
		string countries_string="";
		
	    for (int j = 0;j < number_of_countries;j++){
	   		if (country_pids[j] == target_index){			      		
	   			countries_string = countries_string +" "+ countries_parent[j];	
	        }
	    }
	    int message_length = countries_string.length();
	    int times = message_length/glob_buff_size;
	    if (message_length % glob_buff_size != 0 || times == 0)
	       	times++;
	    string temp;
	    for (int j = 0;j<times;j++){
	   		if (glob_buff_size < countries_string.length()){
	        	temp = countries_string.substr(0,glob_buff_size);
	        	countries_string = countries_string.substr(glob_buff_size,countries_string.length() - glob_buff_size);
	        }
	        else{
	        	temp = countries_string.substr(0,countries_string.length());
	        	countries_string = "";
	        }
	        
	        strcpy(buf,temp.c_str());
	        write(fd1[temp_index],buf,glob_buff_size);
	    }
	    string dolla = "$";
	    strcpy(buf,dolla.c_str());
	    write(fd1[temp_index],buf,glob_buff_size);
	    

	    char out[glob_buff_size];
	    string message="";
	    int num;

	}

}


int main(int argc, char *argv[]){
	char fff[2]="$";
	int numWorkers = atoi(argv[2]);
	int bufferSize = atoi(argv[4]);
	string serverIP = argv[6];
	string serverPort = argv[8];

	char input_dir[30];
	
	strcpy(input_dir,argv[10]);
	in_dir = argv[6];					//global input_dir
	global_numWorkers = numWorkers;
	glob_buff_size = bufferSize;
	cout << "Workers are " << numWorkers << " buffer size is " << bufferSize << " server ip is " << serverIP << " serverPort is " << serverPort << " and input directory is " << input_dir << endl;
	DIR *dir;
	dir = opendir(input_dir);
	struct dirent *entry;
	if (!dir){
		cout << "Directory not found\n";
		return 0;
	}

	number_of_countries = 0;
	
	while ((entry = readdir(dir)) != NULL){
		if (entry->d_name[0] != '.'){
			
			number_of_countries++;
		}
	}

	
	countries_parent = new string[number_of_countries];
	int x = 0;
	closedir(dir);
	dir = opendir(input_dir);
	while ((entry = readdir(dir)) != NULL){
		if (entry->d_name[0] != '.'){
			countries_parent[x++] = entry->d_name;
			
		}
	}
	closedir(dir);
	if (number_of_countries < numWorkers)					//we dont need more workers than subdirectories because they will do nothing
		numWorkers = number_of_countries;

	country_pids = new int[number_of_countries];
	int temp_worker = 0;

	for (int i = 0; i < number_of_countries;i++){
		country_pids[i] = temp_worker++;
		if (temp_worker == numWorkers)
			temp_worker = 0;
	}





    int num, fd;
    char* buf = new char[bufferSize + 1];									//need one more byte for terminal char '\0'
    memset(buf,0,bufferSize);																			//initialize buffer because valgrind complains
    fd1 = new int[numWorkers];
    pid_t pid;
    int temp_index;
    string dolla ="$";
    string str1[numWorkers];
    string str2[numWorkers];
    pids = new int[numWorkers];
    for (int i=0;i<numWorkers;i++){
    	str1[i] = "pipe" + to_string(i);									//PtW stands for Parent to worker (parent will write to worker on this pipe)
   		if (mkfifo(str1[i].c_str(), 0666) < 0)
        	perror("mkfifo1");
		temp_index = i;
		pids[i] = fork();
		pid = pids[i];
		if (pid == 0)
			break;
		
	}

    
    if (pid == 0)
    {
    	delete[] fd1;											//delete duplicated allocations because child does not need them
    	delete[] country_pids;
	 	delete[] countries_parent;
	 	delete[] pids;
	 	delete[] buf;

       	worker(temp_index,str1[temp_index],bufferSize,input_dir);
       	
        exit(0);

        
        
    }
    else
    {	signal(SIGCHLD,child_death);
    	signal(SIGINT,signalHandler);
		signal(SIGQUIT,signalHandler);
		
		
        for (int i=0;i<numWorkers;i++){
        	string countries_string=serverIP + " " + serverPort;
        	
	       
        
	        if ((fd1[i] = open(str1[i].c_str(), O_WRONLY)) < 0)
	        	perror("parent - open");
	        for (int j = 0;j < number_of_countries;j++){
	        	if (country_pids[j] == i){			      		
	        		countries_string = countries_string +" "+ countries_parent[j];	
	        	}

	        }
	        int message_length = countries_string.length();
	        int times = message_length/bufferSize;
	        if (message_length % bufferSize != 0 || times == 0)
	        		times++;
	        string temp;
	        for (int j = 0;j<times;j++){
	        	if (bufferSize < countries_string.length()){
	        		temp = countries_string.substr(0,bufferSize);
	        		countries_string = countries_string.substr(bufferSize,countries_string.length() - bufferSize);
	        	}
	        	else{
	        		temp = countries_string.substr(0,countries_string.length());
	        		countries_string = "";
	        	}
	        	strcpy(buf,temp.c_str());
	        	

	        	write(fd1[i],buf,bufferSize);
	        }
	        strcpy(buf,"$");
	        write(fd1[i],buf,1);
	       

	      
    	}
    	//for (int i=0;i<numWorkers;i++)
    	//	wait(0);
    	
    	char out[bufferSize];
        
    	
    	
    	//QUERIES
    	string answer;
    	string answer1;
    	//cout << "Give option :) " << endl;
    	
    	//int success = 0;
    	//int fail = 0;
    	while(1){
    		if (sig_child_death == 1)
    			new_worker(global_pid);
	    	if (sig_flag == 1){
	    		for (int i = 0; i < numWorkers; i++){
					kill(pids[i],SIGKILL);	
				}
	    		break;
	    	}
    	}
 	}
 	delete[] country_pids;
 	delete[] countries_parent;
 	delete[] pids;
 	delete[] buf;
 	
    for (int i=0;i<numWorkers;i++){
    	close(fd1[i]);
    	
    	cout << "Unlinking" << str1[i] << endl;
    	unlink(str1[i].c_str());
	}
	delete[] fd1;
    return 0;
}