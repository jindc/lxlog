
#ifndef LXLOG_H 
#define LXLOG_H 
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

/*log level*/ 
#define LX_LOG_OFF      -1 
#define LX_LOG_FATAL    0
#define LX_LOG_ERROR    1
#define LX_LOG_WARN     2 
#define LX_LOG_INFO     3 
#define LX_LOG_DEBUG    4 

#define LX_LOG_ERR 1
#define LX_LOG_NOERR 0

#define LX_LOG_DIR_LENGTH 1024
#define LX_LOG_FILE_LENGTH 1024
#define LX_LOG_MAXLEN 1024

//#define LXLOG_TLOCK_D
//#define LXLOG_PLOCK_D

/*log prefix*/ 
static char * lxlog_level_str[]= {"fatal","error","warn","info","debug" };

struct lxlog_dailyas
{
    int newhour;//0~23,hour to reopen new log file, -1 to off 
    time_t expiretime;
};
typedef struct lxlog_dailyas lxlog_dailyas;

#define lxlog_lockfile "lxlog.lock"

typedef struct lxlog lxlog;
struct lxlog 
{
    char dir[LX_LOG_DIR_LENGTH] ;
    char fname[LX_LOG_FILE_LENGTH];
    char fpath[LX_LOG_FILE_LENGTH];
    int fd;
    FILE * fh;
    pthread_mutex_t tmutex;    
    int loglevel;
    int flushnow;
    int showpid;
    int showtid;

    int (*init)(lxlog * ,const char * dir,const char * fname,int loglevel );
    int (*reopen)(lxlog *, const char * dir,const char * fname);
    int (*cleanup)(lxlog *);
   
    void * arg;
    lxlog_dailyas defaultarg;
    int (*asinit)(lxlog *);
    int (*autosplit)(lxlog *,char *log);
    int (*ascleanup)(lxlog *);
    
    int tlockflag;
    int (*lock)(lxlog *);
    int (*unlock)(lxlog *);
    
    int lockfd;
    int plockflag;
    int (*plockinit)(lxlog *);
    int (*plock)(lxlog *);
    int (*punlock)(lxlog *);    
    int (*plockcleanup)(lxlog * );

    char * (*tostring)(lxlog *);

    void (*hdnoplace)(lxlog * ,char * );
    void (*log_quit_onerr)(lxlog *,int,char *,...);

    int (*logcore)(lxlog *,int,int,char *,va_list);
    int (*logfatal)(lxlog *,char *,...);
    int (*logerror)(lxlog *,char *,...);
    int (*logwarn)(lxlog *,char *,...);
    int (*loginfo)(lxlog *,char *,...);
    int (*logdebug)(lxlog *,char *,...);
    
};            


extern int lxlog_init(lxlog * ,const char * dir,const char * fname,int loglevel );
extern char * lxlog_tostring(lxlog * hlog);
extern int lxlog_reopen(lxlog *hlog, const char * dir,const char * fname);
extern int lxlog_cleanup(lxlog *hlog);

extern int lxlog_daily_asinit(lxlog *);
extern int lxlog_daily_autosplit(lxlog *,char * log);
extern int lxlog_daily_ascleanup(lxlog *);

extern int lxlog_lock(lxlog *);
extern int lxlog_unlock(lxlog *);

extern int lxlog_plockinit(lxlog*);
extern int lxlog_plock(lxlog *);
extern int lxlog_punlock(lxlog *);
extern int lxlog_plockcleanup(lxlog *);

extern void lxlog_hdnoplace(lxlog *,char *);
extern void lxlog_log_quit_onerr(lxlog *,int errflag,char * fmt, ...);

extern int lxlog_logcore(lxlog *,int,int,char *,va_list);
#define lxlog_log(level) \
extern int lxlog_log##level(lxlog*hlog, char *fmt, ...);
lxlog_log(fatal)
lxlog_log(error)
lxlog_log(warn)
lxlog_log(info)
lxlog_log(debug)

#define newlxlog(hlog) \
{\
    memset(hlog,0,sizeof(hlog));\
    \
    hlog->init = lxlog_init;\
    hlog->reopen = lxlog_reopen;\
    hlog->cleanup = lxlog_cleanup;\
\
    hlog->asinit = lxlog_daily_asinit;\
    hlog->autosplit = lxlog_daily_autosplit;\
    hlog->ascleanup = lxlog_daily_ascleanup;\
    \
    hlog->arg = (void * )&hlog->defaultarg;\
    hlog->lock = lxlog_lock;\
    hlog->unlock = lxlog_unlock;\
    \
    hlog->plockinit = lxlog_plockinit;\
    hlog->plock = lxlog_plock;\
    hlog->punlock = lxlog_punlock;\
    hlog->plockcleanup = lxlog_plockcleanup;\
    \
    hlog->tostring = lxlog_tostring;\
    hlog->hdnoplace = lxlog_hdnoplace;\
    hlog->log_quit_onerr = lxlog_log_quit_onerr;\
    \
    hlog->logcore = lxlog_logcore;\
    hlog->logfatal = lxlog_logfatal;\
    hlog->logerror = lxlog_logerror;\
    hlog->logwarn = lxlog_logwarn;\
    hlog->loginfo = lxlog_loginfo;\
    hlog->logdebug = lxlog_logdebug;\
}

#endif

