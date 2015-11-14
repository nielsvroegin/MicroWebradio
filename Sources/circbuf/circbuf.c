// Circ buf based on http://embedjournal.com/implementing-circular-buffer-embedded-c/

#include "circbuf.h"

bool circBufPush(circBuf_t *c, char data)
{
    unsigned int next = c->head + 1;
    if (next >= c->maxLen)
        next = 0;
 
    // Cicular buffer is full
    if (next == c->tail)
        return 0;  // quit with an error
 
    c->buffer[c->head] = data;
    c->head = next;
    return 1;
}
 
bool circBufPop(circBuf_t *c, char *data)
{
    // if the head isn't ahead of the tail, we don't have any characters
    if (c->head == c->tail)
        return 0;  // quit with an error
 
    *data = c->buffer[c->tail];
    c->buffer[c->tail] = 0;  // clear the data (optional)
 
    unsigned int next = c->tail + 1;
    if(next >= c->maxLen)
        next = 0;
 
    c->tail = next;
 
    return 1;
}

// Return used space in buffer
int circBufUsedSpace(circBuf_t *c) {
	if (c->tail > c->head) {
		return (c->maxLen - c->tail) + c->head;
	} else {
		return c->head - c->tail;
	}
}
