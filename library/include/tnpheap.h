#ifdef __cplusplus
extern "C" {
#endif
#include <unistd.h>
#include <signal.h>
__u64 current_tx;
__u64 tnpheap_start_tx(int npheap_dev, int tnpheap_dev);
int tnpheap_commit(int npheap_dev, int tnpheap_dev);
void *tnpheap_alloc(int npheap_dev, int tnpheap_dev, __u64 offset, __u64 size);
__u64 tnpheap_get_version(int npheap_dev, int tnpheap_dev, __u64 offset);
static void handler(int sig, siginfo_t *si, void *unused) {
           tnpheap_handler(sig, si);
}

int tnpheap_init()
{
    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sa.sa_sigaction = handler;
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        fprintf(stderr,"sigaction");
        exit(1);
    }
}
#define START_TX(npheap,tnpheap) do{ current_tx = tnpheap_start_tx(npheap,tnpheap);
#define COMMIT(npheap,tnpheap) } while(tnpheap_commit(npheap,tnpheap));
#ifdef __cplusplus
}
#endif
