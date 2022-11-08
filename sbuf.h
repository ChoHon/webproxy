#include <semaphore.h>
#include "csapp.h"

typedef struct
{
    int *buf;    // 버퍼
    int n;       // 빈칸 최대 개수
    int front;   // buf[(front + 1) % n] = 첫번째 item
    int rear;    // buf[rear % n] = 마지막 item
    sem_t mutex; // 버퍼 접근에 대한 뮤텍스
    sem_t slots; // 사용 가능한 빈칸 세마포어
    sem_t items; // 사용 가능한 item 세마포어
} sbuf_t;

void sbuf_init(sbuf_t *sp, int n);
void sbuf_deinit(sbuf_t *sp);

void sbuf_insert(sbuf_t *sp, int item);
int sbuf_remove(sbuf_t *sp);