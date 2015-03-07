#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include "lxlog.h"
#include "lxpath.h"
#include "lx_fileio.h"
#include "lxtime.h"
#include "lxflock.h"

#define FILE_MODE (S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP)

void lxlog_hdnoplace(lxlog *hlog ,char * buff)
{
    writen(STDERR_FILENO,buff,strlen(buff));
}

void lxlog_log_quit_onerr(lxlog *hlog,int errflag,char * fmt,... )
{
    assert(fmt);

    char buff[LX_LOG_MAXLEN];
    char errbuff[LX_LOG_MAXLEN];
    int errlen = 0;
    
    errlen += getwidetime(time(NULL),buff+errlen,LX_LOG_MAXLEN - errlen); 
    errlen += snprintf(buff+errlen,LX_LOG_MAXLEN -errlen, " " );

    va_list ap;
    va_start(ap,fmt);
    errlen += vsnprintf(buff + errlen,LX_LOG_MAXLEN - errlen,fmt,ap);
    va_end(ap);
    
    if(errflag)
    {
        strerror_r(errno,errbuff,LX_LOG_MAXLEN  );
        errlen +=snprintf(buff + errlen,LX_LOG_MAXLEN - errlen,"( %d:%s)",errno,errbuff);
    }
    errlen += snprintf(buff+errlen,LX_LOG_MAXLEN -errlen, "\n" );
    
    hlog->hdnoplace(hlog, buff); 
    if(errflag)
    {
        abort();
    }
}

int lxlog_openlog(lxlog * hlog)
{
    int lasti;
    char c; 
    lasti = strlen(hlog->dir) -1;
    c = hlog->dir[lasti];
    if(c == '/' ||c == '\\')
        hlog->dir[lasti] = 0; 
    snprintf(hlog->fpath,LX_LOG_FILE_LENGTH,"%s/%s",hlog->dir,hlog->fname);    
    
    hlog->fh = fopen(hlog->fpath,"a");
    if(hlog->fh == NULL)
    {
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"open log file [%s] failed.",hlog->fpath);
    }
    return 0; 
}

int lxlog_reopen(lxlog *hlog, const char * dir,const char * fname)
{
    if(fclose(hlog->fh) )
    {
        hlog->log_quit_onerr(hlog,1,"close file [%s] failed",hlog->fpath);
    }
    
    snprintf(hlog->dir,LX_LOG_DIR_LENGTH,"%s",dir);
    snprintf(hlog->fname,LX_LOG_FILE_LENGTH,"%s",fname);
    lxlog_openlog(hlog);
 
    return 0; 
} 

int lxlog_init(lxlog * hlog,const char * dir, const char * fname,int loglevel)
{
    int ret;
    int c,lasti;

    hlog->loglevel = loglevel;
    hlog->flushnow = 1;
    hlog->showpid = 1;
    hlog->showtid = 1;
    
    hlog->tlockflag = 1;
    hlog->plockflag = 1;
        
    snprintf(hlog->dir,LX_LOG_DIR_LENGTH,"%s",dir);
    snprintf(hlog->fname,LX_LOG_FILE_LENGTH,"%s",fname);
    
    if(ret = hlog->plockinit(hlog))
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"plock init error");
    
    if( ret = pthread_mutex_init(&(hlog->tmutex), NULL ))
    {
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"pthread_init_mutex error"); 
    }
    
    if(hlog->asinit)
    {
        ret = hlog->asinit(hlog);
        if(ret)
            hlog->log_quit_onerr(hlog,LX_LOG_ERR,"hlog->asinit error"); 
    }
    
    lxlog_openlog(hlog); 
     
    return 0; 
}

int lxlog_cleanup(lxlog *hlog)
{
    int ret;
    
    if(hlog->ascleanup)
    {
        if( hlog->ascleanup(hlog))
            hlog->log_quit_onerr(hlog,LX_LOG_ERR,"hlog->ascleanup error");
    }

    if(ret = pthread_mutex_destroy( &(hlog->tmutex)) )
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"pthread_mutex_destroy error");
    
    if(hlog->plockcleanup)
        if(ret = hlog->plockcleanup(hlog) )
            hlog->log_quit_onerr(hlog,LX_LOG_ERR,"plockcleanup error");

    if(ret = hlog->plockcleanup(hlog))
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"plockcleanup error");

    if(fclose(hlog->fh) )
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"close file [%s,%s] failed",hlog->fpath);
        
    hlog->fh = NULL; 
    return 0;
}

static inline int append_timefix(char * buff, size_t size)
{
    char subfix[64];
    getnarrowtime(time(NULL),subfix+1,64-1);
    *subfix = '.';
    strncat(buff,subfix,size);
    return 0;
}

int lxlog_daily_asinit(lxlog * hlog)
{
    lxlog_dailyas *arg = (lxlog_dailyas *) hlog->arg;
    arg->expiretime = getnexthour(time(NULL),arg->newhour);
    if(arg->expiretime <=0)
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"getnexthour() error");

    return append_timefix(hlog->fname, LX_LOG_FILE_LENGTH);
}

int lxlog_daily_autosplit(lxlog *hlog, char * log)
{
    int ret;

    char dir[LX_LOG_DIR_LENGTH];
    char fname[LX_LOG_FILE_LENGTH];
    char * dot ;
    
    lxlog_dailyas *arg = (lxlog_dailyas *) hlog->arg; 
    
    if( difftime(time(NULL), arg->expiretime) < 0)
        return 0;     
    
    arg->expiretime = getnexthour(time(NULL),arg->newhour);
    
    snprintf(dir,LX_LOG_DIR_LENGTH,hlog->dir);
    snprintf(fname,LX_LOG_FILE_LENGTH,hlog->fname);
    dot = strrchr(fname,'.');
    if(dot)
        *dot = 0;    
    append_timefix(fname,LX_LOG_FILE_LENGTH);
    
    ret = hlog->reopen(hlog,dir,fname);
    if(ret)     
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"hlog->reopen error");
    return 0;
}

int lxlog_daily_ascleanup(lxlog *hlog)
{
    return 0;
}

char * lxlog_tostring(lxlog * hlog)
{
    static char buff[1024];
    snprintf(buff,1024,"%s,%s,loglevel:%d,flushnow:%d,thread lock:%d,process lock:%d\n",
                hlog->dir,hlog->fname,hlog->loglevel,hlog->flushnow,hlog->tlockflag,hlog->plockflag );
    return buff;
}

int lxlog_plockinit(lxlog * hlog)
{
    int ret;
    char path[LX_LOG_FILE_LENGTH];

    snprintf(path,LX_LOG_FILE_LENGTH,"%s/%s",hlog->dir,lxlog_lockfile);
    if( (ret = open(path,O_CREAT|O_RDWR,FILE_MODE) ) == -1){
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"open lock file(%s) error",path);
    }

    hlog->lockfd = ret;
    return 0;
}

int lxlog_plock(lxlog * hlog)
{
    while(lx_lwritew(hlog->lockfd,SEEK_SET,0,0))
    {
        hlog->log_quit_onerr(hlog,LX_LOG_NOERR,"get lock error(%d:%s)",errno,strerror(errno));
    }
    return 0;
}

int lxlog_punlock(lxlog * hlog)
{
    while(lx_lunlock(hlog->lockfd,SEEK_SET,0,0))
        hlog->log_quit_onerr(hlog,LX_LOG_NOERR,"unlock error(%d:%s)",errno,strerror(errno));
    return 0;
}

int lxlog_plockcleanup(lxlog * hlog){return 0;};

int lxlog_lock(lxlog *hlog)
{
   int ret = pthread_mutex_lock( &(hlog->tmutex) );
   if(ret){
       errno = ret;
       hlog->log_quit_onerr(hlog,LX_LOG_ERR,"pthread_mutex_lock error");
   }
   return 0;
}

int lxlog_unlock(lxlog *hlog)
{
    int ret = pthread_mutex_unlock( &(hlog->tmutex));
    if(ret)
    {
        errno = ret;
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"pthread_mutex_unlock error");
    }
    return 0;
}

int lxlog_logcore(lxlog * hlog,int errflag,int loglevel,char * fmt,va_list ap)
{
    assert(fmt);

    char buff[LX_LOG_MAXLEN];
    char errbuff[LX_LOG_MAXLEN];    
    int errlen = 0,ret;

    if(loglevel > hlog->loglevel)
        return 0;
    
    errlen += getwidetime(time(NULL),buff,LX_LOG_MAXLEN); 
    errlen += snprintf(buff + errlen,LX_LOG_MAXLEN - errlen," [%-5s] ",lxlog_level_str[loglevel]);
    
    if(hlog->showpid)
        errlen += snprintf(buff + errlen,LX_LOG_MAXLEN - errlen," %ld ",(long)getpid());
    
    if(hlog->showtid)
        errlen += snprintf(buff + errlen,LX_LOG_MAXLEN - errlen," %ld ",(long)pthread_self());

    errlen += vsnprintf(buff + errlen ,LX_LOG_MAXLEN -errlen,fmt,ap);

    if(errflag)
    {
        strerror_r(errno,errbuff,LX_LOG_MAXLEN  );
        errlen += snprintf(buff +errlen,LX_LOG_MAXLEN - errlen,"( %d:%s)",errno,errbuff);
    }
    strncat(buff + errlen,"\n",LX_LOG_MAXLEN - errlen);
    
    if(hlog-> plockflag)
        hlog->plock(hlog);

    if(hlog->tlockflag)    
        hlog->lock(hlog);

    if(hlog->autosplit)
    {
        if( hlog->autosplit(hlog,buff))
            hlog->log_quit_onerr(hlog,LX_LOG_ERR,"hlog->autosplit error");
    }
    
    ret = fwriten(hlog->fh,buff,strlen(buff));
    if(ret <(int) strlen(buff))
    {
        hlog->log_quit_onerr(hlog,LX_LOG_ERR,"write to log [%s,%s] error",hlog->dir,hlog->fname);
    }
    if(hlog->flushnow)
        fflush(hlog->fh);

    if(hlog->tlockflag)
        hlog->unlock(hlog);

    if(hlog->plockflag)
        hlog->punlock(hlog);

    return 0;
}

#define lxlog_logdefine(name,errflag,level) \
 int  lxlog_log##name(lxlog *hlog, char * fmt,...)\
{\
    va_list ap;\
    va_start(ap,fmt);\
    int ret = hlog->logcore(hlog, errflag,level,fmt,ap);\
    va_end(ap);\
    if(!strcmp("fatal",#name)) abort();\
    return ret; \
}

lxlog_logdefine(fatal,LX_LOG_ERR,LX_LOG_FATAL)
lxlog_logdefine(error,LX_LOG_ERR,LX_LOG_ERROR)
lxlog_logdefine(warn,LX_LOG_NOERR,LX_LOG_WARN)
lxlog_logdefine(info,LX_LOG_NOERR,LX_LOG_INFO)
lxlog_logdefine(debug,LX_LOG_NOERR,LX_LOG_DEBUG)



