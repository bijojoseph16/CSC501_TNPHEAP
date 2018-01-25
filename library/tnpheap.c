#include <npheap/tnpheap_ioctl.h>
#include <npheap/npheap.h>
#include <npheap.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>


//my changes below
#include <errno.h>
//used to store the npheap_dev fd so we can lock locations in tnpheap_handler
int devfd;
//used to store tnpheap_devfd
int tnpheap_fd;
//keep track of the transaction number
__u64 tx_number = 0;

//struct to keep track of the offsets for this transaction
struct offset_struct {
    struct offset_struct * next; //singly linked list
    int read;
    int written;
    void * npheap_data; //offset returned by npheap
    void * buffer_data; //local to this transaction
    __u64 object_size; //how big this object is
    __u64 object_offset;
    __u64 object_version;
};

struct offset_struct * offset_list = NULL; //head of the list


__u64 tnpheap_get_version(int npheap_dev, int tnpheap_dev, __u64 offset)
{
    //printf("Everything all right.\n");
    struct tnpheap_cmd cmd;
    cmd.offset = offset;
     return ioctl(tnpheap_dev, TNPHEAP_IOCTL_GET_VERSION, &cmd);
}


//section 1.3.3 Software Transactional Memory of the linked book
//says that locations updated/written should be automatically locked, 
//and locations that are read should have their version numbers saved
int tnpheap_handler(int sig, siginfo_t *si)//, ucontext_t *ctx)
{
    void * data = si->si_addr;
    //need to find the correct struct
    struct offset_struct * current = offset_list;
    while (current != NULL) {
        if (current->buffer_data == data) {
            //printf("They match\n");
            break;
        } else {
            //printf("Not the right one %p\n", current->buffer_data);
            current = current->next;
        }
    }
    if (current == NULL) {
        return -1;
    }
    return 0;
}


void *tnpheap_alloc(int npheap_dev, int tnpheap_dev, __u64 offset, __u64 size)
{
    struct offset_struct * current = offset_list;
    //need to allocate a new one
    struct offset_struct * temp_struct = malloc(sizeof(struct offset_struct));
        
    temp_struct->object_offset = offset;
    __u64 aligned_size= ((size + getpagesize() - 1) / getpagesize())*getpagesize();
    temp_struct->object_size = aligned_size;
    void * mem = NULL;
    int retval = posix_memalign(&mem, getpagesize(), temp_struct->object_size);
    //printf("REtval is %d\n", retval);
    if (retval != 0) {
        exit(-1);
    }
    if (mem == NULL) {
        exit(-1);
    }
    temp_struct->buffer_data = mem;
    memset(temp_struct->buffer_data, 0, temp_struct->object_size);
    temp_struct->read = 1;
    temp_struct->written = 0;
    //now insert it
    temp_struct->next = offset_list;
    offset_list = temp_struct;
    struct tnpheap_cmd cmd;
    cmd.offset = temp_struct->object_offset;
    temp_struct->object_version = ioctl(tnpheap_dev, TNPHEAP_IOCTL_GET_VERSION, &cmd);
    //now protect it
    //don't let it be accessed at all. Deal with this in the tnpheap_handler
    //printf("Before protection\n");
    mprotect(temp_struct->buffer_data, temp_struct->object_size, PROT_READ);
    //printf("After protection\n");
     return temp_struct->buffer_data;
}

__u64 tnpheap_start_tx(int npheap_dev, int tnpheap_dev)
{
    struct tnpheap_cmd cmd;
    //save the device fd, so they can be used in the segfault handler
    devfd = npheap_dev;
    tnpheap_fd = tnpheap_dev;
    offset_list = NULL;
    tx_number = ioctl(tnpheap_dev, TNPHEAP_IOCTL_START_TX, &cmd);
    return tx_number;
}

/*return 1 on abort, 0 on commit */
int tnpheap_commit(int npheap_dev, int tnpheap_dev)
{
    int success = 0;
	int locked = 0;
    struct offset_struct * current = offset_list;
	if (current != NULL) {
		locked = 1;
		npheap_lock(npheap_dev, 0);
}
    while (current != NULL) {
       struct tnpheap_cmd cmd;
       cmd.offset = current->object_offset;
       __u64 curr_version = ioctl(tnpheap_dev, TNPHEAP_IOCTL_GET_VERSION, &cmd); 
        //if it has been read, need to check the version number
        if (current->read) {
            if (current->object_version != curr_version) {
               success = 1;
               break;
            }
        }
        current = current->next;
    }
    if (success) {
        //printf("Something changed. Aborting\n"); 
        //need to abort
        current = offset_list;
        while (current != NULL) {
            free(current->buffer_data);
            //shouldn't need to free/delete the npheap_data
            struct offset_struct * temp = current;
            current = current->next;
            free(temp);
        }
    } else {
        current = offset_list;
        while (current != NULL) {
            if (current->written) {
		npheap_delete(npheap_dev, current->object_offset);
		current->npheap_data = npheap_alloc(npheap_dev, current->object_offset, current->object_size);
		memset(current->npheap_data, 0, current->object_size);
                memcpy(current->npheap_data, current->buffer_data, current->object_size);
                struct tnpheap_cmd com_cmd;
                com_cmd.offset = current->object_offset;
                com_cmd.version = tx_number;
                ioctl(tnpheap_dev, TNPHEAP_IOCTL_COMMIT, &com_cmd);
            }
            //printf("Freeing buff data\n");
            free(current->buffer_data);
            struct offset_struct * temp = current;
            current = current->next;
            //printf("Freeing whole node\n");
            free(temp);
        }
    }
if (locked) {
	npheap_unlock(npheap_dev, 0);
}
    offset_list = NULL;
    return success;
}

