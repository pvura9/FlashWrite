#ifndef __FIFO_H__
#define __FIFO_H__

struct fifo 
{
    //circular buffer for fifo
    char buffer[128];
    //first thing to remove from fifo
    volatile uint8_t head;
    //next place to insert new char
    volatile uint8_t tail;
    //offset of previously entered newline
    volatile uint8_t newline;
};

int fifo_empty(const struct fifo *f);
int fifo_full(const struct fifo *f);
void fifo_insert(struct fifo *f, char ch);
char fifo_uninsert(struct fifo *f);
int fifo_newline(const struct fifo *f);
char fifo_remove(struct fifo *f);

#endif /* __FIFO_H__ */