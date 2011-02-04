/*
 * H.264 encoder
 * Copyright (c) 2006-2007 Expertisecentrum Digitale Media, UHasselt
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "libavutil/common.h"
#include "dsputil.h"
#include "avcodec.h"
#include "get_bits.h"
#include "mpegvideo.h"
#include "h264data.h"
#include "h264encdata.h"

//#define DEBUG_H264CAVLC

static int length_table[7][4095];
static int code_table[7][4095];

void ff_h264_cavlc_generate_tables()
{
    int vlcnum, level;
    for (vlcnum=0; vlcnum<7; vlcnum++)
    {
        for(level=-2047; level<2048; level++)
        {
            int sign = level < 0;
            int levabs = FFABS(level);
            int len, inf;

            if (vlcnum == 0)
            {

                if (levabs < 8)
                {
                    len = levabs * 2 + sign - 1;
                    inf = 1;
                }
                else if (levabs < 8+8)
                {
                    len = 14 + 1 + 4;
                    inf = (1 << 4) | ((levabs - 8) << 1) | sign;
                }
                else
                {
                    len = 14 + 2 + 12;
                    inf = (0x1 << 12) | ((levabs - 16)<< 1) | sign;
                }
                length_table[vlcnum][level+2047] = len;
                code_table[vlcnum][level+2047] = inf;
            }
            else
            {
                int iCodeword;
                int iLength;
                int shift = vlcnum-1;
                int escape = (15<<shift)+1;
                int numPrefix;
                int sufmask = ~((0xffffffff)<<shift);
                int suffix;

                numPrefix = (levabs-1)>>shift;
                suffix = (levabs-1)&sufmask;

#ifdef DEBUG_H264CAVLC
                av_log(NULL, AV_LOG_DEBUG, "numPrefix %d | suffix %d | levabs %d | escape %d | sufmask %d | vlcnum %d | level %d | sign %d\n",
                        numPrefix,suffix,levabs,escape,sufmask,vlcnum,level,sign);
#endif // DEBUG_H264CAVLC
                if (levabs < escape)
                {
                    iLength = numPrefix + vlcnum + 1;
                    iCodeword = (1<<(shift+1))|(suffix<<1)|sign;
                }
                else
                {
                    iLength = 28;
                    iCodeword = (1<<12)|((levabs-escape)<<1)|sign;
                }
                len = iLength;
                inf = iCodeword;

#ifdef DEBUG_H264CAVLC
                av_log(NULL, AV_LOG_DEBUG, "len %d | code %d\n",len,inf);
#endif // DEBUG_H264CAVLC

                length_table[vlcnum][level+2047] = len;
                code_table[vlcnum][level+2047] = inf;
            }
        }
    }
}

static inline void h264_cavlc_encode_vlc_level(PutBitContext *b, int vlcnum, int16_t level)
{
    int16_t index;
    index = level+2047;
    put_bits(b,length_table[vlcnum][index],code_table[vlcnum][index]);
#ifdef DEBUG_H264CAVLC
//    av_log(NULL, AV_LOG_DEBUG, "Encoded level with number %d\n",code_table[vlcnum][index]);
#endif
}

static inline void h264_cavlc_encode_vlc_totalzeros(PutBitContext *b, int vlcnum, int total_zeros)
{
    put_bits(b,total_zeros_len[vlcnum][total_zeros],total_zeros_bits[vlcnum][total_zeros]);
}

static inline void h264_cavlc_encode_vlc_run(PutBitContext *b, int vlcnum, int runbefore)
{
    put_bits(b,run_len[vlcnum][runbefore],run_bits[vlcnum][runbefore]);
}

static inline void h264_cavlc_encode_vlc_coefftoken(PutBitContext *b, int lookup_table, int total_coeffs, int trailing_ones)
{
    put_bits(b,coeff_token_len[lookup_table][trailing_ones+total_coeffs*4],coeff_token_bits[lookup_table][trailing_ones+total_coeffs*4]);
}

static inline void h264_cavlc_encode_vlc_coefftoken_chromadc(PutBitContext *b, int total_coeffs, int trailing_ones)
{
    put_bits(b,chroma_dc_coeff_token_len[trailing_ones + total_coeffs * 4],chroma_dc_coeff_token_bits[trailing_ones + total_coeffs * 4]);
}

static inline void h264_cavlc_encode_vlc_totalzeros_chromadc(PutBitContext *b, int vlcnum, int value)
{
    if(vlcnum + value == 3) put_bits(b, value  , 0);
    else                    put_bits(b, value+1, 1);
}

static inline int h264_cavlc_get_lookup_table(int na, int nb)
{
    int nc = 0;
    int8_t lookup_table[8] = {0, 0, 1, 1, 2, 2, 2, 2};

    if (na >= 0 && nb >= 0)
    {
        nc = na+nb+1;
        nc >>= 1;
    }
    else
    {
        if (na >= 0) // nB < 0
            nc = na;
        else if (nb >= 0) // nA < 0
            nc = nb;
    }

    return (nc < 8) ? lookup_table[nc] : 3;
}

int ff_h264_cavlc_encode(PutBitContext *b, int16_t *coefficients, int len, int na, int nb, int is_chroma_dc)
{
    static const int8_t increment_vlcnum[6] = { 0, 3, 6, 12, 24, 48 };

    int i, t;
    int total_coeffs;
    int trailing_ones;
    int total_zeros;
    int numlevels;
    int16_t levels[256];
    int16_t zeros[256];

#ifdef DEBUG_H264CAVLC
    for (i = 0 ; i < len ; i++)
        av_log(NULL, AV_LOG_DEBUG, "%6d",coefficients[i]);
    av_log(NULL, AV_LOG_DEBUG, "\n");
#endif

    // Count traling ones, total non-zero coefficients and the number of non-trailing zeros

    total_coeffs = 0;
    trailing_ones = 0;
    total_zeros = 0; // For now, we'll count the number of zeros at the end
    for (i = 0 ; i < len ; i++)
    {
        int16_t val = coefficients[i];
        if (val != 0)
        {
            levels[total_coeffs] = val;
            zeros[total_coeffs] = total_zeros;
            if (val == -1 || val == +1)
                trailing_ones++;
            else
                trailing_ones = 0;
            total_coeffs++;
            total_zeros = 0;
        }
        else
            total_zeros++;
    }
    if (trailing_ones > 3)
        trailing_ones = 3;

    total_zeros = len - total_zeros - total_coeffs; // The actual value of zeros (except the zeros at the end)
    numlevels = total_coeffs - trailing_ones;

    // Encode coeff_token. This is different for Chroma DC values

    if (!is_chroma_dc)
    {
        int lookupTable = h264_cavlc_get_lookup_table(na,nb);
#ifdef DEBUG_H264CAVLC
//        av_log(NULL, AV_LOG_DEBUG, "Luma: vlc=%d #c=%d #t1=%d\n", lookupTable, total_coeffs, trailing_ones);
#endif
        h264_cavlc_encode_vlc_coefftoken(b,lookupTable,total_coeffs,trailing_ones);
    }
    else
    {
#ifdef DEBUG_H264CAVLC
//        av_log(NULL, AV_LOG_DEBUG, "Chroma: #c=%d #t1=%d\n", total_coeffs, trailing_ones);
#endif
        h264_cavlc_encode_vlc_coefftoken_chromadc(b,total_coeffs,trailing_ones);
    }
    if (total_coeffs == 0) // Only zeros here, nothing left to do
        return 0;

    // Encode the trailing one sign bits

    for (i = total_coeffs-1, t = trailing_ones ; t > 0 ; i--, t--)
    {
        put_bits(b,1, levels[i] <= 0);
    }

    // Encode levels of the remaining nonzero coefficients

    if (numlevels > 0)
    {
        int level_two_or_higher = 1;
        int firstlevel = 1;
        int vlcnum;

        if (total_coeffs > 3 && trailing_ones == 3)
            level_two_or_higher = 0;

        vlcnum = total_coeffs > 10 && trailing_ones < 3;

        for (i = numlevels-1 ; i >= 0 ; i--)
        {
            int16_t val = levels[i];
            int16_t level = FFABS(val);

            if (level_two_or_higher)
            {
                val -= (val>>15)|1;
                level_two_or_higher = 0;
            }

#ifdef DEBUG_H264CAVLC
//            av_log(NULL, AV_LOG_DEBUG, "Encoding level %d with vlc %d\n",val,vlcnum);
#endif
            h264_cavlc_encode_vlc_level(b,vlcnum,val);

            // update VLC table
            if (vlcnum < 6 && level > increment_vlcnum[vlcnum])
                vlcnum++;

            if (firstlevel)
            {
                firstlevel = 0;
                if (level > 3)
                    vlcnum = 2;
            }
        }
    }

    // If necessary, encode the amount of non-trailing zeros

    if (total_coeffs < len)
    {
        int vlcnum = total_coeffs-1;

#ifdef DEBUG_H264CAVLC
//        av_log(NULL, AV_LOG_DEBUG, "Encoding total_zeros %d with vlc %d\n",total_zeros,vlcnum);
#endif

        if (!is_chroma_dc)
            h264_cavlc_encode_vlc_totalzeros(b,vlcnum,total_zeros);
        else
            h264_cavlc_encode_vlc_totalzeros_chromadc(b,vlcnum,total_zeros);
    }

    // If necessary, encode the run_before values

    for (i = total_coeffs-1 ; i > 0 && total_zeros > 0 ; i--)
    {
        int runbefore = zeros[i];
        int vlcnum = FFMIN(total_zeros-1, 6);

#ifdef DEBUG_H264CAVLC
//        av_log(NULL, AV_LOG_DEBUG, "Encoding run %d with vlc %d\n",runbefore,vlcnum);
#endif

        h264_cavlc_encode_vlc_run(b,vlcnum,runbefore);
        total_zeros -= runbefore;
    }

    return total_coeffs;
}

