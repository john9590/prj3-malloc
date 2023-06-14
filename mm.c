/*
 * mm-naive.c - The fastest, least memory-efficient malloc package.
 * 
 * In this naive approach, a block is allocated by simply incrementing
 * the brk pointer.  A block is pure payload. There are no headers or
 * footers.  Blocks are never coalesced or reused. Realloc is
 * implemented directly using mm_malloc and mm_free.
 *
 * NOTE TO STUDENTS: Replace this header comment with your own header
 * comment that gives a high level description of your solution.
 */
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <string.h>

#include "mm.h"
#include "memlib.h"

/*********************************************************
 * NOTE TO STUDENTS: Before you do anything else, please
 * provide your information in the following struct.
 ********************************************************/
team_t team = {
    /* Your student ID */
    "20191669",
    /* Your full name*/
    "HWANG SUNG MIN",
    /* Your email address */
    "john9590@sogang.ac.kr",
};

#define NEXT_FITx

/* $begin mallocmacros */
/* Basic constants and macros */
#define WSIZE       4       /* Word and header/footer size (bytes) */ //line:vm:mm:beginconst
#define DSIZE       8       /* Double word size (bytes) */
#define CHUNKSIZE  (1<<12)  /* Extend heap by this amount (bytes) */  //line:vm:mm:endconst 

#define MAX(x, y) ((x) > (y)? (x) : (y))  

/* Pack a size and allocated bit into a word */
#define PACK(size, alloc)  ((size) | (alloc)) //line:vm:mm:pack

/* Read and write a word at address p */
#define GET(p)       (*(unsigned int *)(p))            //line:vm:mm:get
#define PUT(p, val)  (*(unsigned int *)(p) = (val))    //line:vm:mm:put

/* Read the size and allocated fields from address p */
#define GET_SIZE(p)  (GET(p) & ~0x7)                   //line:vm:mm:getsize
#define GET_ALLOC(p) (GET(p) & 0x1)                    //line:vm:mm:getalloc

/* Given block ptr bp, compute address of its header and footer */
#define HDRP(bp)       ((char *)(bp) - WSIZE)                      //line:vm:mm:hdrp
#define FTRP(bp)       ((char *)(bp) + GET_SIZE(HDRP(bp)) - DSIZE) //line:vm:mm:ftrp
#define PVRP(bp)       (char *)(bp)
#define NTRP(bp)       ((char *)(bp) + WSIZE)

/* Given block ptr bp, compute address of next and previous blocks */
#define NEXT_BLKP(bp)  ((char *)(bp) + GET_SIZE(((char *)(bp) - WSIZE))) //line:vm:mm:nextblkp
#define PREV_BLKP(bp)  ((char *)(bp) - GET_SIZE(((char *)(bp) - DSIZE))) //line:vm:mm:prevblkp

#define NEXT_BLNK(bp)  GET((char *)(NTRP(bp)))
#define PREV_BLNK(bp)  GET((char *)(PVRP(bp)))

//#define NEXT_BLNK(bp)  GET((char *)(bp))
/* $end mallocmacros */
/*
 * If NEXT_FIT defined use next fit search, else use first-fit search 
 */

/* Global variables */
static char *heap_listp = 0;  /* Pointer to first block */  

static char *seg_listp = 0;
int ft_input = 1;
static void *extend_heap(size_t words);
static void *place(void *bp, size_t asize);
static void *find_fit(size_t asize);
static void *coalesce(void *bp);
static void printblock(void *bp); 
static void checkheap(int verbose);
static void checkblock(void *bp);

void *size_find(int size) {
    static char* result;
	int i = 0;
	for (i = 32; i > 0; i--) {
		if (size & (1 << i)) break;
	}
	result = seg_listp + (i * WSIZE);
	return result;
}

void link_add(void * bp) {
    //printf("%p %d\n",bp,GET_SIZE(HDRP(bp+8)));
    void *seg = size_find(GET_SIZE(HDRP(bp)));
    if(PREV_BLNK(seg)) NEXT_BLNK(PREV_BLNK(seg)) = bp;
    PREV_BLNK(bp) = PREV_BLNK(seg);
    NEXT_BLNK(bp) = seg;
    PREV_BLNK(seg) = bp;
    //printf("add : %p %p\n",bp,PREV_BLNK(bp));
    //printf("%p %d\n",bp+8,GET_SIZE(HDRP(bp+8)));
    //printf("%p %p %p %p\n",NEXT_BLNK(PREV_BLNK(seg)),PREV_BLNK(bp),NEXT_BLNK(bp),PREV_BLNK(seg));
}

void link_remove(void *bp) {
    //printf("remove : %p %p\n",bp,GET(bp));
    if (PREV_BLNK(bp)) NEXT_BLNK(PREV_BLNK(bp)) = NEXT_BLNK(bp);
    if (NEXT_BLNK(bp)) PREV_BLNK(NEXT_BLNK(bp)) = PREV_BLNK(bp);
}

int mm_init(void)
{
    if ((seg_listp = mem_sbrk(32*WSIZE)) == (void *)-1) return -1;
    for (int i=0;i<32;i++) PUT(seg_listp+(i*WSIZE),NULL); 
    if ((heap_listp = mem_sbrk(4*WSIZE)) == (void *)-1) //line:vm:mm:begininit
        return -1;
    PUT(heap_listp, 0);                          /* Alignment padding */
    PUT(heap_listp + (1*WSIZE), PACK(DSIZE, 1)); /* Prologue header */ 
    //PUT(heap_listp + (2*WSIZE), NULL); //initialize to start point
    //PUT(heap_listp + (3*WSIZE), NULL); //next is null
    PUT(heap_listp + (2*WSIZE), PACK(DSIZE, 1)); /* Prologue footer */ 
    PUT(heap_listp + (3*WSIZE), PACK(0, 1));     /* Epilogue header */
    heap_listp += 2*WSIZE;
    /* $end mminit */
//#ifdef NEXT_FIT
    //rover = heap_listp;
//#endif
    /* $begin mminit */

    /* Extend the empty heap with a free block of CHUNKSIZE bytes */
    //if (extend_heap(CHUNKSIZE/WSIZE) == NULL) 
    //    return -1;
    return 0;
}

/* 
 * mm_malloc - Allocate a block by incrementing the brk pointer.
 *     Always allocate a block whose size is a multiple of the alignment.
 */
void *mm_malloc(size_t size)
{
    size_t asize;      /* Adjusted block size */
    size_t extendsize; /* Amount to extend heap if no fit */
    char *bp, *ft;      
    //printf("%d\n",size);
    /* $end mmmalloc */
    ft_input = (ft_input+1) % 2;
    //printf("malloc : %d %d\n",size,ft_input);
    if (heap_listp == 0){
        mm_init();
    }
    /* $begin mmmalloc */
    /* Ignore spurious requests */
    if (size == 0)
        return NULL;

    /* Adjust block size to include overhead and alignment reqs. */
    if (size <= DSIZE)                                          //line:vm:mm:sizeadjust1
        asize = 2*DSIZE;                                        //line:vm:mm:sizeadjust2
    else
        asize = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE); //line:vm:mm:sizeadjust3
    /* Search the free list for a fit */
    int bpsize = 0;
    if ((bp = find_fit(asize)) != NULL) {  //line:vm:mm:findfitcall
        /*if (ft_input) {
            bpsize = GET_SIZE(HDRP(bp)) - asize;
            if (bpsize >= 2*DSIZE) {
                //printf("cur bp : %p %d\n",bp,GET_SIZE(HDRP(bp)));
                //place(bp,bpsize);
                ft = FTRP(bp) - asize + DSIZE;
                PUT(HDRP(bp),PACK(bpsize,1));
                PUT(FTRP(bp),PACK(bpsize,1));
                //mm_free(bp);
                link_add(ft);
                place(ft,asize);
                bp = ft;
            }
            else {
                place(bp,asize);
                /*link_remove(bp);
                PUT(HDRP(bp),PACK(asize,1));
                PUT(FTRP(bp),PACK(asize,1));
            }
            return bp;
        }*/
        return place(bp, asize);                  //line:vm:mm:findfitplace
        //printf("malloc : %p %d\n",bp,size);
    }
    /* No fit found. Get more memory and place the block */
    extendsize = MAX(asize,CHUNKSIZE);                 //line:vm:mm:growheap1
    if ((bp = extend_heap(extendsize/WSIZE)) == NULL)  
        return NULL;                                  //line:vm:mm:growheap2
    /*if (ft_input) {
        bpsize = GET_SIZE(HDRP(bp)) - asize;
        if (bpsize >= 2*DSIZE) {
            //printf("cur bp : %p %d %d\n",PREV_BLNK(bp),bpsize,asize);
            //place(bp,bpsize);
            ft = FTRP(bp) - asize + DSIZE;
            PUT(HDRP(bp),PACK(bpsize,1));
            PUT(FTRP(bp),PACK(bpsize,1));
            //mm_free(bp);
            link_add(ft);
            place(ft,asize);
            bp = ft;
            //printf("bp: %p\n",PREV_BLNK(bp));
        }
        else {
            place(bp,asize);
            /*link_remove(bp);
            PUT(HDRP(bp),PACK(asize,1));
            PUT(FTRP(bp),PACK(asize,1));
        }
        return bp;
    }*/
    return place(bp, asize);                                 //line:vm:mm:growheap3
    //printf("malloc : %p %d\n",bp,size);
}

/*
 * mm_free - Freeing a block does nothing.
 */
void mm_free(void *bp)
{
    //printf("free : %p\n",bp);
    /* $end mmfree */
    if (bp == 0) 
        return;

    /* $begin mmfree */
    size_t size = GET_SIZE(HDRP(bp));
    /* $end mmfree */
    if (heap_listp == 0){
        mm_init();
    }
    //printf("size : %d\n",size);
    /* $begin mmfree */
    PUT(HDRP(bp), PACK(size, 0));
    PUT(FTRP(bp), PACK(size, 0));
    coalesce(bp);
}

/* $begin mmfree */
static void *coalesce(void *bp) 
{
    size_t prev_alloc = GET_ALLOC(FTRP(PREV_BLKP(bp)));
    size_t next_alloc = GET_ALLOC(HDRP(NEXT_BLKP(bp)));
    size_t size = GET_SIZE(HDRP(bp));
    //printf("coal %p %d %d %d %d\n",bp,GET_SIZE(FTRP(PREV_BLKP(bp))),GET_SIZE(HDRP(NEXT_BLKP(bp))),prev_alloc,next_alloc);
    if (prev_alloc && !next_alloc) {      /* Case 2 */
        link_remove(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(NEXT_BLKP(bp)));
        PUT(HDRP(bp), PACK(size, 0));
        PUT(FTRP(bp), PACK(size, 0));
        //printf("10 %p %p %p\n",bp,NEXT_BLNK(bp),NEXT_BLNK(NEXT_BLNK(bp)));
    }

    else if (!prev_alloc && next_alloc) {      /* Case 3 */
        link_remove(PREV_BLKP(bp));
        //printf("%p\n",PREV_BLKP(bp));
        
        size += GET_SIZE(HDRP(PREV_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(PREV_BLKP(bp)), PACK(size,0));
        //printf("cur %p next %p\n",bp,PREV_BLKP(bp));
        bp = PREV_BLKP(bp);

        //printf("prev %p %p\n",bp,NEXT_BLNK(bp));
        //printf("prve unalloc : %p\n",bp);
        //printf("%p %p\n",PREV_BLNK(bp),NEXT_BLNK(bp));
    }

    else if (!prev_alloc && !next_alloc){                                     /* Case 4 */
        link_remove(PREV_BLKP(bp));
        link_remove(NEXT_BLKP(bp));
        size += GET_SIZE(HDRP(PREV_BLKP(bp))) + 
            GET_SIZE(FTRP(NEXT_BLKP(bp)));
        PUT(HDRP(PREV_BLKP(bp)), PACK(size, 0));
        PUT(FTRP(PREV_BLKP(bp)), PACK(size, 0));
        bp = PREV_BLKP(bp);
    }
    link_add(bp);
    /* $end mmfree */
    //printf("bp next %p %p\n",bp,NEXT_BLNK(bp));
    /* $begin mmfree */
    return bp;
}
/* $end mmfree */


/*
 * mm_realloc - Implemented simply in terms of mm_malloc and mm_free
 */
void *mm_realloc(void *ptr, size_t size)
{
    ft_input = (ft_input+1) % 2;
    size_t oldsize = GET_SIZE(HDRP(ptr));
    //printf("ptr : %p size : %d oldsize : %d\n",ptr,size,oldsize);
    if (size <= DSIZE)                                          //line:vm:mm:sizeadjust1
        size = 2*DSIZE;                                        //line:vm:mm:sizeadjust2
    else
        size = DSIZE * ((size + (DSIZE) + (DSIZE-1)) / DSIZE);
    void *newptr = NEXT_BLKP(ptr);
    void *prevp = PREV_BLKP(ptr);
    //void *temp;
    size_t newsize = oldsize + GET_SIZE(HDRP(newptr));
    size_t prevsize = oldsize + GET_SIZE(HDRP(prevp));
    size_t casesize = oldsize + GET_SIZE(HDRP(newptr)) + GET_SIZE(HDRP(prevp));
    /* If size == 0 then this is just free, and we return NULL. */
    if(size == 0) {
        mm_free(ptr);
        return 0;
    }
    /* If oldptr is NULL, then this is just malloc. */
    if(ptr == NULL) {
        return mm_malloc(size);
    }
    if (size<=oldsize) {
        if (size>2*DSIZE && oldsize - size > 2*DSIZE) {
            PUT(HDRP(ptr), PACK(size, 1));
            PUT(FTRP(ptr), PACK(size, 1));
            newptr = NEXT_BLKP(ptr);
            PUT(HDRP(newptr), PACK(oldsize - size, 1));
            PUT(FTRP(newptr), PACK(oldsize - size, 1));
            mm_free(newptr);
            //printf("ptr : %p\n",ptr);
            return ptr;
        }
        oldsize = size;
    }
    else{
        if (!GET_ALLOC(HDRP(newptr))&&!GET_ALLOC(HDRP(prevp))&& casesize - size > 2*DSIZE){
            link_remove(newptr);
            link_remove(prevp);
            ptr = memmove(prevp,ptr,oldsize);
            PUT(HDRP(prevp), PACK(size,1));
            PUT(FTRP(prevp), PACK(size,1));
            newptr = NEXT_BLKP(prevp);
            PUT(HDRP(newptr), PACK(casesize-size,0));
            PUT(FTRP(newptr), PACK(casesize-size,0));
            link_add(newptr);
            return ptr;
        }
        if (!GET_ALLOC(HDRP(newptr)) && newsize>size + 2*DSIZE){
            link_remove(newptr);
            PUT(HDRP(ptr), PACK(size,1));
            PUT(FTRP(ptr), PACK(size,1));
            newptr = NEXT_BLKP(ptr);
            PUT(HDRP(newptr), PACK(newsize-size,1));
            PUT(FTRP(newptr), PACK(newsize-size,1));
            mm_free(newptr);
            //printf("%p %d %d\n",ptr,newsize,size);
            //printf("ptr : %p\n",ptr);
            return ptr;
        }
        if (!GET_ALLOC(HDRP(prevp)) && prevsize>size + 2*DSIZE){
            link_remove(prevp);
            ptr = memmove(prevp, ptr, oldsize);
            PUT(HDRP(prevp), PACK(size,1));
            PUT(FTRP(prevp), PACK(size,1));
            newptr = NEXT_BLKP(prevp);
            //printf("\n");
            PUT(HDRP(newptr), PACK(prevsize-size,0));
            PUT(FTRP(newptr), PACK(prevsize-size,0));
            //printf("%p %p %p\n",HDRP(prevp),HDRP(newptr),FTRP(newptr));
            link_add(newptr);
            //for(int i=0;i<=prevsize;i+=4) {
                //printf("%p %d ",((char*)ptr)+i,GET_SIZE(HDRP((char*)ptr+i)));
            //}
            //printf("\n");
            //printf("ptr : %p\n",ptr);
            return ptr;
        }
    }
    //ft_input = (ft_input+1) % 2;
    newptr = mm_malloc(size);
    /* If realloc() fails the original block is left untouched  */
    if(!newptr) {
        return 0;
    }
    /* Copy the old data. */
    //if(size < oldsize) oldsize = size;
    memmove(newptr, ptr, oldsize);
    mm_free(ptr);
    /* Free the old block. */
    //printf("ptr : %p\n",newptr);
    return newptr;
}


/* 
 * mm_checkheap - Check the heap for correctness
 */
void mm_checkheap(int verbose)  
{ 
    checkheap(verbose);
}

/* 
 * The remaining routines are internal helper routines 
 */

/* 
 * extend_heap - Extend heap with free block and return its block pointer
 */
/* $begin mmextendheap */
static void *extend_heap(size_t words) 
{
    char *bp;
    size_t size;
    /* Allocate an even number of words to maintain alignment */
    size = (words % 2) ? (words+1) * WSIZE : words * WSIZE; //line:vm:mm:beginextend
    //printf("extend : %d\n",size);
    if ((long)(bp = mem_sbrk(size)) == -1) 
        return NULL;                //line:vm:mm:endextend
    /* Initialize free block header/footer and the epilogue header */
    PUT(HDRP(bp), PACK(size, 0));         /* Free block header */   //line:vm:mm:freeblockhdr
    //PUT(bp, NULL); //prev null
    //PUT(bp+WSIZE, heap_listp); //next 이전꺼
    PUT(FTRP(bp), PACK(size, 0));         /* Free block footer */   //line:vm:mm:freeblockftr
    PUT(HDRP(NEXT_BLKP(bp)), PACK(0, 1)); /* New epilogue header */ //line:vm:mm:newepihdr
    //printf("extend %d %p\n",size,bp);
    /* Coalesce if the previous block was free */
    return coalesce(bp);                                          //line:vm:mm:returnblock
}
/* $end mmextendheap */

/* 
 * place - Place block of asize bytes at start of free block bp 
 *         and split if remainder would be at least minimum block size
 */
/* $begin mmplace */
/* $begin mmplace-proto */
static void *place(void *bp, size_t asize)
/* $end mmplace-proto */
{
    size_t csize = GET_SIZE(HDRP(bp));   
    int next,prev;
    //printf("place start %p %d\n",bp,asize);
    link_remove(bp);
    //else printf("%p\n",bp);
    if ((csize - asize) >= (2*DSIZE)) {
        if (ft_input) {
            link_add(bp);
            PUT(HDRP(bp),PACK(csize-asize,0));
            PUT(FTRP(bp),PACK(csize-asize,0));
            bp = NEXT_BLKP(bp);
            PUT(HDRP(bp),PACK(asize,1));
            PUT(FTRP(bp),PACK(asize,1));
        }
        else {
            PUT(HDRP(bp), PACK(asize, 1));
            PUT(FTRP(bp), PACK(asize, 1));
            //printf("prev bp next : %p %p\n",bp, NEXT_BLKP(bp));
            PUT(HDRP(NEXT_BLKP(bp)), PACK(csize-asize, 0));
            PUT(FTRP(NEXT_BLKP(bp)), PACK(csize-asize, 0));
            link_add(NEXT_BLKP(bp));
        //printf("heap : %p %p\n",heap_listp,NEXT_BLNK(heap_listp));
        //printf("%p\n",bp);
        //printf("%p\n",NEXT_BLNK(bp));
        }
    }
    else { 
        PUT(HDRP(bp), PACK(csize, 1));
        PUT(FTRP(bp), PACK(csize, 1));
    }
    return bp;
    //printf("place %p %p\n",bp,NEXT_BLNK(bp));
}
/* $end mmplace */

/* 
 * find_fit - Find a fit for a block with asize bytes 
 */
/* $begin mmfirstfit */
/* $begin mmfirstfit-proto */
static void *find_fit(size_t asize)
/* $end mmfirstfit-proto */
{
    /* $end mmfirstfit */


    /* $begin mmfirstfit */
    /* First-fit search */
    int i=0;
    for (i=32;i>0;i--) {
        if((asize & (1<<i))>0) break;
    }
    void *bp,*best = NULL;  
    int size;
    for (int j=i;j<=32;j++){
        bp = seg_listp + j*WSIZE;
        if(PREV_BLNK(bp)){
            //printf("%p\n",PREV_BLNK(bp));
            size = 1<<(j+1);
            for (void *cur = PREV_BLNK(bp); cur!=NULL; cur = PREV_BLNK(cur)){
                //printf("%p\n",cur);
                if (asize<=GET_SIZE(HDRP(cur))&&GET_SIZE(HDRP(cur))-asize<size){
                    best = cur;
                    size = GET_SIZE(HDRP(cur)) - asize;
                }
            }
            if (best!=NULL) return best;
        }
    }
    return NULL; /* No fit */
}
/* $end mmfirstfit */

static void printblock(void *bp) 
{
    size_t hsize, halloc, fsize, falloc;

    checkheap(0);
    hsize = GET_SIZE(HDRP(bp));
    halloc = GET_ALLOC(HDRP(bp));  
    fsize = GET_SIZE(FTRP(bp));
    falloc = GET_ALLOC(FTRP(bp));  

    if (hsize == 0) {
        printf("%p: EOL\n", bp);
        return;
    }

    printf("%p: header: [%ld:%c] footer: [%ld:%c]\n", bp, 
           hsize, (halloc ? 'a' : 'f'), 
           fsize, (falloc ? 'a' : 'f')); 
}

static void checkblock(void *bp) 
{
    if ((size_t)bp % 8)
        printf("Error: %p is not doubleword aligned\n", bp);
    if (GET(HDRP(bp)) != GET(FTRP(bp)))
        printf("Error: header does not match footer\n");
}

/* 
 * checkheap - Minimal check of the heap for consistency 
 */
void checkheap(int verbose) 
{
    char *bp = heap_listp;

    if (verbose)
        printf("Heap (%p):\n", heap_listp);

    if ((GET_SIZE(HDRP(heap_listp)) != 2*DSIZE) || !GET_ALLOC(HDRP(heap_listp)))
        printf("Bad prologue header\n");
    checkblock(heap_listp);

    for (bp = heap_listp; GET_SIZE(HDRP(bp)) > 0; bp = NEXT_BLKP(bp)) {
        if (verbose) 
            printblock(bp);
        checkblock(bp);
    }

    if (verbose)
        printblock(bp);
    if ((GET_SIZE(HDRP(bp)) != 0) || !(GET_ALLOC(HDRP(bp))))
        printf("Bad epilogue header\n");
}
