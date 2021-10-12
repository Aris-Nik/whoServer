#include "../Headers/whoClient.h"



queryList::queryList(){
	head = NULL;
	tail = NULL;
}

queryList::~queryList(){
	
	queryNode* temp = this->head;
	while (temp != NULL){
		queryNode* temp1 = temp;
		temp = temp->next;
		delete temp1;
	}
}

void queryList::insert_query(string query){
	queryNode* temp = new queryNode;
	temp->query = query;
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

void queryList::print_list(){
	queryNode* temp = this->head;
	int i = 0;
	while (temp != NULL){
		cout << "Position " << i << " has query : " << temp->query << endl;
		i++;
		temp = temp->next;
	}
}

int queryList::length(){
	queryNode* temp = this->head;
	int i = 0;
	while (temp != NULL){
		i++;
		temp=temp->next;
	}
	return i;
}

string queryList::pop(){
	if (this->head == NULL)
		return "EOF";
	queryNode* temp = this->head;
	this->head = this->head->next;
	string ret = temp->query;
	delete temp;
	return ret;
}