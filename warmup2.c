#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "my402list.h"
#include "cs402.h"


//arrival time = Q1 entry time
//departure time = Q2 exit time
//inter-arrival time = curr arrival-prev arrival
//time in Q1 = Q1 exit-Q1 exit
//time in Q2 = Q2 exit-Q2 exit
//service time = departs from S1-enters S1
//time in system = P1 departs from S1-P1 arrives


//EVERYTHING MUST BE IN MILLISECONDS

pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cv = PTHREAD_COND_INITIALIZER;

pthread_t packetArrival;
pthread_t tokenArrival;
pthread_t serverThread1;
pthread_t serverThread2;
pthread_t signalThread;

typedef struct tagPacket{
	int num;
	int req;
	int serviceTime;
	double arrivalTime;

	double q1Enter;
	double q2Enter;
	double q1Exit;
	double q2Exit;

	double enterService;
	double exitService;
}Packet;

//Global/Shared Variables
struct timeval startTime, endTime, prev_token_time;
My402List *Q1;
My402List *Q2;
sigset_t sig;
int MAXLEN=2147483647;
double avgInterArr;
double avgPacServ;
double avgPacQ1;
double avgPacQ2;
double lambda=1;
double mu=0.35;
double r=4;
double B=10;
double P=3;
int n=20;
int dropped=0;
int currTokens=0;
int tokID=0;
char *fname=NULL;
int pacDone=0;
int threadEnd=0;
int tokDone=0;
FILE *f;


double getTimeDiff(struct timeval prevTime){
	struct timeval presentTime;
	double preTime= 1000 * (prevTime.tv_sec) + (prevTime.tv_usec) /1000.0;

	gettimeofday(&presentTime,NULL);
	double presTime= 1000*(presentTime.tv_sec) + (presentTime.tv_usec) /1000.0;

	return (presTime-preTime);
}


void *sigHandler(){
	int oldSet;
	double signalReceived;
	sigwait(&sig,&oldSet);

	pthread_mutex_lock(&mutex);
	pacDone=1;
	tokDone=1;

	signalReceived=getTimeDiff(startTime);
	fprintf(stdout,"\n%012.3lfms: SIGINT caught, no new packets or tokens will be allowed\n",signalReceived);

	pthread_cond_broadcast(&cv);
	pthread_cancel(packetArrival);
	pthread_cancel(tokenArrival);

	for(My402ListElem *elem=My402ListFirst(Q1);elem!=NULL;elem=My402ListNext(Q1,elem)){
		if(My402ListEmpty(Q1)){
			break;
		}
		fprintf(stdout,"%012.3lfms: p%d removed from Q1\n",(double)getTimeDiff(startTime),((Packet*)(elem->obj))->num);
		My402ListUnlink(Q1,elem);
	}
	for(My402ListElem *elem=My402ListFirst(Q2);elem!=NULL;elem=My402ListNext(Q2,elem)){
		if(My402ListEmpty(Q2)){
			break;
		}
		fprintf(stdout,"%012.3lfms: p%d removed from Q2\n",(double)getTimeDiff(startTime),((Packet*)(elem->obj))->num);
		My402ListUnlink(Q2,elem);
	}

	pthread_mutex_unlock(&mutex);
	pthread_exit(NULL);


}
void *server(void* arg){
	int slpTime;
	int SID=(int)arg;

	for(;;){
		pthread_mutex_lock(&mutex);

		while(My402ListEmpty(Q2)){ //Exit Condition
			if (My402ListEmpty(Q2) && tokDone==1 && pacDone==1){
			pthread_mutex_unlock(&mutex);
			pthread_exit(NULL);
			return(0);
			} 
			pthread_cond_wait(&cv,&mutex);
		}
		Packet *currPac=NULL;
		if(!My402ListEmpty(Q2)){
			My402ListElem *currEle=My402ListFirst(Q2);
			currPac=(Packet*)(currEle->obj);

			My402ListUnlink(Q2,currEle);
			currPac->q2Exit=getTimeDiff(startTime);
			fprintf(stdout, "%012.3fms: ",currPac->q2Exit);
            fprintf(stdout, "p%d leaves Q2, time in Q2 = %.3fms\n",currPac->num, currPac->q2Exit-currPac->q2Enter);
            currPac->enterService= getTimeDiff(startTime);
			fprintf(stdout, "%012.3fms: ",currPac->enterService);
            fprintf(stdout, "p%d begins service at S%d, requesting %dms of service\n", currPac->num, SID, currPac->serviceTime);
		}
		slpTime=(currPac->serviceTime)*1000;
		pthread_mutex_unlock(&mutex);
		usleep(slpTime);
		pthread_mutex_lock(&mutex);
		currPac->exitService=getTimeDiff(startTime);
		fprintf(stdout, "%012.3fms: ",currPac->exitService);
		fprintf(stdout, "p%d departs from S%d, service time = %dms, time in system = %4.3lfms\n",currPac->num,SID,currPac->serviceTime,currPac->exitService-currPac->arrivalTime);
		pthread_mutex_unlock(&mutex);
	}

	return(0);
}

void *tokArrive(){


	double tokInterArrival=min(10000,1000.0/r);
	int slpTime=tokInterArrival*1000;

	for(;;){
		usleep(slpTime);
		pthread_mutex_lock(&mutex);
		pthread_cleanup_push((void *)pthread_mutex_unlock, &mutex);
		//Exit Condition
		if(pacDone==1 && My402ListEmpty(Q1)==TRUE){
			tokDone=1;
			pthread_cond_broadcast(&cv);
			pthread_mutex_unlock(&mutex);
			pthread_exit(NULL);
			return(0);
		}

		fprintf(stdout, "%012.3fms: ",getTimeDiff(startTime));

		tokID++;
		if(currTokens<B){
			currTokens++;
			fprintf(stdout, "token t%d arrives, token bucket now has %d token\n",tokID,currTokens);
		}
		else{
			dropped++;
			fprintf(stdout, "%012.3fms: token t%d arrives, dropped\n", getTimeDiff(startTime), tokID);
		}


		if(My402ListEmpty(Q1)==FALSE){
			My402ListElem *currEle=My402ListFirst(Q1);
			Packet *currPac=(Packet*)(currEle->obj);
			fprintf(stdout, "%012.3fms: ",getTimeDiff(startTime));
			if(currPac->req<=currTokens){
				currTokens-=currPac->req;
				currPac->q1Exit=getTimeDiff(startTime);
				fprintf(stdout,"p%d leaves Q1, time in Q1 = %4.3lfms, token bucket now has %d token\n",currPac->num,currPac->q1Exit,currTokens);
				My402ListUnlink(Q1,currEle);
				My402ListAppend(Q2,(void*)currPac);
				currPac->q2Enter=getTimeDiff(startTime);
				fprintf(stdout,"%012.3lfms: p%d enters Q2\n",currPac->q2Enter,currPac->num);
				if(My402ListLength(Q2)==1){ //Broadcast if Q2 now has the packet
					pthread_cond_broadcast(&cv);
				}
			}
		}
		pthread_cleanup_pop(0);
		pthread_mutex_unlock(&mutex);
	}
   	pthread_exit(NULL);
   	return(0);
}

void *pacArrive(){
	Packet *packet;
	int LENMAX=1023;
    char buff[LENMAX];
    double totServTime;
    double prev_packet_time=0;
	long inter_arrival_time;

	for(int i=0;i<n;i++){

		if(fname!=NULL){

			if(fgets(buff,sizeof(buff),f)!=NULL){
		        if(buff[strlen(buff)-1]=='\n'){
            		buff[strlen(buff)-1]='\0';
        		}
        		if(strlen(buff)>(LENMAX-2)){
            		fprintf(stderr, "There are at most 1024 lines\n");
            		exit(1);
        		}
       			packet=(Packet*)malloc(sizeof(Packet));
				packet->num=i+1;


        		char *start=buff;
        		char *temp;
        		temp=strchr(start,' ');
        		if(temp!=NULL){
        			*temp='\0';
        			temp++;
        			while(*temp==' ' || *temp=='\t'){
        				temp++;
        			}
        		}

        		else{
        			fprintf(stderr, "Invalid Argument %s in file\n",start);
        			exit(1);
        		}

        		inter_arrival_time=atoi(start);
        		start=temp;

        		temp=strchr(start,' ');
        		if(temp!=NULL){
        			*temp='\0';
        			temp++;
        			while(*temp==' ' || *temp=='\t'){
        				temp++;
        			}
        		}
        		packet->req=atoi(start);
        		start=temp;

        		temp=strchr(start,' ');
        		if(temp!=NULL){
        			*temp='\0';
        			temp++;
        			while(*temp==' ' || *temp=='\t'){
        				temp++;
        			}
        		}
        		char *endpt;
        		packet->serviceTime=strtod(start,&endpt);
        		//printf("%ld\n",inter_arrival_time);
        		//printf("%d\n",packet->req);
        		//printf("%d\n",packet->serviceTime);

    			long slpTime=inter_arrival_time*1000;
				usleep(slpTime);


				pthread_mutex_lock(&mutex);
				pthread_cleanup_push((void *)pthread_mutex_unlock, &mutex);

				packet->arrivalTime=getTimeDiff(startTime);
				packet->num=i+1;
				totServTime=totServTime+packet->serviceTime;

				printf("%012.3fms: ",packet->arrivalTime);
				if(packet->req>B){
					fprintf(stdout, "p%d arrives, needs %d tokens, inter-arrival time = %.3lfms. dropped\n",i+1,packet->req,packet->arrivalTime-prev_packet_time);
					prev_packet_time=packet->arrivalTime;
					dropped++;
				}
				else{
					fprintf(stdout, "p%d arrives, needs %d tokens, inter-arrival time = %4.3lfms\n",i+1,packet->req,packet->arrivalTime-prev_packet_time);
					prev_packet_time=packet->arrivalTime;
					packet->q1Enter=getTimeDiff(startTime);
					if(!My402ListEmpty(Q1)){
						fprintf(stdout, "%012.3fms: p%d enters Q1\n",packet->arrivalTime,packet->num);
						My402ListAppend(Q1,packet);
					}
					else{
						if(packet->req>currTokens){
							fprintf(stdout, "%012.3fms: p%d enters Q1\n",packet->arrivalTime,packet->num);
							My402ListAppend(Q1,packet);
						}
						else{
							packet->q1Exit=getTimeDiff(startTime);
							printf("%012.3fms: ",packet->q1Exit);
							currTokens-=packet->req;
							fprintf(stdout, "p%d leaves Q1, time in Q1 = %4.3lfms, token bucket now has %d token(s)\n",packet->num,packet->q1Exit-packet->q1Enter,currTokens);
							My402ListAppend(Q2,packet);
							packet->q2Enter=getTimeDiff(startTime);
							fprintf(stdout, "%012.3fms: p%d enters Q2\n", packet->q2Enter, packet->num);
							if(My402ListLength(Q2)==1){
								pthread_cond_broadcast(&cv);
							}
						}
					}
				}
				pthread_cleanup_pop(0);
				pthread_mutex_unlock(&mutex);
			}
		}
		else{
			packet=(Packet*)malloc(sizeof(Packet));
			packet->num=i+1;
			inter_arrival_time=min(10000,1000.0/lambda);
			packet->req=P;
			packet->serviceTime=min(10000,1000.0/mu);


    		int slpTime=inter_arrival_time*1000;
			usleep(slpTime);

			pthread_mutex_lock(&mutex);
			pthread_cleanup_push((void *)pthread_mutex_unlock, &mutex);

			packet->arrivalTime=getTimeDiff(startTime);
			packet->num=i+1;
			totServTime=totServTime+packet->serviceTime;

			printf("%012.3fms: ",packet->arrivalTime);
			if(packet->req>B){
				fprintf(stdout, "p%d arrives, needs %d tokens, inter-arrival time = %.3lfms. dropped\n",i+1,packet->req,packet->arrivalTime-prev_packet_time);
				prev_packet_time=packet->arrivalTime;
				dropped++;
			}
			else{
				fprintf(stdout, "p%d arrives, needs %d tokens, inter-arrival time = %4.3lfms\n",i+1,packet->req,packet->arrivalTime-prev_packet_time);
				prev_packet_time=packet->arrivalTime;
				packet->q1Enter=getTimeDiff(startTime);
				if(!My402ListEmpty(Q1)){
					fprintf(stdout, "%012.3fms: p%d enters Q1\n",packet->arrivalTime,packet->num);
					My402ListAppend(Q1,packet);
				}
				else{
					if(packet->req>currTokens){
						fprintf(stdout, "%012.3fms: p%d enters Q1\n",packet->arrivalTime,packet->num);
						My402ListAppend(Q1,packet);
					}
					else{
						packet->q1Exit=getTimeDiff(startTime);
						printf("%012.3fms: ",packet->q1Exit);
						currTokens-=packet->req;
						fprintf(stdout, "p%d leaves Q1, time in Q1 = %4.3lfms, token bucket now has %d token(s)\n",packet->num,packet->q1Exit-packet->q1Enter,currTokens);
						My402ListAppend(Q2,packet);
						packet->q2Enter=getTimeDiff(startTime);
						fprintf(stdout, "%012.3fms: p%d enters Q2\n", packet->q2Enter, packet->num);
						if(My402ListLength(Q2)==1){
							pthread_cond_broadcast(&cv);
							}
						}
					}
				}
				pthread_cleanup_pop(0);
				pthread_mutex_unlock(&mutex);
			}
	}
	pthread_mutex_lock(&mutex);
	pacDone=1;
	pthread_mutex_unlock(&mutex);
    pthread_exit(NULL);
}

void getParameters(int argNum, char* argVal[]){

	char *curr;
	char *end;
	double temp;
	for(int i=1;i<argNum;i++){
		curr=argVal[i];
		i++;
		if(strcmp(curr,"-lambda")==0){
			if(i>=argNum || argVal[i][0]=='-'){
				fprintf(stderr, "Value for lambda cannot be empty or negative\n");
				exit(1);
			}
            if (strchr(argVal[i], '\n') != NULL) {
                *strchr(argVal[i], '\n') = '\0';
            }
			temp=strtod(argVal[i],&end);
			if(*end!='\0'){
				fprintf(stderr, "Invalid value for lambda\n");
				exit(1);
			}
			lambda=temp;
			printf("lambda is %.6g\n",lambda);
		}
		else if(strcmp(curr,"-mu")==0){
			if(i>=argNum || argVal[i][0]=='-'){
				fprintf(stderr, "Value for mu cannot be empty or negative\n");
				exit(1);
			}
			temp=strtod(argVal[i],&end);
			if(*end!='\0'){
				fprintf(stderr, "Invalid value for mu\n");
				exit(1);
			}
			mu=temp;
			printf("mu is %.6g\n",mu);
		}
		else if(strcmp(curr,"-r")==0){
			if(i>=argNum || argVal[i][0]=='-'){
				fprintf(stderr, "Value for r cannot be empty or negative\n");
				exit(1);
			}
			temp=strtod(argVal[i],&end);
			if(*end!='\0'){
				fprintf(stderr, "Invalid value for r\n");
				exit(1);
			}
			r=temp;
			printf("r is %.6g\n",r);
		}
		else if(strcmp(curr,"-B")==0){
			if(i>=argNum || argVal[i][0]=='-'){
				fprintf(stderr, "Value for B cannot be empty or negative\n");
				exit(1);
			}
			temp=strtod(argVal[i],&end);
			if(*end!='\0'){
				fprintf(stderr, "Invalid value for B\n");
				exit(1);
			}
			if(temp>MAXLEN){
				fprintf(stderr, "Value for B is too large\n");
				exit(1);
			}
			B=temp;
			printf("B is %.6g\n",B);
		}
		else if(strcmp(curr,"-P")==0){
			if(i>=argNum || argVal[i][0]=='-'){
				fprintf(stderr, "Value for P cannot be empty or negative\n");
				exit(1);
			}
			temp=strtod(argVal[i],&end);
			if(*end!='\0'){
				fprintf(stderr, "Invalid value for P\n");
				exit(1);
			}
			if(temp>MAXLEN){
				fprintf(stderr, "Value for P is too large\n");
				exit(1);
			}
			P=temp;
			printf("P is %.6g\n",P);
		}
		else if(strcmp(curr,"-n")==0){
			if(i>=argNum || argVal[i][0]=='-'){
				fprintf(stderr, "Value for num cannot be empty or negative\n");
				exit(1);
			}
			temp=strtod(argVal[i],&end);
			if(*end!='\0'){
				fprintf(stderr, "Invalid value for num\n");
				exit(1);
			}
			if(temp>MAXLEN){
				fprintf(stderr, "Value for num is too large\n");
				exit(1);
			}
			n=temp;
			printf("num is %d\n",n);
		}
		else if(strcmp(curr,"-t")==0){

			if(i>=argNum || argVal[i][0]=='-'){
				fprintf(stderr, "File not found\n");
				exit(1);
			}
			struct stat detail;
			char btemp[10];
			fname=argVal[i];

			stat(argVal[2],&detail);
			f=fopen(fname,"r");

			if(!f){
				fprintf(stderr, "File not found\n");
				exit(1);
			}
	        if(S_ISDIR(detail.st_mode)){
            	fprintf(stderr, "The file cannot be a directory\n");
            	exit(1);
        	}
        	if(strcmp(fname,"/var/log/btmp") == 0 || strcmp(fname, "/root/.bashrc") == 0){
        		fprintf(stderr, "File access denied\n");
        		exit(1);
        	}
			if(fgets(btemp,10,f)!=NULL){

				n=atoi(btemp);
				if(n==0 && btemp[0]!='0'){
					fprintf(stderr, "First line is not a number\n");
					fclose(f);
					exit(1);
				}
			}
			else{
				fprintf(stderr, "File is empty\n");
				fclose(f);
				exit(1);
			}
		}
		else{
			fprintf(stderr, "Invalid command line option\n");
			exit(1);
		}

	}
}

int main(int argc, char *argv[]){

	getParameters(argc,argv);
	Q1=(My402List*)malloc(sizeof(My402List));
	Q2=(My402List*)malloc(sizeof(My402List));
	My402ListInit(Q1);
	My402ListInit(Q2);

	sigemptyset(&sig);
	sigaddset(&sig,SIGINT);
	pthread_sigmask(SIG_BLOCK,&sig,NULL);

	gettimeofday(&startTime,NULL);

	fprintf(stdout,"%012.3fms: emulation begins\n",getTimeDiff(startTime));
	prev_token_time=startTime;

	pthread_create(&packetArrival,NULL,pacArrive,NULL);
	pthread_create(&tokenArrival,NULL,tokArrive,NULL);
	pthread_create(&serverThread1,NULL,server,(void*)1);
	pthread_create(&serverThread2,NULL,server,(void*)2);
	pthread_create(&signalThread,NULL,sigHandler,NULL);



	pthread_join(packetArrival,NULL);
	pthread_join(tokenArrival,NULL);
	pthread_join(serverThread1,NULL);
	pthread_join(serverThread2,NULL);

	fprintf(stdout, "%012.3fms: emulation ends\n",getTimeDiff(startTime));

	return 0;

}