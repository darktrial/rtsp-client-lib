#define NUMBER_OF_THREADS 3
typedef struct _threadinfo
{
    char inUse;
    char stopflag;
    char eventLoopWatchVariable;
}threadinfo;