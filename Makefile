CXXFLAGS := -std=c++11 -g3
all: master whoServer whoClient

master:./src/ex3.o ./src/worker.o ./src/list.o ./src/bucketList.o
	g++ -o master -std=c++11 ./src/ex3.o ./src/worker.o ./src/list.o ./src/bucketList.o -g3
	
whoServer:./src/whoServer.o ./Headers/whoServer.h
	g++ -o whoServer -pthread -std=c++11 ./src/whoServer.o -g3

whoClient:./src/whoClient.o ./src/queryList.o ./Headers/whoClient.h
	g++ -o whoClient -pthread -std=c++11 ./src/whoClient.o ./src/queryList.o -g3
	
ex3.o:./src/ex3.cpp ./Headers/ex3.h
	g++ -std=c++11 -c./src/ex3.o ./src/ex3.cpp -g3

worker.o:./src/worker.cpp ./Headers/ex3.h
	g++ -std=c++11 -c ./src/worker.o ./src/worker.cpp -g3

queryList.o:./src/queryList.cpp ./Headers/whoClient.h
	g++ -std=c++11 -c ./src/queryList.o ./src/queryList.cpp -g3
	
list.o:./src/list.cpp ./Headers/ex3.h
	g++ -std=c++11 -c ./src/list.o ./src/list.cpp -g3

bucketList.o:./src/bucketList.cpp ./Headers/ex3.h
	g++ -std=c++11 -c ./src/bucketList.o ./src/bucketList.cpp -g3

whoServer.o:./src/whoServer.cpp
	g++ -std=c++11 -c ./src/whoServer.o ./src/whoServer.cpp -g3



clean:
	rm -f ./master 
	rm -f ./whoServer
	rm -f ./whoClient
	rm -f ./worker 
	rm -f ./src/*.o
