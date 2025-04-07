#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>

void timer_handler(int sig) {
    printf("Timer expired! Executing task...\n");
}

void timer_handler2(int sig) {
    printf("Timer2 expired! Executing task...\n");
}

int main() {



    
    struct itimerval timer;
    // 设置信号处理
    signal(SIGALRM, timer_handler);

     // 配置定时器：首次5秒后，之后每5秒一次
     timer.it_value.tv_sec = 5;
     timer.it_value.tv_usec = 0;
     timer.it_interval.tv_sec = 5;
     timer.it_interval.tv_usec = 0;

   
     setitimer(ITIMER_REAL, &timer, NULL);
     printf("Timer started (5 second intervals)...\n");


      // 设置信号处理
      signal(SIGALRM, timer_handler2);
    
   
      struct itimerval timer2;
        // 配置定时器：首次5秒后，之后每5秒一次
        timer2.it_value.tv_sec = 10;
        timer2.it_value.tv_usec = 0;
        timer2.it_interval.tv_sec = 10;
        timer2.it_interval.tv_usec = 0;
 

    setitimer(ITIMER_REAL, &timer2, NULL);
    printf("Timer2 started (10 second intervals)...\n");
    

    
    while(1) {
        pause();
    }
    
    return 0;
}