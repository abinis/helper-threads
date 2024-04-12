#include "edge_color_bitmask.h"

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

unsigned int bitmask_get_cell_size()
{
    unsigned int ret;

    //size of bitmask 'cell' in bits, e.g. 4bytes*8bits each = 32 bits
    //TODO: figure out type instead of providing it (?)
    ret = sizeof(unsigned char)*8;

    return ret;
}

bitmask_t * edge_color_bitmask_init(unsigned int nedges)
{
    bitmask_t *bitm = (bitmask_t*)malloc(sizeof(bitmask_t));
    if (!bitm) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    unsigned int cell_size = bitmask_get_cell_size();
    unsigned int num_cells;
    unsigned int bits = BITS_PER_EDGE*nedges;
    num_cells = ( bits%cell_size ) ? bits/cell_size + 1 : bits/cell_size;

    unsigned char * cells = calloc(num_cells, sizeof(unsigned char));
    if (!cells) {
        fprintf(stderr, "%s: Allocation error\n", __FUNCTION__);
        exit(EXIT_FAILURE);
    }

    bitm->num_cells = num_cells;
    bitm->cell_size = cell_size;
    bitm->length = nedges;
    bitm->bitmask = cells;

    return bitm;
}

int edge_color_bitmask_set_color(bitmask_t *bmp, unsigned int pos)
{
    assert(bmp);

    if ( !(pos >= 0 && pos < bmp->length ) ) {
        fprintf(stderr, "function edge_color_bitmask_set_color:"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    unsigned int cellno = pos / bmp->cell_size;
    unsigned int offset = pos % bmp->cell_size;
    offset = bmp->cell_size - offset;

    //TODO bitwise set bit in position offset!
    unsigned char bit = 1U << (offset-1);
    bmp->bitmask[cellno] |= bit;

    return 1;
}

int edge_color_bitmask_set_color_atomic(bitmask_t *bmp, unsigned int pos)
{
    assert(bmp);

    if ( !(pos >= 0 && pos < bmp->length ) ) {
        fprintf(stderr, "function edge_color_bitmask_set_color_atomic:"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    unsigned int cellno = pos / bmp->cell_size;
    unsigned int offset = pos % bmp->cell_size;
    offset = bmp->cell_size - offset;

    //TODO bitwise set bit in position offset!
    unsigned char bit = 1U << (offset-1);
    //bmp->bitmask[cellno] |= bit;
    int ret = __sync_or_and_fetch ( &(bmp->bitmask[cellno]), bit );

    return ret;
}

int edge_color_bitmask_clear_color(bitmask_t *bmp, unsigned int pos)
{
    assert(bmp);

    if ( !(pos >= 0 && pos < bmp->length ) ) {
        fprintf(stderr, "function edge_color_bitmask_clear_color:"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    unsigned int cellno = pos / bmp->cell_size;
    unsigned int offset = pos % bmp->cell_size;
    offset = bmp->cell_size - offset;

    //TODO bitwise clear bit in position offset!
    unsigned char bit = 1U << (offset-1);
    bmp->bitmask[cellno] &= ~bit;

    return 1;
}

int edge_color_bitmask_get_color(bitmask_t *bmp, unsigned int pos)
{
    assert(bmp);

    if ( !(pos >= 0 && pos < bmp->length ) ) {
        fprintf(stderr, "function edge_color_bitmask_get_color:"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    unsigned int index = BITS_PER_EDGE*pos;

    unsigned int cellno = index / bmp->cell_size;
    unsigned int offset = index % bmp->cell_size;
    // position of bit within cell, starting from LSB (index 0)
    offset = bmp->cell_size - offset - 1;

    //TODO bitwise read bit in position offset!
    unsigned char cell_val = bmp->bitmask[cellno];
    unsigned char color = (cell_val >> (offset-1)) & 3U;

    return color;
}

int edge_color_bitmask_msf_edge(bitmask_t *bmp, unsigned int pos)
{
    assert(bmp);

    if ( !(pos >= 0 && pos < bmp->length ) ) {
        fprintf(stderr, "function edge_color_bitmask_msf_edge"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    unsigned int index = BITS_PER_EDGE*pos;

    unsigned int cellno = index / bmp->cell_size;
    unsigned int offset = index % bmp->cell_size;
    // position of bit within cell, starting from LSB (index 0)
    offset = bmp->cell_size - offset - 1;

    //TODO bitwise set bit in position offset!
    // shift left by offset-1 because we're setting the 2nd bit, 00->01
    unsigned char bit = 1U << (offset-1);
    bmp->bitmask[cellno] |= bit;
    //int ret = __sync_or_and_fetch ( &(bmp->bitmask[cellno]), bit );

    return 1;
}

int edge_color_bitmask_cycle_main(bitmask_t *bmp, unsigned int pos)
{
    assert(bmp);

    if ( !(pos >= 0 && pos < bmp->length ) ) {
        fprintf(stderr, "function edge_color_bitmask_msf_edge"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    unsigned int index = BITS_PER_EDGE*pos;

    unsigned int cellno = index / bmp->cell_size;
    unsigned int offset = index % bmp->cell_size;
    // position of bit within cell, starting from LSB (index 0)
    offset = bmp->cell_size - offset - 1;

    //TODO bitwise set bit in position offset!
    // shift left by offset because we're setting the 1st bit, 00->10
    unsigned char bit = 1U << offset;
    bmp->bitmask[cellno] |= bit;
    //int ret = __sync_or_and_fetch ( &(bmp->bitmask[cellno]), bit );

    return 1;
}

int edge_color_bitmask_cycle_helper(bitmask_t *bmp, unsigned int pos)
{
    assert(bmp);

    if ( !(pos >= 0 && pos < bmp->length ) ) {
        fprintf(stderr, "function edge_color_bitmask_msf_edge"
                        " out of bounds position provided (%u);"
                        " doing nothing\n", pos);
        return 0;
    }

    unsigned int index = BITS_PER_EDGE*pos;

    unsigned int cellno = index / bmp->cell_size;
    unsigned int offset = index % bmp->cell_size;
    // position of bit within cell, starting from LSB (index 0)
    offset = bmp->cell_size - offset - 1;

    //TODO bitwise set bit in position offset!
    // this time we're setting both bits, 00->11
    unsigned char bit = 3U << (offset-1);
    bmp->bitmask[cellno] |= bit;
    //int ret = __sync_or_and_fetch ( &(bmp->bitmask[cellno]), bit );

    return 1;
}

void edge_color_bitmask_print(bitmask_t *bmp)
{
    int i;

    for ( i = 0; i < bmp->length; i++ ) {
        printf("%d", edge_color_bitmask_get_color(bmp, i));
        if ( (BITS_PER_EDGE*(i+1)) % bmp->cell_size == 0 ) 
            printf(" ");
    }
    //last cell, remaining bits, if any, should be 0-padded :)
    if ( BITS_PER_EDGE*i < bmp->num_cells*bmp->cell_size ) { 
        printf("(");
        unsigned char last_cell_val = bmp->bitmask[bmp->num_cells-1];
        unsigned int bits_to_check = bmp->cell_size - (BITS_PER_EDGE*i % bmp->cell_size);
        while ( bits_to_check > 0 ) {
            printf("%d", (last_cell_val >> (bits_to_check-BITS_PER_EDGE)) & 3U);
            bits_to_check -= BITS_PER_EDGE;
        }
        //while ( i < bmp->num_cells*bmp->cell_size ) {
        //    unsigned int shift_right = bmp->cell_size - (i % bmp->cell_size);
        //    printf("%d", (last_cell_val >> shift_right-1) & 1U);
        //    i++;
        //}
        printf(")");
    }
        
    printf("\n");
}

void edge_color_bitmask_destroy(bitmask_t *bmp)
{
    assert(bmp);

    free(bmp->bitmask);
    free(bmp);
}
