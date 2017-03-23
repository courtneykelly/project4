/*	test_read.cc

	This file tests the Config class.
*/

#include <iostream>
#include <string>
#include <vector>
#include <stdio.h>
#include <stdlib.h> 
#include <curl/curl.h>
#include <signal.h>
#include <fstream>
#include <pthread.h>
#include <unistd.h>
#include "Config.h"
#include "Parse.h"
#include "CurlSite.h"
#include "Queue.h"
#include "Node.h"

using namespace std;

// Global Variables
Queue fetchQueue;
Queue parseQueue;
Config config("config.txt");
int FILECOUNT = 0;
vector<string> keywords;
vector<string> sites;
vector<string> output;
int keepRunning = 1;
pthread_mutex_t output_mutex = PTHREAD_MUTEX_INITIALIZER;

// Functions
void createFetchThreads();
void createParseThreads();
void alarmHandler(int);
void* fetchThreadHandler( void* threadID );
void* parseThreadHandler( void* threadID );
void exitHandler();

int main() {

	//config.setParams("config.txt");

	// Parse search file
	Parse search(config.SEARCH_FILE, config.NUM_PARSE);
	keywords = search.getData();

	// Parse sites file
	Parse psites(config.SITE_FILE, config.NUM_PARSE);
	sites = psites.getData();

	// Fill fetch queue for the first run
	fetchQueue.fill(sites);
	//fetchQueue.printQueue();

	// Set timer
	signal(SIGALRM, alarmHandler);

	// Set exit signal
	// signal(SIGINT, exitHandler);

	// Perform data fetch
	createFetchThreads();
	//parseQueue.printQueue();

	// Perform keyword parse
	createParseThreads();

	alarm(1);

	// keyword search
	
	while(1) {
		sleep(1);
	}

	//fetchQueue.printQueue();

	return 0;

}

void alarmHandler( int sig) {

	// Make output file
	FILECOUNT++;
	ofstream outputFile;
	string filename = to_string(FILECOUNT) + ".csv";
	outputFile.open (filename);

	// Refill fetch queue
	fetchQueue.fill(sites);

	// Reset alarm
	signal(SIGALRM, alarmHandler);
	alarm(10);

	for ( vector<string>::iterator it = output.begin(); it != output.end(); ++it ) {
		outputFile << *it << endl;
	}



	// Close file
	outputFile.close();
	output.clear();


}

void createFetchThreads() {

	// Create threads
	pthread_t* threads = (pthread_t*) malloc( sizeof(pthread_t) * config.NUM_FETCH);
	int rc;

	for( int i = 0; i < config.NUM_FETCH; i++) {

		cout << "thread created..." << endl;
		
		rc = pthread_create(&threads[i], NULL, fetchThreadHandler, (void*) i);
		if (rc) {
        	cout << "Error: unable to create thread: " << rc << endl;
        	exit(1);
      	}

	}

}

void* fetchThreadHandler( void* threadID ) {

	// create curl object
	//CurlSite curl;

	// while fetchQueue is not empty threads try to pop
	while (keepRunning) {
		CurlSite curl;

		// get website contents
		Node newNode = fetchQueue.pop();
		const char* c = newNode.siteName.c_str();
		newNode.siteData = curl.getSite(c);

		// push into parseQueue
		parseQueue.push(newNode);
		//cout << "node pushed into parseQueue..." << endl;

	}


}

void createParseThreads() {

	// Create threads
	pthread_t* parsethreads = (pthread_t*) malloc( sizeof(pthread_t) * config.NUM_PARSE);
	int rc;

	for( int i = 0; i < config.NUM_PARSE; i++) {
		
		rc = pthread_create(&parsethreads[i], NULL, parseThreadHandler, (void*) i);
		if (rc) {
        	cout << "Error: unable to create thread: " << rc << endl;
        	exit(1);
      	}

	}

}

void* parseThreadHandler( void* threadID ) {

	// while parseQueue is not empty threads try to pop
	while (keepRunning) {

		string outputString;

		// get website contents
		Node newNode = parseQueue.pop();

		// Perform keyword search on data
		for (vector<string>::iterator it = keywords.begin(); it != keywords.end(); ++it) {
			// Set count to 0
			int count = 0;

			// Find first occurance of keyword
			size_t nPos = newNode.siteData.find(*it, 0);

			// Find all occurances of keyword
			while(nPos != string::npos) {
				count++;
				nPos = newNode.siteData.find(*it, nPos + 1);
			}


			pthread_mutex_lock( &output_mutex );
			outputString = *it + "," + newNode.siteName + "," + to_string(count);
			output.push_back(outputString);
			pthread_mutex_unlock( &output_mutex );

		}		

	}

}

void exitHandler() {

	cout << "Exit Handler called..." << endl;
	// set keepRunning to false so threads will exit loop
	keepRunning = 0;

	// wait for all threads to finish
	// pthread_join

}