#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <mqueue.h>
#include <signal.h>

#define QUEUE_NAME "/fg_queue"
#define MAX_MSG_COUNT 10
#define PROCESS_COUNT 2

using namespace std;

struct message {
    char funcName;
    int funcResult;
};


int f(int x) {
    sleep(5);
    return 1 / x;
}

int g(int x) {
    sleep(15);
    return x;
}

int main()
{
    pid_t pid[PROCESS_COUNT];
    int args[PROCESS_COUNT];
    int flags = O_CREAT | O_RDWR | O_NONBLOCK;
    mode_t perms = 0777;
    mqd_t mqd;
    struct mq_attr attr;
    attr.mq_maxmsg = MAX_MSG_COUNT;
    attr.mq_msgsize = sizeof(message);
    
    mqd = mq_open(QUEUE_NAME, flags, perms, &attr);
                 
    if (mqd == (mqd_t) -1){
        perror("mq_open");
        exit(EXIT_FAILURE);
    }
    
    cout << "Argument for f(x): ";
    cin >> args[0];
    cout << "Argument for g(x): ";
    cin >> args[1];
    
    pid[0] = fork();
    if (pid[0] == 0) {
        int res = f(args[0]);
        struct message msgSend;
        msgSend.funcName = 'f';
        msgSend.funcResult = res;
        if (mq_send(mqd, reinterpret_cast<const char*>(&msgSend), sizeof(message), 0) == -1){
            perror("mq_send");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    
    pid[1] = fork();
    if (pid[1] == 0) {
        int res = g(args[1]);
        struct message msgSend;
        msgSend.funcName = 'g';
        msgSend.funcResult = res;
        if (mq_send(mqd, reinterpret_cast<const char*>(&msgSend), sizeof(message), 0) == -1){
            perror("mq_send");
            exit(EXIT_FAILURE);
        }
        exit(EXIT_SUCCESS);
    }
    
    struct message msgReceives[2];
    ssize_t numRead;
    int calc_mode = 1;
    int timeToSleep = 1;
    int resCount = 0;
    int isComplete = 0;
    int result = 0;
    
    while (resCount < 2) {
        struct message msgReceive;
        if (calc_mode == 1 || calc_mode == 3) {
            sleep(timeToSleep);
            if (timeToSleep < 10) {
                timeToSleep++;
            }
            numRead = mq_receive(mqd, reinterpret_cast<char*>(&msgReceive), sizeof(msgReceive), NULL);
            if (numRead == -1 && errno == EAGAIN) {
                if (calc_mode == 1) {
                    cout << "Calculations are not completed. Do you want to continue? (1 - yes; 2 - no; 3 - yes without questioning): ";
                    cin >> calc_mode;
                }
            } else if (numRead == -1) {
                perror("mq_receive");
            } else {
                msgReceives[resCount].funcName = msgReceive.funcName;
                msgReceives[resCount].funcResult = msgReceive.funcResult;
                resCount++;
                if (msgReceive.funcResult != 0) {
                    result = msgReceive.funcResult || false;
                    isComplete = 1;
                    break;
                }
            }
        }else{
            break;
        }
    }
    
    if (isComplete == 1 || resCount == 2) {
        cout << "Result: " << (result != 0 ? "True" : "False") << endl;
    }else{
        cout << "Sorry, we can't solve your problem." << endl;
    }
    
    mq_unlink(QUEUE_NAME);
    
    for (int i = 0; i < PROCESS_COUNT; i++) {
        kill(pid[i], SIGKILL);
    }
}
