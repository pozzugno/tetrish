#ifndef PIECE_H
#define PIECE_H

#define PIECES_NUM           7

struct Piece {
    unsigned int xl;
    unsigned int yl;
    unsigned char rot_num;
    char data[];
};

extern const struct Piece *pieces[PIECES_NUM];

unsigned int piece_xl(const struct Piece *p, int rot);
unsigned int piece_yl(const struct Piece *p, int rot);
char piece_char(const struct Piece *p, int rot, unsigned int x, unsigned int y);
unsigned int piece_rotate(const struct Piece *p, int rot);

#endif
