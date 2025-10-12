#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

 void *myFirstThread(void *p)
 {
     	sleep(1);
     	printf("I am the first thread \n");
     	return NULL;
}

void *mySecondThread(void *p)
{ 
     	printf("I am the second thread \n");
     	pthread_yield();
}

void *myNewThread(void *p)
{
  printf("Hello!\n");

  //Ask user to enter an integer
  int num;
	
  printf("Please enter an integer: ");
  scanf("%d", &num);
	
  //print the integer in decimal, binary, and in hexadecimal form
  printf("Decimal: %d\n", num);
  printf("Hexadecimal: %X\n", num);	
}

int main() 
{
  pthread_t tid,tid2, tid3;

  printf("Program started...\n");
	
	pthread_create(&tid, NULL, myFirstThread, NULL);
	pthread_create(&tid2, NULL, mySecondThread, NULL);
	pthread_create(&tid3, NULL, myNewThread, NULL);
	
	pthread_join(tid, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid3, NULL);
	
	printf("All threads done.\n");
       	exit(0);
}
