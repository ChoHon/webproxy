#include "sbuf.h"

void sbuf_init(sbuf_t *sp, int n)
{
    sp->buf = Calloc(n, sizeof(int)); // 버퍼 할당
    sp->n = n;                        // 최대 크기
    sp->front = sp->rear = 0;         // 첫 item과 마지막 item 위치
    Sem_init(&sp->mutex, 0, 1);       // 뮤텍스
    Sem_init(&sp->slots, 0, n);       // 빈칸 세마포어
    Sem_init(&sp->items, 0, 0);       // item 세마포어
}

void sbuf_deinit(sbuf_t *sp)
{
    Free(sp->buf);
}

void sbuf_insert(sbuf_t *sp, int item)
{
    P(&sp->slots); // 빈칸 하나 줄어들고
    P(&sp->mutex); // 뮤텍스 락

    sp->buf[(++sp->rear) % (sp->n)] = item;

    V(&sp->mutex); // 뮤텍스 언락
    V(&sp->items); // 아이템 하나 증가
}

int sbuf_remove(sbuf_t *sp)
{
    int item;

    P(&sp->items); // 아이템 하나 줄어들고
    P(&sp->mutex); // 뮤텍스 락

    item = sp->buf[(++sp->front) % (sp->n)];

    V(&sp->mutex); // 뮤텍스 언락
    V(&sp->slots); // 빈칸 하나 증가

    return item;
}