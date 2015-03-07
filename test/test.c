#include <pthread.h>
#include <getopt.h>
#include <sys/time.h>
#include <stdlib.h>
#include "lxlog.h"
#include "lxtime.h"
#include "lxprocess.h"


int g_inval = 0;
int g_tlock = 1;
int g_plock = 1;
static void test_write_and_swap();
static void test_mutex(int processnum,int threadnum,int times);

int main(int argc,char ** argv)
{
    int o, processnum = 1, threadnum = 1,looptimes=1 ;
    char * help = "usage:test [-p process_num] [-t thread_num] [-n looptimes] [-i interval] [-f] [-m]\n" ;
    struct timeval sval,eval;

    while( (o = getopt(argc,argv,"fmhp:t:n:i:" )) != -1)
    {
        switch(o){
        case '?':
            exit(1);
        case 'h':
            printf(help);
            exit(EXIT_SUCCESS);
        case 'p':
            processnum = atoi(optarg);
            break;
        case 't':
            threadnum = atoi(optarg);
            break;
        case 'n':
            looptimes = atoi(optarg);
            break;
        case 'm':
            g_tlock = 0;
            break;
        case 'f':
            g_plock = 0;
            break;
        case 'i':
            g_inval = atoi(optarg);
            break;
        }
    } 

    if(optind != argc 
        || processnum <= 0 
        || threadnum <= 0
        || looptimes <= 0 ){
        printf(help);
        exit(EXIT_FAILURE);
    }
    
    gettimeofday(&sval,NULL); 
    test_mutex(processnum,threadnum,looptimes);
    gettimeofday(&eval,NULL);

    printf("program run %s\n", gettimeval(&sval, &eval));

    return 0;
}

static void startthread(int threadnum,int times);

static void test_mutex(int processnum,int threadnum,int times)
{
    int i, status;
    pid_t pid;
    lxprocinfo  procs [processnum];

    for(i = 0; i < processnum;i++)
        procs[i].stat = LX_UNFORK;
    
    for(i = 0; i < processnum;i++)
    {
        if( (pid = fork()) == -1)
        {
            lxkillprocs(procs,processnum);
            printf("fork failed");
            exit(1); 
        }

        if(pid > 0)
        {
            procs[i].stat = LX_UNWAIT;
            procs[i].pid = pid;
            printf("fork work process %ld.(%ld#%ld)\n",(long)pid,(long)getpid(), lxgettid(NULL));
            continue;
        }
         
        if(pid == 0)
        {
           startthread(threadnum,times);
           printf("work process finished .%ld#%ld\n",(long)getpid(), lxgettid(NULL));
           return;
        }
    }

    //master process
    for(i = 0 ; i < processnum; i++)
    {
        if(procs[i].stat == LX_UNWAIT && lxwaitpid(procs[i].pid ))
        {
            printf("waitpid error\n");
            abort();
        }
        procs[i].stat = LX_WAITED;
        printf("wait work process %ld.(%ld#%ld)\n",procs[i].pid,(long)getpid(), lxgettid(NULL));
    }
}

static void * writelog(void * arg)
{
    lxlog log;
    int i,times;    
    char buff[256];
    int pid;
    int tid;
    newlxlog( (&log) );
    ((lxlog_dailyas *)log.arg )->newhour =18;
    if( lxlog_init(&log,"log","access.log",LX_LOG_DEBUG))
    {
        exit(EXIT_FAILURE);
    }
    log.flushnow = 1;
    if(!g_tlock)
        log.tlockflag = 0;
    if(!g_plock)
        log.plockflag = 0;

    times = (int)(long)arg;
    buff[255]=0;
    pid = (long)getpid()%10;
    tid = (long)pthread_self() %10;
    for(i = 0; i < times; ++i)
    {
        if(g_inval > 0)
            sleep(g_inval);
        memset(buff,'0'+pid,128);
        memset(buff+128,'0'+tid,64);
        memset(buff+192,'A'+i%26,63);
        log.loginfo(&log, "%s",buff);
    }
    log.cleanup(&log);
    return 0;
}

static void startthread(int threadnum,int times)
{
    int i,ret;
    pthread_t tid,tids[threadnum];
    for( i = 0; i < threadnum;++i)
    {
        if( ret = pthread_create(&tid,NULL,writelog,(void *)(long)times))
        {
            printf("pthread_create error (%d:%s)\n",ret,strerror(ret));
            exit(EXIT_FAILURE);
        }
        tids[i] = tid;
        printf("create thread (%ld#%ld)\n",(long)getpid(),(long)tid);
   }

   for(i = 0; i < threadnum; ++i)
   {
        pthread_join(tids[i],NULL);
        printf("thread finished.(%d#%ld)\n",(int)getpid(),lxgettid(NULL));
   }
   //printf("process work finished (%d#%ld).\n",(int)getpid(),lxgettid(NULL) );
}

static void test_write_and_swap()
{
    struct lxlog log;
    struct lxlog_dailyas asarg;
  
    newlxlog( (&log));
    asarg.newhour = 18;
    log.arg = &asarg;
    if( lxlog_init(&log,"log","access.log", LX_LOG_DEBUG) )
    {
        exit(-1);
    }
    log.flushnow = 1;
   log.log_quit_onerr(&log,LX_LOG_NOERR,log.tostring( &log));

#define lxlog_testmac(level)   log.log##level(&log,"level:%s", #level " log");  
    lxlog_testmac(error) 
    lxlog_testmac(warn) 
    lxlog_testmac(info) 
    lxlog_testmac(debug) 
      
    while(1){
        log.loglevel = LX_LOG_INFO;
        lxlog_testmac(warn) 
        lxlog_testmac(info) 
        lxlog_testmac(debug) 
        sleep(1);
    }   

    log.cleanup(&log);     
}
