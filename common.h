#ifndef common.h
#define common.h
struct node
{
    int level;
    uint64_t elementMortonIndex;
    ////////////////////////////////
    struct node *  children[8];
    uint64_t offsets[8];
};

#endif