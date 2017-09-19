#include <stdio.h>
#include <stdlib.h>
#include <sys/syscall.h>
#include <time.h>
#include <npheap.h>
#include <tnpheap.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <sys/time.h>
#include <string.h>
#include <sys/wait.h>
#include <malloc.h>

int main(int argc, char *argv[])
{
    int i=0,number_of_processes=1, number_of_objects=1024, number_of_transactions = 65536,j; 
    int a;
    int pid;
    int size;
    char data[8192];
    char filename[256];
    char *mapped_data;
    int npheap_dev, tnpheap_dev;
    unsigned long long msec_time;
    FILE *fp;
    struct timeval current_time;

    if(argc < 3)
    {
        fprintf(stderr, "Usage: %s number_of_objects number_of_transactions number_of_processes\n",argv[0]);
        exit(1);
    }
    number_of_objects = atoi(argv[1]);
    number_of_transactions = atoi(argv[2]);
    number_of_processes = atoi(argv[3]);
    tnpheap_init();
    npheap_dev = open("/dev/npheap",O_RDWR);
    tnpheap_dev = open("/dev/tnpheap",O_RDWR);
    if(tnpheap_dev < 0 || npheap_dev < 0)
    {
        fprintf(stderr, "Device open failed");
        exit(1);
    }
    // Writing to objects
    i=0;
    do{
        pid=fork();
        i++;
    }
    while(i<(number_of_processes-1) && pid != 0);
    pid=(int)getpid();
    sprintf(filename,"npheap.%d.log",pid);
    fp = fopen(filename,"w");

    srand((int)time(NULL)+(int)getpid());
    START_TX(npheap_dev, tnpheap_dev);
    for(i = 0; i < number_of_objects; i++)
    {
//        npheap_lock(devfd,i);
        size = npheap_getsize(npheap_dev,i);
        while(size ==0)
        {
            size = rand()%(8192);
        }
        mapped_data = (char *)tnpheap_alloc(npheap_dev,tnpheap_dev,i,size);
        if(!mapped_data)
        {
            fprintf(stderr,"Failed in npheap_alloc()\n");
            exit(1);
        }
        memset(mapped_data, 0, size);
        a = rand() + 1;
        gettimeofday(&current_time, NULL);
        for(j = 0; j < size-10; j=strlen(mapped_data))
        {
            sprintf(mapped_data,"%s%d",mapped_data,a);
        }
        fprintf(fp,"S\t%d\t%ld\t%d\t%lu\t%s\n",pid,current_time.tv_sec * 1000000 + current_time.tv_usec,i,strlen(mapped_data),mapped_data);
    }
    COMMIT(npheap_dev, tnpheap_dev);
//    tnpheap_commit(devfd);
/*    
    // try delete something
    i = rand()%256;
    npheap_lock(devfd,i);
    npheap_delete(devfd,i);
    fprintf(fp,"D\t%d\t%ld\t%d\t%lu\t%s\n",pid,current_time.tv_sec * 1000000 + current_time.tv_usec,i,strlen(mapped_data),mapped_data);
    npheap_unlock(devfd,i);*/
    close(npheap_dev);
    close(tnpheap_dev);
    if(pid != 0)
        wait(NULL);
    exit(0);
    return 0;
}

