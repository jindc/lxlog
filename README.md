# lxlog
    lxlog是一个用C语言编写的支持线程安全和进程安全的日志模块。
    
    支持6级log OFF,FATAL,ERROR,WARN,INFO,DEBUG
    
    支持打开或者关闭线程锁
    支持打开或者关闭进程锁
    
    默认日志转储策略为在每日指定时间转储
    可定制的转储策略
    
    支持定制显示进程id,线程id

    进程锁用文件锁实现

编译：
    编译时要用到我的另一个基础库lxlib。编译时要先下载下来 https://github.com/jindc/lxlib.git。
    ./build.sh
    cd test
    ./test -h 查看用法
    
测试程序用法：
    usage:test [-p process_num] [-t thread_num] [-n looptimes]  [-f] [-m]
    -p 指定测试的并发进程数
    -t 指定并发的线程数
    -n 每个线程写多少次log
    -f 关闭进程锁
    -m 关闭线程锁
    
    run_report.txt 是对各种情况的一个测试报告，可以作为一个参考    
作者：德才
email: jindc@163.com
