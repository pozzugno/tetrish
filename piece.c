#include "piece.h"

static const struct Piece pc_square = {
    .xl = 2,
    .yl = 2,
    .rot_num = 1,
    .data = {
        'O', 'O',
        'O', 'O',
    }
};

static const struct Piece pc_s1 = {
    .xl = 3,
    .yl = 3,
    .rot_num = 2,
    .data = {
        'X', 'X', '.',
        '.', 'X', 'X',
        '.', '.', '.',
    }
};


static const struct Piece pc_s2 = {
    .xl = 3,
    .yl = 3,
    .rot_num = 2,
    .data = {
        '.', 'Y', 'Y',
        'Y', 'Y', '.',
        '.', '.', '.',
    }
};

static const struct Piece pc_ti = {
    .xl = 3,
    .yl = 3,
    .rot_num = 4,
    .data = {
        '.', '.', '.',
        'T', 'T', 'T',
        '.', 'T', '.',
    }
};

static const struct Piece pc_l1 = {
    .xl = 3,
    .yl = 3,
    .rot_num = 4,
    .data = {
        '.', '.', '.',
        'P', 'P', 'P',
        'P', '.', '.',
    }
};

static const struct Piece pc_l2 = {
    .xl = 3,
    .yl = 3,
    .rot_num = 4,
    .data = {
        '.', '.', '.',
        'G', 'G', 'G',
        '.', '.', 'G',
    }
};

static const struct Piece pc_line = {
    .xl = 4,
    .yl = 4,
    .rot_num = 2,
    .data = {
        '.', '.', '.', '.',
        'H', 'H', 'H', 'H',
        '.', '.', '.', '.',
        '.', '.', '.', '.',
    }
};

const struct Piece *pieces[] = {
    &pc_square,
    &pc_s1,
    &pc_s2,
    &pc_line,
    &pc_ti,
    &pc_l1,
    &pc_l2,
};

unsigned int piece_xl(const struct Piece *p, int rot)
{
    if ((rot == 0) || (rot == 2)) {
        return p->xl;
    } else {
        return p->yl;
    }
}

unsigned int piece_yl(const struct Piece *p, int rot)
{
    if ((rot == 0) || (rot == 2)) {
        return p->yl;
    } else {
        return p->xl;
    }
}

char piece_char(const struct Piece *p, int rot, unsigned int x, unsigned int y)
{
    if ((rot == 0) || (rot == 2)) {
        // 0° and 180° rotations
        if((x < p->xl) && (y < p->yl)) {
            if (rot == 2) {
                x = p->xl - x - 1;
                y = p->yl - y - 1;
            }
            return p->data[x + y * p->xl];
        } else {
            return ' ';
        }
    } else {
        // 90° and 270° rotations
        unsigned int xp;
        unsigned int yp;
        if ((x < p->yl) && (y < p->xl)) {
            if (rot == 1) {
                xp = p->xl - y - 1;
                yp = x;
            } else {
                xp = y;
                yp = p->yl - x - 1;
            }
            return p->data[xp + yp * p->xl];
        } else {
            return ' ';
        }
    }
}

unsigned int
piece_rotate(const struct Piece *p, int rot)
{
    unsigned int ret = rot + 1;
    if (ret >= p->rot_num) {
        ret = 0;
    }
    return ret;
}

