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

struct data_array_element
{
    int size;
    char *data;
};

int main(int argc, char *argv[])
{
    int i=0,number_of_processes=1, number_of_objects=1024, max_object_size = 65536,j; 
    int a;
    int pid;
    int size;
    __u64 prev_tx =0;
    char data[8192];
    char filename[256];
    char *mapped_data;
    struct data_array_element *data_array;
    int npheap_dev, tnpheap_dev;
    int object_id;
    unsigned long long msec_time;
    FILE *fp;
    struct timeval current_time;

    if(argc < 3)
    {
        fprintf(stderr, "Usage: %s number_of_objects max_object_size number_of_processes\n",argv[0]);
        exit(1);
    }
    number_of_objects = atoi(argv[1]);
    max_object_size = atoi(argv[2]);
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
// Generate input data 
    srand((int)time(NULL)+(int)getpid());
    data_array = (struct data_array_element *)calloc(number_of_objects*2, sizeof(struct data_array_element));
    for(i = 0; i < number_of_objects; i++)
    {
        object_id = rand()%(number_of_objects*2);
        while(size == 0 || size <= 10)
        {
            size = rand()%(max_object_size);
        }
        if(data_array[object_id].size)
            free(data_array[object_id].data);
        data_array[object_id].data = (char *)calloc(size,sizeof(char));
        data_array[object_id].size = size;
        a = rand() + 1;
        for(j = 0; j < size-11; j=strlen(data_array[object_id].data))
        {
            sprintf(data_array[object_id].data,"%s%d",data_array[object_id].data,a);
        }
    }

    START_TX(npheap_dev, tnpheap_dev);
    for(i = 0; i < number_of_objects*2; i++)
    {
        if(data_array[i].size)
        {
            size = data_array[i].size;
            mapped_data = (char *)tnpheap_alloc(npheap_dev,tnpheap_dev,i,size);
            if(!mapped_data)
            {
                fprintf(stderr,"Failed in npheap_alloc()\n");
                exit(1);
            }
            memset(mapped_data, 0, data_array[i].size);
            memcpy(mapped_data, data_array[i].data, data_array[i].size);
        }
    }
    COMMIT(npheap_dev, tnpheap_dev);
    gettimeofday(&current_time,NULL);
    msec_time = current_time.tv_usec + current_time.tv_sec*1000000;

    // print commit log
    pid=(int)getpid();
    sprintf(filename,"tnpheap.%d.log",pid);
    fp = fopen(filename,"w");
    for(i = 0; i < number_of_objects*2; i++)
    {
        if(data_array[i].size)
        {
            size = npheap_getsize(npheap_dev,i);
            mapped_data = (char *)tnpheap_alloc(npheap_dev,tnpheap_dev,i,size);
            if(!mapped_data)
            {
                fprintf(stderr,"Failed in npheap_alloc()\n");
                exit(1);
            }
//            memset(mapped_data, 0, data_array[i].size);
//            memcpy(mapped_data, data_array[i].data, data_array[i].size);
            fprintf(fp,"S\t%d\t%llu\t%d\t%lu\t%s\n",pid,msec_time,i,strlen(data_array[i].data),data_array[i].data);
        }
    }

    close(npheap_dev);
    close(tnpheap_dev);
    fclose(fp);
    if(pid != 0)
    {
        for(i=0;i<number_of_processes;i++)
            wait(NULL);
    }
    exit(0);
    return 0;
}

