#include <semaphore.h>
#include "cache.h"

cache_header *cache_init()
{
    cache_header *ch;
    ch = (cache_header *)Malloc(sizeof(cache_header));

    ch->head = NULL;
    ch->current_cache_size = 0;
    Sem_init(&ch->w, 0, 1);

    return ch;
}

int cache_append(cache_header *ch, char *url, char *response_header, char *response_body, int filesize)
{
    cache_t *last_cache = ch->head;

    if (filesize > MAX_OBJECT_SIZE)
    {
        printf("=====Response Body is bigger than MAX_OBJECT_SIZE=====\r\n\r\n");
        return 0;
    }

    while (ch->current_cache_size + filesize > MAX_CACHE_SIZE)
        cache_delete_LRU(ch);

    cache_t *new_cache;
    new_cache = (cache_t *)Malloc(sizeof(cache_t));

    strcpy(new_cache->url, url);
    strcpy(new_cache->response_header, response_header);
    strcpy(new_cache->response_body, response_body);
    new_cache->filesize = filesize;
    new_cache->count = 1;
    new_cache->prev = NULL;
    new_cache->next = NULL;

    while (last_cache != NULL && last_cache->next != NULL)
        last_cache = last_cache->next;

    P(&ch->w);
    ch->current_cache_size += filesize;

    if (last_cache == NULL)
    {
        ch->head = new_cache;
    }
    else
    {
        last_cache->next = new_cache;
        new_cache->prev = last_cache;
    }
    V(&ch->w);

    return 1;
}

void cache_delete_LRU(cache_header *ch)
{
    cache_t *lru_cache = ch->head;
    cache_t *prev_cache = NULL;

    if (lru_cache == NULL)
        return;

    while (lru_cache->next != NULL)
    {
        prev_cache = lru_cache;
        lru_cache = lru_cache->next;
    }

    P(&ch->w);
    if (prev_cache == NULL)
        ch->head = NULL;
    else
        prev_cache->next = NULL;

    ch->current_cache_size -= lru_cache->filesize;

    Free(lru_cache);
    V(&ch->w);
}

cache_t *cache_search(cache_header *ch, char *url)
{
    cache_t *cur_cache = ch->head;
    cache_t *temp;

    while (cur_cache != NULL)
    {
        if (!strcmp(cur_cache->url, url))
            break;

        cur_cache = cur_cache->next;
    }

    if (cur_cache == NULL)
        return NULL;
    // else
    // {
    //     cur_cache->count++;

    //     while (cur_cache->prev != NULL && cur_cache->count > cur_cache->prev->count)
    //     {
    //         temp = cur_cache->prev;

    //         if (cur_cache->prev->prev == NULL)
    //         {
    //             ch->head = cur_cache;
    //             cur_cache->next = cur_cache->prev;
    //         }
    //     }
    // }

    return cur_cache;
}

void cache_display(cache_t *cache)
{
    if (cache == NULL)
    {
        printf("Cache is NULL\r\n\r\n");
        return;
    }

    printf("URL : %s\r\n", cache->url);
    printf("filesize : %d\r\n", cache->filesize);
    printf("Next : %p\r\n", cache->next);
    printf("Prev : %p\r\n\r\n", cache->prev);
}

void cache_all_display(cache_header *ch)
{
    cache_t *cur_cache = ch->head;

    while (cur_cache != NULL)
    {
        cache_display(cur_cache);
        cur_cache = cur_cache->next;
    }

    printf("Total Cache Size : %d / %d\r\n\r\n", ch->current_cache_size, MAX_CACHE_SIZE);
}