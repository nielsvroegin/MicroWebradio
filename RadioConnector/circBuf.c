// Circ buf based on http://embedjournal.com/implementing-circular-buffer-embedded-c/

#include "circBuf.h"

bit circBufPush(circBuf_t *c, unsigned char data)
{
    unsigned char next = c->head + 1;
    if (next >= c->maxLen)
        next = 0;
 
    // Cicular buffer is full
    if (next == c->tail)
        return 0;  // quit with an error
 
    c->buffer[c->head] = data;
    c->head = next;
    return 1;
}
 
bit circBufPop(circBuf_t *c, unsigned char *data)
{
    // if the head isn't ahead of the tail, we don't have any characters
    if (c->head == c->tail)
        return 0;  // quit with an error
 
    *data = c->buffer[c->tail];
    c->buffer[c->tail] = 0;  // clear the data (optional)
 
    unsigned char next = c->tail + 1;
    if(next >= c->maxLen)
        next = 0;
 
    c->tail = next;
 
    return 1;
}
