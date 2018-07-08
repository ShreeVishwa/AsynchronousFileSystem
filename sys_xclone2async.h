
#ifndef _SYS_XCLONE2ASYNC_H
#define _SYS_XCLONE2ASYNC_H
#endif

// #include "params.h"

#define max_producers 2
#define max_consumers 2
#define WQ_maxsize 2



struct wq_data{
   char * fname;
  u_int b1;

 };
struct task_struct * consum[max_consumers];
struct wq_node
{

    struct wq_data *wq_data_node;
    struct list_head wq_list;
};

struct w_queue{
    struct wq_node *w_qnode;
    struct mutex wq_mutex;
    atomic_t wq_size;

};
atomic_t t_producers;

static wait_queue_head_t consumer_wq,producer_wq;
struct w_queue *wi_queue;