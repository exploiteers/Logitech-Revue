/*
 * H.264/MPEG-4 Part 10 (Base profile) encoder.
 *
 * DSP functions
 *
 * Copyright (c) 2006-2007 Expertisecentrum Digitale Media, UHasselt
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavcodec/h264dspenc.c
 * H.264 encoder related DSP utils
 *
 */

#include "dsputil.h"
#include "mpegvideo.h"
#include "h264data.h"
#include "h264enc.h"

/* See JVT-B038 p. 10 */

#define  H264_DCT_PART1(X) \
         M0 = block[0][X]+block[3][X]; \
         M3 = block[0][X]-block[3][X]; \
         M1 = block[1][X]+block[2][X]; \
         M2 = block[1][X]-block[2][X]; \
         pieces[0][X] = M0 + M1; \
         pieces[2][X] = M0 - M1; \
         pieces[1][X] = M2 + (M3 << 1); \
         pieces[3][X] = M3 - (M2 << 1);

#define  H264_DCT_PART2(X) \
         M0 = pieces[X][0]+pieces[X][3]; \
         M3 = pieces[X][0]-pieces[X][3]; \
         M1 = pieces[X][1]+pieces[X][2]; \
         M2 = pieces[X][1]-pieces[X][2]; \
         block[0][X] = M0 + M1; \
         block[2][X] = M0 - M1; \
         block[1][X] = M2 + (M3 << 1); \
         block[3][X] = M3 - (M2 << 1);

/**
 * Transform the provided matrix using the H.264 modified DCT.
 * @note
 * we'll always work with transposed input blocks, to avoid having to make a
 * distinction between C and mmx implementations.
 *
 * @param block transposed input block
 */
static void h264_dct_c(DCTELEM block[4][4])
{
    DCTELEM pieces[4][4];
    DCTELEM M0, M1, M2, M3;

    H264_DCT_PART1(0);
    H264_DCT_PART1(1);
    H264_DCT_PART1(2);
    H264_DCT_PART1(3);
    H264_DCT_PART2(0);
    H264_DCT_PART2(1);
    H264_DCT_PART2(2);
    H264_DCT_PART2(3);
}

inline void ff_h264_hadamard_mult_2x2(DCTELEM Y[2][2])
{
    DCTELEM pieces[2][2];

    pieces[0][0] = Y[0][0]+Y[0][1];
    pieces[0][1] = Y[1][0]+Y[1][1];
    pieces[1][0] = Y[0][0]-Y[0][1];
    pieces[1][1] = Y[1][0]-Y[1][1];
    Y[0][0] = pieces[0][0]+pieces[0][1];
    Y[0][1] = pieces[1][0]+pieces[1][1];
    Y[1][0] = pieces[0][0]-pieces[0][1];
    Y[1][1] = pieces[1][0]-pieces[1][1];
}

static void h264_hadamard_mult_4x4_c(DCTELEM Y[4][4])
{
    DCTELEM pieces[4][4];

    pieces[0][0] = Y[0][0]+Y[0][1]+Y[0][2]+Y[0][3];
    pieces[0][1] = Y[1][0]+Y[1][1]+Y[1][2]+Y[1][3];
    pieces[0][2] = Y[2][0]+Y[2][1]+Y[2][2]+Y[2][3];
    pieces[0][3] = Y[3][0]+Y[3][1]+Y[3][2]+Y[3][3];

    pieces[1][0] = Y[0][0]+Y[0][1]-Y[0][2]-Y[0][3];
    pieces[1][1] = Y[1][0]+Y[1][1]-Y[1][2]-Y[1][3];
    pieces[1][2] = Y[2][0]+Y[2][1]-Y[2][2]-Y[2][3];
    pieces[1][3] = Y[3][0]+Y[3][1]-Y[3][2]-Y[3][3];

    pieces[2][0] = Y[0][0]-Y[0][1]-Y[0][2]+Y[0][3];
    pieces[2][1] = Y[1][0]-Y[1][1]-Y[1][2]+Y[1][3];
    pieces[2][2] = Y[2][0]-Y[2][1]-Y[2][2]+Y[2][3];
    pieces[2][3] = Y[3][0]-Y[3][1]-Y[3][2]+Y[3][3];

    pieces[3][0] = Y[0][0]-Y[0][1]+Y[0][2]-Y[0][3];
    pieces[3][1] = Y[1][0]-Y[1][1]+Y[1][2]-Y[1][3];
    pieces[3][2] = Y[2][0]-Y[2][1]+Y[2][2]-Y[2][3];
    pieces[3][3] = Y[3][0]-Y[3][1]+Y[3][2]-Y[3][3];

    Y[0][0] = pieces[0][0]+pieces[0][1]+pieces[0][2]+pieces[0][3];
    Y[0][1] = pieces[1][0]+pieces[1][1]+pieces[1][2]+pieces[1][3];
    Y[0][2] = pieces[2][0]+pieces[2][1]+pieces[2][2]+pieces[2][3];
    Y[0][3] = pieces[3][0]+pieces[3][1]+pieces[3][2]+pieces[3][3];

    Y[1][0] = pieces[0][0]+pieces[0][1]-pieces[0][2]-pieces[0][3];
    Y[1][1] = pieces[1][0]+pieces[1][1]-pieces[1][2]-pieces[1][3];
    Y[1][2] = pieces[2][0]+pieces[2][1]-pieces[2][2]-pieces[2][3];
    Y[1][3] = pieces[3][0]+pieces[3][1]-pieces[3][2]-pieces[3][3];

    Y[2][0] = pieces[0][0]-pieces[0][1]-pieces[0][2]+pieces[0][3];
    Y[2][1] = pieces[1][0]-pieces[1][1]-pieces[1][2]+pieces[1][3];
    Y[2][2] = pieces[2][0]-pieces[2][1]-pieces[2][2]+pieces[2][3];
    Y[2][3] = pieces[3][0]-pieces[3][1]-pieces[3][2]+pieces[3][3];

    Y[3][0] = pieces[0][0]-pieces[0][1]+pieces[0][2]-pieces[0][3];
    Y[3][1] = pieces[1][0]-pieces[1][1]+pieces[1][2]-pieces[1][3];
    Y[3][2] = pieces[2][0]-pieces[2][1]+pieces[2][2]-pieces[2][3];
    Y[3][3] = pieces[3][0]-pieces[3][1]+pieces[3][2]-pieces[3][3];
}

/*
 *
 * Book p. 184, spec p. 182
 */
static inline void h264_deblock_line_luma(int p[4], int q[4], int QP, int bS)
{
    int delta0, delta0i, deltap1i, deltaq1i, deltap1, deltaq1;
    int pa0, pa1, pa2, qa0, qa1, qa2;
    int alpha, beta;

    if (bS == 0)
        return;

    alpha = alpha_table[QP];
    beta = beta_table[QP];

    if (!(
        (FFABS(p[0] - q[0]) < alpha) /* (1) */
        &&
        (FFABS(p[1] - p[0]) < beta) /* (2) */
        &&
        (FFABS(q[1] - q[0]) < beta) /* (3) */
        ))
        return;

    pa0 = p[0];
    pa1 = p[1];
    pa2 = p[2];
    qa0 = q[0];
    qa1 = q[1];
    qa2 = q[2];

    if (bS == 4)
    {
        int aP = FFABS(p[2] - p[0]);
        int aQ = FFABS(q[2] - q[0]);

        if (aP < beta && FFABS(p[0] - q[0]) < ((alpha>>2) + 2))
        {
            // Luminance filtering
            pa0 = (p[2] + 2*p[1] + 2*p[0] + 2*q[0] + q[1] + 4) >> 3; /* (20) */
            pa1 = (p[2] + p[1] + p[0] + q[0] + 2) >> 2; /* (21) */
            pa2 = (2*p[3] + 3*p[2] + p[1] + p[0] + q[0] + 4) >> 3; /* (22) */
        }
        else
            pa0 = (2*p[1] + p[0] + q[1] + 2) >> 2; /* (23) */

        if (aQ < beta && FFABS(p[0] - q[0]) < ((alpha>>2) + 2))
        {
            // Luminance filtering
            qa0 = (p[1] + 2*p[0] + 2*q[0] + 2*q[1] + q[2] + 4) >> 3; /* (20) */
            qa1 = (p[0] + q[0] + q[1] + q[2] + 2) >> 2; /* (21) */
            qa2 = (2*q[3] + 3*q[2] + q[1] + q[0] + p[0] + 4) >> 3; /* (22) */
        }
        else
            qa0 = (2*q[1] + q[0] + p[1] + 2) >> 2; /* (23) */
    }
    else
    {
        int aP = FFABS(p[2] - p[0]);
        int aQ = FFABS(q[2] - q[0]);
        int c0, c1;

        c0 = c1 = tc0_table[QP][bS-1];

        // All conditions are met to filter this line of samples

        delta0i = (((q[0] - p[0])<<2) + (p[1] - q[1]) + 4) >> 3;

        if (aP < beta) /* condition (8) */
        {
            /* c0 should be incremented for each condition being true, 8-473 */
            c0++;

            deltap1i = (p[2] + ((p[0] + q[0] + 1) >> 1) - (p[1]<<1)) >> 1;
            deltap1 = av_clip(deltap1i, -c1, c1);
            pa1 = p[1] + deltap1;
        }

        if (aQ < beta) /* condition (9) */
        {
            /* c0 should be incremented for each condition being true, 8-473 */
            c0++;

            deltaq1i = (q[2] + ((p[0] + q[0] + 1) >> 1) - (q[1]<<1)) >> 1;
            deltaq1 = av_clip(deltaq1i, -c1, c1);
            qa1 = q[1] + deltaq1;
        }

        delta0 = av_clip(delta0i, -c0, c0);
        pa0 = av_clip_uint8(p[0] + delta0);
        qa0 = av_clip_uint8(q[0] - delta0);
    }
    p[0] = pa0;
    p[1] = pa1;
    p[2] = pa2;
    q[0] = qa0;
    q[1] = qa1;
    q[2] = qa2;
}

static inline void h264_deblock_line_chroma(int p[4], int q[4], int QP, int bS)
{
    int delta0i, delta0;
    int pa0, pa1, pa2, qa0, qa1, qa2;
    int alpha, beta;

    if (bS == 0)
        return;

    alpha = alpha_table[QP];
    beta = beta_table[QP];

    if (!(
        (FFABS(p[0] - q[0]) < alpha) /* (1) */
        &&
        (FFABS(p[1] - p[0]) < beta) /* (2) */
        &&
        (FFABS(q[1] - q[0]) < beta) /* (3) */
        ))
        return;

    pa0 = p[0];
    pa1 = p[1];
    pa2 = p[2];
    qa0 = q[0];
    qa1 = q[1];
    qa2 = q[2];

    if (bS == 4)
    {
        pa0 = ((p[1]<<1) + p[0] + q[1] + 2) >> 2; /* (23) */
        qa0 = ((q[1]<<1) + q[0] + p[1] + 2) >> 2; /* (23) */
    }
    else
    {
        int c0, c1;

        c0 = c1 = tc0_table[QP][bS-1];

        // All conditions are met to filter this line of samples

        delta0i = (((q[0] - p[0])<<2) + (p[1] - q[1]) + 4) >> 3;

        c0++; /* p. 191, (8-474) */

        delta0 = av_clip(delta0i, -c0, c0);
        pa0 = av_clip_uint8(p[0] + delta0);
        qa0 = av_clip_uint8(q[0] - delta0);
    }
    p[0] = pa0;
    p[1] = pa1;
    p[2] = pa2;
    q[0] = qa0;
    q[1] = qa1;
    q[2] = qa2;
}

static void h264_deblock_macroblock(MacroBlock *mb, int filter_left_edge, int filter_top_edge, int isIDR, int QPYav, int QPCav)
{
    int p[4],q[4];
    int x,y;
    int bS[4][16];

    // First step is filtering of vertical edges

    // first filter left edge
    if (filter_left_edge)
    {
        MacroBlock *leftmb = mb->leftblock;

        // first Y
        for (y = 0 ; y < 16 ; y++)
        {
            if (isIDR)
                bS[0][y] = 4;
            else
            {
                if (leftmb->Y_nonzero[y>>2][3] != 0 || mb->Y_nonzero[y>>2][0] != 0)
                    bS[0][y] = 2;
                else
                {
                    if (FFABS(leftmb->mv_x - mb->mv_x) >= 4 || FFABS(leftmb->mv_y - mb->mv_y) >= 4)
                        bS[0][y] = 1;
                    else
                        bS[0][y] = 0;
                }
            }

            p[0] = leftmb->Y[y][15];
            p[1] = leftmb->Y[y][14];
            p[2] = leftmb->Y[y][13];
            p[3] = leftmb->Y[y][12];
            q[0] = mb->Y[y][0];
            q[1] = mb->Y[y][1];
            q[2] = mb->Y[y][2];
            q[3] = mb->Y[y][3];

            h264_deblock_line_luma(p,q,QPYav,bS[0][y]);

            leftmb->Y[y][15] = p[0];
            leftmb->Y[y][14] = p[1];
            leftmb->Y[y][13] = p[2];
            mb->Y[y][0] = q[0];
            mb->Y[y][1] = q[1];
            mb->Y[y][2] = q[2];
        }

        // then U and V

        for (y = 0 ; y < 8 ; y++)
        {
            p[0] = leftmb->U[y][7];
            p[1] = leftmb->U[y][6];
            p[2] = leftmb->U[y][5];
            p[3] = leftmb->U[y][4];
            q[0] = mb->U[y][0];
            q[1] = mb->U[y][1];
            q[2] = mb->U[y][2];
            q[3] = mb->U[y][3];

            h264_deblock_line_chroma(p,q,QPCav,bS[0][y<<1]);

            leftmb->U[y][7] = p[0];
            leftmb->U[y][6] = p[1];
            leftmb->U[y][5] = p[2];
            mb->U[y][0] = q[0];
            mb->U[y][1] = q[1];
            mb->U[y][2] = q[2];

            p[0] = leftmb->V[y][7];
            p[1] = leftmb->V[y][6];
            p[2] = leftmb->V[y][5];
            p[3] = leftmb->V[y][4];
            q[0] = mb->V[y][0];
            q[1] = mb->V[y][1];
            q[2] = mb->V[y][2];
            q[3] = mb->V[y][3];

            h264_deblock_line_chroma(p,q,QPCav,bS[0][y<<1]);

            leftmb->V[y][7] = p[0];
            leftmb->V[y][6] = p[1];
            leftmb->V[y][5] = p[2];
            mb->V[y][0] = q[0];
            mb->V[y][1] = q[1];
            mb->V[y][2] = q[2];
        }
    }

    // then the internal vertical edges

    for (x = 4 ; x < 16 ; x += 4)
    {
        int xidx = x >> 2;

        // first Y
        for (y = 0 ; y < 16 ; y++)
        {
            if (isIDR)
                bS[xidx][y] = 3;
            else
            {
                if (mb->Y_nonzero[y>>2][(x>>2)-1] != 0 || mb->Y_nonzero[y>>2][x>>2] != 0)
                    bS[xidx][y] = 2;
                else
                {
                    // one motion vector per 16x16 block, so there will be no difference
                    // between the motion vectors
                    bS[xidx][y] = 0;
                }
            }

            p[0] = mb->Y[y][x-1];
            p[1] = mb->Y[y][x-2];
            p[2] = mb->Y[y][x-3];
            p[3] = mb->Y[y][x-4];
            q[0] = mb->Y[y][x+0];
            q[1] = mb->Y[y][x+1];
            q[2] = mb->Y[y][x+2];
            q[3] = mb->Y[y][x+3];

            h264_deblock_line_luma(p,q,QPYav,bS[xidx][y]);

            mb->Y[y][x-1] = p[0];
            mb->Y[y][x-2] = p[1];
            mb->Y[y][x-3] = p[2];
            mb->Y[y][x+0] = q[0];
            mb->Y[y][x+1] = q[1];
            mb->Y[y][x+2] = q[2];
        }
    }

    // then U and V

    for (y = 0 ; y < 8 ; y++)
    {
        p[0] = mb->U[y][3];
        p[1] = mb->U[y][2];
        p[2] = mb->U[y][1];
        p[3] = mb->U[y][0];
        q[0] = mb->U[y][4];
        q[1] = mb->U[y][5];
        q[2] = mb->U[y][6];
        q[3] = mb->U[y][7];

        h264_deblock_line_chroma(p,q,QPCav,bS[2][y<<1]);

        mb->U[y][3] = p[0];
        mb->U[y][2] = p[1];
        mb->U[y][1] = p[2];
        mb->U[y][4] = q[0];
        mb->U[y][5] = q[1];
        mb->U[y][6] = q[2];

        p[0] = mb->V[y][3];
        p[1] = mb->V[y][2];
        p[2] = mb->V[y][1];
        p[3] = mb->V[y][0];
        q[0] = mb->V[y][4];
        q[1] = mb->V[y][5];
        q[2] = mb->V[y][6];
        q[3] = mb->V[y][7];

        h264_deblock_line_chroma(p,q,QPCav,bS[2][y<<1]);

        mb->V[y][3] = p[0];
        mb->V[y][2] = p[1];
        mb->V[y][1] = p[2];
        mb->V[y][4] = q[0];
        mb->V[y][5] = q[1];
        mb->V[y][6] = q[2];
    }

    // Next step is filtering of horizontal edges

    // first, filter top edge

    if (filter_top_edge)
    {
        MacroBlock *topmb = mb->topblock;

        // first Y
        for (x = 0 ; x < 16 ; x++)
        {
            if (isIDR)
                bS[0][x] = 4;
            else
            {
                if (topmb->Y_nonzero[3][x>>2] != 0 || mb->Y_nonzero[0][x>>2] != 0)
                    bS[0][x] = 2;
                else
                {
                    if (FFABS(topmb->mv_x - mb->mv_x) >= 4 || FFABS(topmb->mv_y - mb->mv_y) >= 4)
                        bS[0][x] = 1;
                    else
                        bS[0][x] = 0;
                }
            }

            p[0] = topmb->Y[15][x];
            p[1] = topmb->Y[14][x];
            p[2] = topmb->Y[13][x];
            p[3] = topmb->Y[12][x];
            q[0] = mb->Y[0][x];
            q[1] = mb->Y[1][x];
            q[2] = mb->Y[2][x];
            q[3] = mb->Y[3][x];

            h264_deblock_line_luma(p,q,QPYav,bS[0][x]);

            topmb->Y[15][x] = p[0];
            topmb->Y[14][x] = p[1];
            topmb->Y[13][x] = p[2];
            mb->Y[0][x] = q[0];
            mb->Y[1][x] = q[1];
            mb->Y[2][x] = q[2];
        }

        // then U and V

        for (x = 0 ; x < 8 ; x++)
        {
            p[0] = topmb->U[7][x];
            p[1] = topmb->U[6][x];
            p[2] = topmb->U[5][x];
            p[3] = topmb->U[4][x];
            q[0] = mb->U[0][x];
            q[1] = mb->U[1][x];
            q[2] = mb->U[2][x];
            q[3] = mb->U[3][x];

            h264_deblock_line_chroma(p,q,QPCav,bS[0][x<<1]);

            topmb->U[7][x] = p[0];
            topmb->U[6][x] = p[1];
            topmb->U[5][x] = p[2];
            mb->U[0][x] = q[0];
            mb->U[1][x] = q[1];
            mb->U[2][x] = q[2];

            p[0] = topmb->V[7][x];
            p[1] = topmb->V[6][x];
            p[2] = topmb->V[5][x];
            p[3] = topmb->V[4][x];
            q[0] = mb->V[0][x];
            q[1] = mb->V[1][x];
            q[2] = mb->V[2][x];
            q[3] = mb->V[3][x];

            h264_deblock_line_chroma(p,q,QPCav,bS[0][x<<1]);

            topmb->V[7][x] = p[0];
            topmb->V[6][x] = p[1];
            topmb->V[5][x] = p[2];
            mb->V[0][x] = q[0];
            mb->V[1][x] = q[1];
            mb->V[2][x] = q[2];
        }
    }

    // then the internal horizontal edges

    for (y = 4 ; y < 16 ; y += 4)
    {
        int yidx = y >> 2;

        // first Y
        for (x = 0 ; x < 16 ; x++)
        {
            if (isIDR)
                bS[yidx][x] = 3;
            else
            {
                if (mb->Y_nonzero[(y>>2)-1][(x>>2)] != 0 || mb->Y_nonzero[y>>2][x>>2] != 0)
                    bS[yidx][x] = 2;
                else
                {
                    // one motion vector per 16x16 block, so there will be no difference
                    // between the motion vectors
                    bS[yidx][x] = 0;
                }
            }

            p[0] = mb->Y[y-1][x];
            p[1] = mb->Y[y-2][x];
            p[2] = mb->Y[y-3][x];
            p[3] = mb->Y[y-4][x];
            q[0] = mb->Y[y+0][x];
            q[1] = mb->Y[y+1][x];
            q[2] = mb->Y[y+2][x];
            q[3] = mb->Y[y+3][x];

            h264_deblock_line_luma(p,q,QPYav,bS[yidx][x]);

            mb->Y[y-1][x] = p[0];
            mb->Y[y-2][x] = p[1];
            mb->Y[y-3][x] = p[2];
            mb->Y[y+0][x] = q[0];
            mb->Y[y+1][x] = q[1];
            mb->Y[y+2][x] = q[2];
        }
    }

    // then U and V

    for (x = 0 ; x < 8 ; x++)
    {
        p[0] = mb->U[3][x];
        p[1] = mb->U[2][x];
        p[2] = mb->U[1][x];
        p[3] = mb->U[0][x];
        q[0] = mb->U[4][x];
        q[1] = mb->U[5][x];
        q[2] = mb->U[6][x];
        q[3] = mb->U[7][x];

        h264_deblock_line_chroma(p,q,QPCav,bS[2][x<<1]);

        mb->U[3][x] = p[0];
        mb->U[2][x] = p[1];
        mb->U[1][x] = p[2];
        mb->U[4][x] = q[0];
        mb->U[5][x] = q[1];
        mb->U[6][x] = q[2];

        p[0] = mb->V[3][x];
        p[1] = mb->V[2][x];
        p[2] = mb->V[1][x];
        p[3] = mb->V[0][x];
        q[0] = mb->V[4][x];
        q[1] = mb->V[5][x];
        q[2] = mb->V[6][x];
        q[3] = mb->V[7][x];

        h264_deblock_line_chroma(p,q,QPCav,bS[2][x<<1]);

        mb->V[3][x] = p[0];
        mb->V[2][x] = p[1];
        mb->V[1][x] = p[2];
        mb->V[4][x] = q[0];
        mb->V[5][x] = q[1];
        mb->V[6][x] = q[2];
    }
}

void ff_h264_deblock(H264Context *t, FrameInfo *frame, int isIDR, int QPYav, int QPCav)
{
    int y,x;
    int w,h;

    w = t->mb_width;
    h = t->mb_height;

    // for the top row, only vertical filtering is done at the edges, for the top-left block, no filtering is
    // done at the edge

    h264_deblock_macroblock(&(frame->reconstructed_mb_map[0][0]),0,0,isIDR,QPYav,QPCav);
    for (x = 1 ; x < w ; x++)
        h264_deblock_macroblock(&(frame->reconstructed_mb_map[0][x]),1,0,isIDR,QPYav,QPCav);
    for (y = 1 ; y < h ; y++)
    {
        h264_deblock_macroblock(&(frame->reconstructed_mb_map[y][0]),0,1,isIDR,QPYav,QPCav);
        for (x = 1 ; x < w ; x++)
            h264_deblock_macroblock(&(frame->reconstructed_mb_map[y][x]),1,1,isIDR,QPYav,QPCav);
    }
}


void ff_h264dspenc_init(DSPContext* c, AVCodecContext *avctx)
{
    c->h264_dct = h264_dct_c;
    c->h264_idct_notranspose_add = ff_h264_idct_add_c;
    c->h264_hadamard_mult_4x4 = h264_hadamard_mult_4x4_c;
}
