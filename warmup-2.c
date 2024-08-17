#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include "my402list.h"
#include "cs402.h"


pthread_mutex_t mutex=PTHREAD_MUTEX_INITIALIZER;

typedef struct tagPacket{
	int num;
	int req;
	struct timeval arrivalTime;
	struct timeval departureTime;
	struct timeval serviceTime;

	struct timeval q1wait;
	struct timeval q2wait;
}Packet;

int MAXLEN=2147483647;
double lambda=1;
double mu=0.35;
double r=4;
double B=10;
double P=3;
double n=20;
char *fname=NULL;
FILE *f;

void *arrive(void *arg){
	int val=(int)arg;
	fprintf(stdout,"%d\n",val);
	return NULL;
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
			printf("num is %.6g\n",n);
		}
		else if(strcmp(curr,"-t")==0){

			if(i>=argNum || argVal[i][0]=='-'){
				fprintf(stderr, "File not found\n");
				exit(1);
			}
			struct stat detail;
			char *btemp;
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
			if(fgets(btemp,10,f)){

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
			printf("File name is %s\n",fname);

		}
		else{
			fprintf(stderr, "Invalid command line option\n");
			exit(1);
		}

	}
}

int main(int argc, char *argv[]){

	getParameters(argc,argv);
	My402List *Q1=(My402List*)malloc(sizeof(My402List));
	My402List *Q2=(My402List*)malloc(sizeof(My402List));
	My402ListInit(Q1);
	My402ListInit(Q2);
	Packet *newPacket=(Packet*)malloc(sizeof(Packet));
	pthread_t packetArrival;


	int status=pthread_create(&packetArrival,NULL,arrive,(void *)1);

	if(status!=0){
		printf("Error");
		return 0;
	}
	pthread_join(packetArrival,NULL);

	return 0;

}