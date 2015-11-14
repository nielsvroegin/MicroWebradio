// Circ buf based on http://embedjournal.com/implementing-circular-buffer-embedded-c/

#include <stdbool.h>

#define CIRCBUF_DEF(x,y) char x##_space[y]; circBuf_t x = { x##_space,0,0,y}

typedef volatile struct
{
    char * const buffer;
    unsigned int head;
    unsigned int tail;
    const unsigned int maxLen;
} circBuf_t;

// Push byte to buffer
bool circBufPush(circBuf_t *c, char data);

// Read byte from buffer
bool circBufPop(circBuf_t *c, char *data);

// Return used space in buffer
int circBufUsedSpace(circBuf_t *c);
