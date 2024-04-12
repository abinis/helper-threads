#ifndef BFS_H__
#define BFS_H__

enum color { 'A' = 0, 'Y', 'E' };

typedef struct queue_st {
    int head;
    int tail;
    int *queue;
} queue_t;

void bfs_init(adjlist_t *al/*, enum color **vertex_color, queue_t **vertex_queue*/);
void bfs(adjlist_t *al, int root/*, enum color *vertex_color, queue_t *vertex_queue*/);
void bfs_destroy(/*enum color *vertex_color, queue_t *vertex_queue*/);


#endif
