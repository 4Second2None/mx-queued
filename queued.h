#ifndef __MX_QUEUED_H
#define __MX_QUEUED_H

#include "list.h"
#include "skiplist.h"
#include "hash.h"
#include "ae.h"

#define MX_VERSION "0.02"
#define MX_SERVER_PORT 21021
#define MX_LOG_FILE "./mx-queue.log"
#define MX_RECVBUF_SIZE 2048
#define MX_SENDBUF_SIZE 2048
#define MX_FREE_CONNECTIONS_LIST_SIZE 1000

#ifndef offsetof
#define offsetof(type, member) ((size_t)&((type *)0)->member)
#endif

#define mx_item_data(item) (item)->data
#define mx_item_size(item) (item)->length

#define mx_dirty_update() do { mx_daemon->dirty++; } while(0)
#define mx_dirty_clean()  do { mx_daemon->dirty = 0; } while(0)

#define mx_recycle_inc() do { mx_daemon->recycle->count++; } while(0)
#define mx_recycle_dec() do { mx_daemon->recycle->count--; } while(0)

#define MX_CONNECTION_WATCHER 1


typedef struct mx_connection_s mx_connection_t;
typedef struct mx_queue_item_s mx_queue_item_t;

typedef enum {
    mx_log_error,
    mx_log_notice,
    mx_log_debug
} mx_log_level;


typedef enum {
    mx_send_header_phase,
    mx_send_body_phase
} mx_send_item_phase;


typedef enum {
    mx_event_reading,
    mx_event_writing
} mx_event_state;


typedef struct mx_queue_s {
    struct list_head watcher;
    mx_skiplist_t *list;
    int  name_len;
    char name_val[0];
} mx_queue_t;


struct mx_queue_item_s {
    int prival;
    int delay;
    mx_queue_t *belong;
    int length;
    char data[0];
};


typedef struct mx_recycle_s {
    mx_skiplist_t *recycle;
    int last_id;
    int count;
} mx_recycle_t;


typedef struct mx_recycle_item_s {
    int id;
    int expire;
    mx_queue_item_t *item;
} mx_recycle_item_t;


struct mx_connection_s {
    int fd;
    mx_event_state state;
    int flag;
    char *recvbuf;
    char *recvpos;
    char *recvlast;
    char *recvend;
    char *sendbuf;
    char *sendpos;
    char *sendlast;
    char *sendend;
    void (*rev_handler)(mx_connection_t *conn);
    void (*wev_handler)(mx_connection_t *conn);
    mx_queue_item_t *item;
    char *itemptr;
    int itembytes;
    struct list_head watch;
    mx_send_item_phase phase;
    int sent_recycle;
    int recycle_id;
    mx_connection_t *free_next;
};


typedef struct mx_daemon_s {
    int fd;
    aeEventLoop *event;
    short port;
    int daemon_mode;
    char *log_file;
    FILE *log_fd;
    char *conf_file;
    HashTable *table;
    mx_queue_t *delay_queue;
    /* support to persistence feature */
    char *bgsave_filepath;
    pid_t bgsave_pid;
    time_t last_bgsave_time;
    int bgsave_rate;
    int changes_todisk;
    int dirty;
    mx_recycle_t *recycle;
    int recycle_expire;
    mx_connection_t *free_connections;
    int free_connections_count;
} mx_daemon_t;


extern mx_daemon_t *mx_daemon;
extern time_t mx_current_time;

/*
 * Queue API
 */
#define mx_queue_insert(queue, key, item)     mx_skiplist_insert((queue)->list, (key), (item))
#define mx_queue_fetch_head(queue, retval)   (mx_skiplist_find_min((queue)->list, (retval)) == SKL_STATUS_OK)
#define mx_queue_delete_head(queue)           mx_skiplist_delete_min((queue)->list)
#define mx_queue_size(queue)                  mx_skiplist_elements((queue)->list)

void mx_write_log(mx_log_level level, const char *fmt, ...);
mx_queue_t *mx_queue_create(char *name, int name_len);
mx_queue_item_t *mx_queue_item_create(int prival, int delay, mx_queue_t *belong, int size);
int mx_try_bgsave_queue();
int mx_load_queue();

#endif
