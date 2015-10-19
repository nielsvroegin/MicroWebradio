// Circ buf based on http://embedjournal.com/implementing-circular-buffer-embedded-c/

#define CIRCBUF_DEF(x,y) unsigned char x##_space[y]; circBuf_t x = { x##_space,0,0,y}

typedef volatile struct
{
    unsigned char * const buffer;
    unsigned char head;
    unsigned char tail;
    const unsigned char maxLen;
} circBuf_t;

// Push byte to buffer
inline bit circBufPush(circBuf_t *c, unsigned char data);

// Read byte from buffer
inline bit circBufPop(circBuf_t *c, unsigned char *data);

