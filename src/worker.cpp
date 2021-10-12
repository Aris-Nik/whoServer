#include "../Headers/ex3.h"
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h> 

#define BUFFERSIZE 50


int count_of_countries = 0;
string* countries;
List** records_list;
bucketList** buckets;
string global_input_dir;
string** files;
int* count_of_files_array;
int fd1_worker;
int global_buffer_size;
int new_sock_worker;


volatile sig_atomic_t sig_int = 0;
volatile sig_atomic_t sig_quit = 0;
volatile sig_atomic_t sig_usr1 = 0;


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

void signalHandler_worker( int signum ) {
    if (signum == SIGINT || signum == SIGQUIT){
        close(new_sock_worker);

        sig_quit = 1;
        
    } 
    else if (signum == SIGUSR1){                                       
        sig_usr1 = 1; 
    }
}


bool isFileInside(int country,string name){
    for (int i = 0; i < count_of_files_array[country]; i++)
        if (name == files[country][i])
            return 1;
    return 0;
}


int worker(int index,string pipe,int bufferSize,char* input_dir){
	
    signal(SIGINT,signalHandler_worker);
    signal(SIGQUIT,signalHandler_worker);
    signal(SIGUSR1,signalHandler_worker);
    signal(SIGPIPE,SIG_IGN);                                        //ignore sigpipe in case of server sudden close
	int fd;
    global_buffer_size = bufferSize;
	int num;
     
    global_input_dir = input_dir;
   

    long int success = 0;
    long int fail = 0;
   
    string dolla = "$";


    if ((fd1_worker = open(pipe.c_str(), O_RDONLY)) < 0)
    	perror("child - open");
                                                                                                               
    int i = 0;
   
    int flag=0;

    char out[bufferSize + 1];
    
    char buf[bufferSize + 1];                                                                   //need one more byte for terminal char '\0'
    memset(buf,0,bufferSize);                                                                           //initialize buffer because valgrind complains
    string message="";
    string temp123;
    while ((num = read(fd1_worker,out,bufferSize)) > 0){
        out[num] = '\0';
        temp123 = out;
        if (temp123 == "$")
            break;
        message = message + out; 

    }
 
    int a = 0;
    string word1 = "";
    istringstream ss1(message);
    
    while (ss1 >> word1){
        count_of_countries++;                                                           //count the countries parent gave to this specific worker
    }
    count_of_countries = count_of_countries - 2;
    countries = new string[count_of_countries];
    int i_temp = 0;
    word1 = "";
    istringstream ss2(message);
    string serverIP;                                                                    //take the serverIp
    string serverPort;                                                                  //take the serverPort
    ss2 >> serverIP;
    ss2 >> serverPort;
    while (ss2 >> word1){
        countries[i_temp++] = word1;                                                                //create an array of countries a worker has to handle
    }
    
    
    struct dirent *entry;
    DIR *dir;

    

    
    char dir_name[40];
    records_list = new List*[count_of_countries];                                              //for every country a worker has we have a list that we keep all patients
    buckets = new bucketList*[count_of_countries];                                          //for every country a worker has we have a list of diseases that contain a list of patients with that disease
   
    count_of_files_array = new int[count_of_countries];
    files = new string*[count_of_countries];

    string stats="";
    for (int country_index = 0; country_index < count_of_countries; country_index++){
        struct dirent *entry;
        
        DIR *dir;
        sprintf(dir_name,"%s/%s",input_dir,countries[country_index].c_str());
        dir = opendir(dir_name);
        if (!dir)
            cout << "Directory not found with name " << input_dir << "/" << countries[country_index] << endl;
        int count_of_files=0;
        while ((entry = readdir(dir)) != NULL){
            if (entry->d_name[0] != '.'){        
               
                count_of_files++;
            }
        }
        closedir(dir);
        files[country_index] = new string[count_of_files];
        count_of_files_array[country_index] = count_of_files;
        dir = opendir(dir_name);
        int c = 0;
        while ((entry = readdir(dir)) != NULL){
            if (entry->d_name[0] != '.'){        
                files[country_index][c++] = entry->d_name;                                                                 //insert files (DD-MM-YYYY) on a array to sort them
            }
        }

        for (c = 0; c < count_of_files;c++){                                                            //order them by chronological order
            int minValue = totalDays(files[country_index][c]);
            int minj = c;
            for (int j = c + 1; j < count_of_files; j++ ){
                if (totalDays(files[country_index][j]) < minValue){
                    minValue = totalDays(files[country_index][j]);
                    minj = j;
                }
            }
            string temp = files[country_index][c];
            files[country_index][c] = files[country_index][minj];
            files[country_index][minj] = temp;
            
        }

        records_list[country_index] = new List();
        buckets[country_index] = new bucketList(100,countries[country_index]);                                                   //100 capacity
        

        closedir(dir);
        for (c = 0; c < count_of_files; c++){
            string age_range="";
            char* file_txt = new char[100];
            sprintf(file_txt,"%s/%s/%s",input_dir,countries[country_index].c_str(),files[country_index][c].c_str());
        
            ifstream textfile(file_txt);
            delete[] file_txt;
           
            if (textfile.is_open() && textfile.good()){

                string line = "";

                while( getline(textfile, line)){   
                    Record* rec = new Record();
                    istringstream ss(line);
                    string word;
                        ss >> word;
                        rec->recordID = word;
                        ss >> word;
                        rec->status = word;
                        ss >> word;
                        rec->firstName = word;
                        ss >> word;
                        rec->lastName = word;
                        ss >> word;
                        rec->disease = word;
                        ss >> word;
                        rec->age = word;
                        rec->country = countries[country_index];
                        
                        string temp100 = files[country_index][c].substr(0, 10);
                        rec->date = temp100;
                        if (rec->status == "EXIT"){

                            if (records_list[country_index]->canEnter(rec)){
                                records_list[country_index]->insertNode(rec);
                                buckets[country_index]->insert(rec);
                            }
                            else{
                                //fprintf(stderr, "ERROR\n");
                                
                                delete rec;
                            }
                        }else{

                            records_list[country_index]->insertNode(rec);

                            buckets[country_index]->insert(rec);
                        }

                            

                }
                textfile.close();
            }
            else{
                cout << "Failed to open file" << endl;
            }
      
            age_range =  buckets[country_index]->age_range(files[country_index][c].substr(0,10));
           
          	stats =  stats + " \n" + age_range;

        }


    }


    int statisticsPort = atoi(serverPort.c_str());

    int sock = 0,valread;
    struct sockaddr_in serv_addr; 
    //string hello = "Hello from worker";
   
    if ((sock = socket(AF_INET, SOCK_STREAM,0)) < 0){
        cout << "Socket creation error" << endl;
        return  -1;
    }

    serv_addr.sin_family = AF_INET;\
   
    serv_addr.sin_port = htons(statisticsPort);
   

    if (inet_pton(AF_INET,serverIP.c_str(),&serv_addr.sin_addr) <= 0){
        cout << "Invalid address or not support" << endl;
        return -1;
    }
    
    if (connect(sock, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) < 0){
        cout << "Connection failed" << endl;
        return -1;
    }    
    


    int sock_worker;
    int new_port;
    struct sockaddr_in worker_addr; 
    if ((sock_worker = socket(AF_INET, SOCK_STREAM,0)) < 0){
    	cout << "Socket creation error" << endl;
    	return  -1;
    }


    worker_addr.sin_family = AF_INET;
    worker_addr.sin_addr.s_addr = INADDR_ANY;
    worker_addr.sin_port = 0;


    if (bind(sock_worker,(struct sockaddr *)&worker_addr,sizeof(worker_addr)) < 0){
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sock_worker,3) < 0){
        perror("listen");
        exit(EXIT_FAILURE);
    }
    //if (inet_pton(AF_INET,serverIP.c_str(),&worker_addr.sin_addr) <= 0){
      //  cout << "Invalid address or not support" << endl;
       // return -1;
    //}

	socklen_t worker_length = sizeof(worker_addr);
   // cout << "prin to accept " << endl;
    
   


	if (getsockname(sock_worker,(struct sockaddr *)&worker_addr,&worker_length) < 0){
		perror("getsockname");
		exit(EXIT_FAILURE);
	}



	string worker_port = to_string(worker_addr.sin_port);
	//cout << "port is " << worker_port << endl;



    string to_send = worker_port + " " + stats;
	
    //send(sock,to_send.c_str(),to_send.length(),0);
    char dolla_buf[2];								//need one more byte for terminal char '\0'
    memset(dolla_buf,0,2);
    strcpy(dolla_buf,"$");
   // send(sock,dolla_buf,1,0);
    sendMessage(sock,to_send);
    
        //new_sock_worker is the global sock for the connection between worker and Server
    if ((new_sock_worker = accept(sock_worker,(struct sockaddr *)&worker_addr,&worker_length)) < 0){
        perror("accept");
        exit(EXIT_FAILURE);
    }
   // cout << "meta to accept";
   	while(1){
   		//cout << "PRIN TO READ " << endl;
        string query = readMessage(new_sock_worker);
        if (sig_quit == 1){
            for (int i = 0; i < count_of_countries; i++){ 
                records_list[i]->delete_recs();
                delete records_list[i];
            }
            delete[] records_list;
            for (int i =0; i < count_of_countries; i++)
                delete buckets[i];
            delete[] buckets;
            delete[] count_of_files_array;
            for (int i = 0; i < count_of_countries; i++)
                delete[] files[i];                                            //for every country a worker has we have a list that we keep all patients
            delete[] files;

            
            delete[] countries;
            break;
        }
        
        istringstream ss(query);
        string answer1;
        ss >> answer1;
        if (answer1 == "/diseaseFrequency"){
            string virusName;
            string date1;
            string date2;
            string country;
            ss >> virusName;
            ss >> date1;
            ss >> date2;
            ss >> country;
            int sum = 0;
            if (country == ""){
                for (int i = 0;i < count_of_countries; i++){
                    sum = sum + buckets[i]->diseaseFrequency(date1,date2,virusName);
                }
            }
            else{
                for (int i = 0; i < count_of_countries; i++){
                    if (country == countries[i]){
                        sum = buckets[i]->diseaseFrequency(date1,date2,virusName);
                        break;
                    }
                }
            }
            string to_send = to_string(sum);
            //cout << "THA STEILW PISW TO SUM " << to_send << endl;
            //send(new_sock_worker,to_send.c_str(),to_send.length(),0);
            sendMessage(new_sock_worker,to_send);
        }
        else if (answer1 == "/topk-AgeRanges"){

            string k;
            string country;
            string virusName;
            string date1;
            string date2;
            ss >> k;
            ss >> country;
            ss >> virusName;
            ss >> date1;
            ss >> date2;
            string to_send = "";
            for (int i = 0;i < count_of_countries; i ++){
                if (country == countries[i]){
                    to_send = buckets[i]->topk_AgeRanges(k,virusName,date1,date2);
                    break;
                }
            }
            //string to_send = to_string(sum);
            //cout << "THA STEILW PISW sto topk " << to_send << endl;
            //send(new_sock_worker,to_send.c_str(),to_send.length(),0);
            sendMessage(new_sock_worker,to_send);
        }
        else if (answer1 == "/searchPatientRecord"){
            string recordID;
            ss >> recordID;
            string to_send ="";
            for (int i = 0; i < count_of_countries; i++){
                to_send = records_list[i]->searchPatientRecord(recordID);
                if (to_send != "")  
                     break;
            }
           // send(new_sock_worker,to_send.c_str(),to_send.length(),0);
        sendMessage(new_sock_worker,to_send);
        }
        else if (answer1 == "/numPatientAdmissions"){
            string virusName;
            string date1;
            string date2;
            string country;
            ss >> virusName;
            ss >> date1;
            ss >> date2;
            ss >> country;
            string to_send = "";
            int sum = 0;
            if (country == ""){
                for (int i = 0;i < count_of_countries; i++){
                    sum = sum + buckets[i]->diseaseFrequency(date1,date2,virusName);
                    //to_send = to_send + countries[i] + " " + to_string(sum) + "\n";
                }
            }
            else{
                for (int i = 0; i < count_of_countries; i++){
                    if (country == countries[i]){
                        sum = buckets[i]->diseaseFrequency(date1,date2,virusName);
                        //to_send = countries[i] + " " + to_string(sum) + "\n";
                        break;
                    }
                }
            }
            to_send = to_string(sum);
            //send(new_sock_worker,to_send.c_str(),to_send.length(),0);
            sendMessage(new_sock_worker,to_send);
        }
        else if(answer1 == "/numPatientDischarges"){
            string virusName;
            string date1;
            string date2;
            string country;
            ss >> virusName;
            ss >> date1;
            ss >> date2;
            ss >> country;
            int sum=0;
            string to_send = "";
            if (country == ""){
                for (int i = 0;i < count_of_countries; i++){
                    sum = sum + buckets[i]->numPatientDischarges(date1,date2,virusName);
                    //to_send = to_send + countries[i] + " " + to_string(sum) + "\n";
                }
            }
            else{
                for (int i = 0; i < count_of_countries; i++){
                    if (country == countries[i]){
                        sum = buckets[i]->numPatientDischarges(date1,date2,virusName);
                       // to_send = countries[i] + " " + to_string(sum) + "\n";
                        break;
                    }
                }
            }
            to_send = to_string(sum);
            //cout << " THA STEILW " << to_send << endl;
            //send(new_sock_worker,to_send.c_str(),to_send.length(),0);
            sendMessage(new_sock_worker,to_send);
        }
        //if (sig_quit == 0)
            //send(new_sock_worker,dolla_buf,1,0);
   	}
    
    
    //delete[] fd2;

	return 0; 
	
}



long int totalDays(string date){
    if (date == "-")
        return -1;
    string tempString;
    tempString = date.substr(0,2);
    int days = atoi(tempString.c_str());
    tempString = date.substr(3,2);
    int months = atoi(tempString.c_str());
    tempString = date.substr(6,4);
    int years = atoi(tempString.c_str());

    long int x = years*365 + months * 30 + days;        //convert to days
    return x;
}

