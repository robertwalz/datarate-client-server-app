#include <stdio.h>
#include <stdlib.h>  
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>


#define SERVER_IP "127.0.0.1" // IP-Adresse des Servers
#define SERVER_PORT "7777" // Port der Serveranwendung
#define BLOCK_SIZE 102400 // 100 KiB
#define CHECK_INTERVAL 10 // Sekunden des Testintervalls
#define TEST_DURATION 3 // Übertragungsdauer eines Tests in Sekunden
#define MIN_DATARATE 2.5 // benötigte Mindestdatenrate in MiB/s
#define COMMAND_HISTORY_FILE "history.txt" // Dateiname der History-Datei



double getDatarateInMiBPerSecond();
int changeGateway(char ipAddress[]);
void testAllGateways(char** gatewayList, int numGateways);
char **getCommandHistory();



int main(int argc, char** argv){


	char **listOfGateways;
	int numberOfGateways=0;

	// Wenn die IP-Adressen der Gateways als Parameter angegeben werden,
	// dann werden diese benutzt und anschließend in die history.txt geschrieben.
	if(argc>1){
		numberOfGateways = argc-1;

		FILE *fp;
		fp = fopen(COMMAND_HISTORY_FILE, "w+");
		listOfGateways = malloc(numberOfGateways * sizeof(char*));
		for(int i=0;i<numberOfGateways;i++){
			listOfGateways[i] = malloc(INET_ADDRSTRLEN);
			strcpy(listOfGateways[i], argv[i+1]);
			fputs(listOfGateways[i], fp);
			fputs("\n", fp);
		}

		fclose(fp);

	} 
	// Werden keine Parameter angegeben werden die IP-Adressen aus der history.txt verwendet.
	// Existiert keine history.txt bricht das Programm ab, mit der Fehlermeldung, dass die IP-Adressen
	// der Gateways als Parameter angegeben werden müssen.
	else{
		listOfGateways = getCommandHistory();
		numberOfGateways=0;
		while(*listOfGateways[numberOfGateways]!='\0'){
			numberOfGateways++;
		}
	}

	printf("Found %d Gateway-IPs\n", numberOfGateways);
	for(int i=0;i<numberOfGateways;i++)printf("IP of gateway %d: %s\n", i+1, listOfGateways[i]);


	printf("Starting benchmark client app ...\n");
	double datarate;

	// Test der bestehenden Verbindung durch Senden von Blocks für eine bestimmte Zeit in einem bestimmten Intervall; 
	// wird solange fortgeführt wie die Mindestdatenrate erreicht wird.
	do{
		START:
		printf("================================\n");
		printf("Waiting %d seconds until next test.\n", CHECK_INTERVAL);
		sleep(CHECK_INTERVAL);	
		datarate = getDatarateInMiBPerSecond();
		if(datarate >= MIN_DATARATE){
			printf("Datarate %f MiB/s is sufficient (Minimum: %f).\n", datarate, MIN_DATARATE);
		}
	} while(datarate >= MIN_DATARATE);

	// Wird die Mindestdatenrate bei einem Test nicht erreicht, 
	// dann wird der User gefragt, ob er die anderen Gateways testen möchte
	printf("Datarate %f MiB/s is not sufficient (Minimum: %f).\n", datarate, MIN_DATARATE);
	char checkOtherGateways;
	do{
		printf("Do you want to test the other gateways? (y)es/(n)o\n");
		scanf(" %c", &checkOtherGateways);
	} while(!(checkOtherGateways == 'y' || checkOtherGateways == 'n'));
	// Wird dies verneint, dann bricht das Programm ab.
	if(checkOtherGateways == 'n'){
		printf("Goodbye.\n");
		exit(0);
	}
	// Wird dies bestätigt, dann werden alle Gateways durchprobiert.
	// Als Ergebnis werden die Testergebnisse aller Gateways und das Gateway 
	// mit der höchsten Datenrate am Bildschirm ausgegeben.
	else{
		testAllGateways(listOfGateways, numberOfGateways);
	}

	int index;
	// Anschließen wird der User gefragt, welches Gateway er für 
	// die zukünfigte Übertragung auswählen möchte.
	// Durch Eigabe der Gateway-Ziffer wird das Gateway geändert.
	do{
		char choice;
		printf("Please select a gateway (1-%d)/(q)uit\n", numberOfGateways);
		// This works only if the number of available gateways is < 10 !!
		scanf(" %c", &choice);
		index = (int)choice-48-1;

		if(choice=='q'){
			printf("Goodbye.\n");
			exit(0);
		}

	} while(!(index>=0&&index<numberOfGateways));

	printf("Gateway %d selected.\n", index+1);
	int res = changeGateway(listOfGateways[index]);

	// Dann wird wieder an den Start des Programms zurückgekehrt.
	goto START;

	
	return 0;
}



double getDatarateInMiBPerSecond(){

	struct addrinfo hints;
	int clientBinding;
	int mySocket;
	int myConnection;

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // AF_INET or AF_INET6
	hints.ai_socktype = SOCK_STREAM;

	// Connect to a socket
	struct addrinfo *res;
	getaddrinfo(SERVER_IP, SERVER_PORT, &hints, &res);

	// Create local Socket
	mySocket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);

	// Connect to server socket
	myConnection = connect(mySocket, res->ai_addr, res->ai_addrlen);

	// Send Message to server and receive response

	char *blockData[BLOCK_SIZE]; // 
	memset(&blockData, 65, sizeof blockData);
	int blocksReceivedByServer = 0;

	printf("================================\n");
	printf("Starting test ...\n");
	printf("Sending blocks for %d seconds.\n", TEST_DURATION);
  time_t start, ende;
	start = time(NULL); // now 
	int secs;
	do{
			ende = time(NULL);
			secs = difftime(ende,start);
			int sendMsg = send(mySocket, blockData, BLOCK_SIZE, 0); 
			int return_status = read(mySocket, &blocksReceivedByServer, sizeof(blocksReceivedByServer));
			if (return_status > 0) {
			}

	} while(secs<TEST_DURATION);
	blocksReceivedByServer = ntohl(blocksReceivedByServer);
	fprintf(stdout, "Sent blocks: %d (blocksize = %d Bytes)\n", blocksReceivedByServer, BLOCK_SIZE);

	/* Calculation of datarate */

	unsigned long sentBytes = BLOCK_SIZE*blocksReceivedByServer;
	printf("Sent bytes: %lu\n", sentBytes);

	double sentMiB = (double)(blocksReceivedByServer/(10.24)); // 1 block = 100 KiB

	double dataRateMiBPerSecond = (sentMiB/TEST_DURATION);
	printf("Datarate in MiB/s: %f\n", dataRateMiBPerSecond);

	double dataRateMBitPerSecond = (sentMiB/TEST_DURATION)*8;
	//printf("Datarate in MBit/s: %f\n", dataRateMBitPerSecond);

	close(mySocket);

	return dataRateMiBPerSecond;

}


int changeGateway(char ipAddress[]){

	printf("Trying to change gateway to %s ...\n", ipAddress);

	/* command is linux dependent */
	char command[] = "ip route change default via "; // + gateway ip address

	/* command for windows*/
	//char command[] = "route change 0.0.0.0 mask 0.0.0.0 "; // + gateway ip address

	int len1 = strlen(command);
	int len2 = strlen(ipAddress);

	char res[len1+len2+1];
	for(int i=0;i<len1;i++){
		res[i]=command[i];
	}
	for(int i=0;i<len2;i++){
		res[len1+i]=ipAddress[i];
	}
	res[len1+len2]='\0';

	int resultSystem = system(res);
	return resultSystem;

}

void testAllGateways(char** gatewayList, int numGateways){

	printf("Starting test of all available gateways ...\n");
	printf("*** Number of available gateways: %d \n", numGateways);

	for(int i=0;i<numGateways;i++){
		printf("*** Gateway %d: %s \n", i+1, gatewayList[i]);
	}
	printf("\n");

	double highestDatarate=0;
	int highestDatarateIndex=0;

	// Iterate through all available gateways
	for(int i=0;i<numGateways;i++){
		printf("******* Test of Gateway %d: %s *******\n", i+1, gatewayList[i]);

		// Change route to use this gateway
		int res = changeGateway(gatewayList[i]);
		//printf("res code: %d\n", res);

		// Start benchmark test 
		double datarate = getDatarateInMiBPerSecond();
		//printf("Datarate in MiB/s: %f\n\n", datarate);

		if(datarate > highestDatarate){
			highestDatarate = datarate;
			highestDatarateIndex = i;
		}
		printf("\n");
	}
	printf("Highest datarate is %f on gateway %d\n", highestDatarate, highestDatarateIndex+1);

}
char **getCommandHistory(){

	char ** strArr = malloc(10 * sizeof(char*));
	for (int i=0; i<10; i++){
		strArr[i] = malloc(INET_ADDRSTRLEN);
	}

	/* Open the file for reading */
	char *buffer = NULL;
	size_t bufferSize = 0;
	ssize_t lineSize;
	int counter = 0;

	FILE *fp = fopen(COMMAND_HISTORY_FILE, "r");

	if (!fp){
		fprintf(stderr, "Error: File not found. '%s'\n", COMMAND_HISTORY_FILE);
		printf("usage: ./benchmarkClientApp \"gatewayIP1\" \"gatewayIP2\" ...\n");
		exit(0);

	}

	lineSize = getline(&buffer, &bufferSize, fp);
	while (lineSize >= 0 && counter<10){
		// remove newline from getline
		if(buffer[lineSize-1]== '\n') {
			buffer[lineSize-1] = '\0';
		}
		strcpy(strArr[counter], buffer);
		lineSize = getline(&buffer, &bufferSize, fp);
		counter++;
	}

	free(buffer);
	buffer = NULL;
	fclose(fp);

	return strArr;

}

