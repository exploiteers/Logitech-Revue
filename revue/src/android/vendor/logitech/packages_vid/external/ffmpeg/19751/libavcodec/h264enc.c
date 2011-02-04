/*
 * H.264 encoder
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


//#include "common.h"
#include "get_bits.h"
#include "mpegvideo.h"
#include "h264data.h"
#include "h264pred.h"
#include "avcodec.h"
#include "golomb.h"
#include "dsputil.h"
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include "h264enc.h"
#include <sys/stat.h>

#define DEFAULT_QP                  30
#define NUMBER_OF_FRAMES             2
#define RATECONTROLINTERVAL        0.5
#define CHROMA_QP_INDEX_OFFSET_MAX  12
#define CHROMA_QP_INDEX_OFFSET_MIN -12
#define DEBUG_INTRA_PREDICTION       0
#define DEBUG_LEVEL_DETERMINATION    0
//#define DEBUG_WRITE_DECODED_IMAGE    0
//#define DISABLE_DEBLOCKING           1

#define RTP_H264_FUA 28
#define H264_NAL_NRI_MASK 0x60
#define H264_NAL_TYPE_MASK 0x1f

#define H264_COPY_4X4BLOCK_TRANSPOSED_PART(A, xoffset, yoffset, dest, src1, src2) \
    dest[0][A] = src1[yoffset+A][xoffset+0]-src2[yoffset+A][xoffset+0]; \
    dest[1][A] = src1[yoffset+A][xoffset+1]-src2[yoffset+A][xoffset+1]; \
    dest[2][A] = src1[yoffset+A][xoffset+2]-src2[yoffset+A][xoffset+2]; \
    dest[3][A] = src1[yoffset+A][xoffset+3]-src2[yoffset+A][xoffset+3];

#define H264_COPY_4X4BLOCK_TRANSPOSED(xoffset,yoffset,dest,src1,src2) \
{ \
    H264_COPY_4X4BLOCK_TRANSPOSED_PART(0, xoffset, yoffset, dest, src1, src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED_PART(1, xoffset, yoffset, dest, src1, src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED_PART(2, xoffset, yoffset, dest, src1, src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED_PART(3, xoffset, yoffset, dest, src1, src2); \
}

#define H264_COPY_16X16BLOCK(dest,src1,src2) \
{ \
    H264_COPY_4X4BLOCK_TRANSPOSED( 0, 0,dest[0][0],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 4, 0,dest[0][1],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 8, 0,dest[0][2],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED(12, 0,dest[0][3],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 0, 4,dest[1][0],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 4, 4,dest[1][1],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 8, 4,dest[1][2],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED(12, 4,dest[1][3],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 0, 8,dest[2][0],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 4, 8,dest[2][1],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 8, 8,dest[2][2],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED(12, 8,dest[2][3],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 0,12,dest[3][0],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 4,12,dest[3][1],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED( 8,12,dest[3][2],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED(12,12,dest[3][3],src1,src2); \
}

#define H264_COPY_8X8BLOCK(dest,src1,src2) \
{ \
    H264_COPY_4X4BLOCK_TRANSPOSED(0,0,dest[0][0],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED(4,0,dest[0][1],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED(0,4,dest[1][0],src1,src2); \
    H264_COPY_4X4BLOCK_TRANSPOSED(4,4,dest[1][1],src1,src2); \
}

extern int ff_h264_cavlc_encode(PutBitContext *b, DCTELEM *coefficients, int len, int nA, int nB, int isChromaDC);
extern void ff_h264_cavlc_generate_tables();
extern void ff_h264_deblock(H264Context *t, FrameInfo *frame, int isIDR, int QPYav, int QPCav);
extern void ff_pred16x16_vertical_c(uint8_t *src, int stride);
extern void ff_pred8x8_vertical_c(uint8_t *src, int stride);
extern void ff_pred16x16_horizontal_c(uint8_t *src, int stride);
extern void ff_pred8x8_horizontal_c(uint8_t *src, int stride);
extern void ff_pred16x16_plane_c(uint8_t *src, int stride);
extern void ff_pred8x8_plane_c(uint8_t *src, int stride);
extern void ff_pred16x16_dc_c(uint8_t *src, int stride);
extern void ff_pred8x8_dc_c(uint8_t *src, int stride);
extern void ff_pred16x16_top_dc_c(uint8_t *src, int stride);
extern void ff_pred8x8_top_dc_c(uint8_t *src, int stride);
extern void ff_pred16x16_left_dc_c(uint8_t *src, int stride);
extern void ff_pred8x8_left_dc_c(uint8_t *src, int stride);
extern void ff_pred16x16_128_dc_c(uint8_t *src, int stride);
extern void ff_pred8x8_128_dc_c(uint8_t *src, int stride);

const uint8_t ff_rem6[52]={ 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, 4, 5, 0, 1, 2, 3, };

const uint8_t ff_div6[52]={ 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, };


static const DCTELEM h264_MF00[6] = {13107, 11916, 10082, 9362, 8192, 7282};
static const DCTELEM h264_V00[6] = {10*16, 11*16, 13*16, 14*16, 16*16, 18*16};
static int8_t mbtype_map[4][3][2];

static uint8_t *rtp_buffer;
#ifdef DEBUG_DETERMINE_MAX_MV
static int max_mvx = INT_MIN;
static int max_mvy = INT_MIN;
static int min_mvx = INT_MAX;
static int min_mvy = INT_MAX;
#endif

#ifdef DEBUG_WRITE_DECODED_IMAGE
static void h264_append_image(uint8_t *data, AVCodecContext *avctx)
{
   int f = open("/tmp/teststream.yuv",O_CREAT|O_WRONLY|O_APPEND,S_IRUSR|S_IWUSR);
   write(f,data,avctx->width*avctx->height*3/2);
   close(f);
}
#endif

/**
 * Packetize according to RFC 3984. Only single NAL unit mode and FU-A are
 * currently implemented.
 */
static void h264_rtp_packetize(AVCodecContext *avctx, uint8_t *src, int srcsize)
{
    H264Context *t = (H264Context *)avctx->priv_data;

    assert(avctx->rtp_payload_size != NULL);

    if (srcsize < avctx->rtp_payload_size)
    {
        // NAL
        uint8_t *nalu_payload = src;
        int      tmpsize = srcsize;

        /* skip any data before the start code prefix */
        while (tmpsize > 4 && (nalu_payload[0] != 0x00 || nalu_payload[1] != 0x00 || nalu_payload[2] != 0x01))
        {
            nalu_payload++;
            tmpsize--;
//            av_log(avctx, AV_LOG_WARNING, "Skipping byte before startcode...\n");
        }

        if (tmpsize < 4)
            return;

        /* skip the start code prefix */
        nalu_payload += 3;
        tmpsize -= 3;

        emms_c();
        avctx->rtp_callback(avctx, nalu_payload, tmpsize, t->mb_width * t->mb_height);
    }
    else
    {
        // FU-A
        uint8_t *nalu_payload = src;
        uint8_t nalu_header;
        int      tmpsize = srcsize;
        int 	 start=1, end=0, first=0;

        /* skip any data before the start code prefix */
        while (tmpsize > 4 && (nalu_payload[0] != 0x00 || nalu_payload[1] != 0x00 || nalu_payload[2] != 0x01))
        {
            nalu_payload++;
            tmpsize--;
//            av_log(avctx, AV_LOG_WARNING, "Skipping byte before startcode... (FU-A)\n");
        }

        if (tmpsize < 4)
            return;

        /* skip the start code prefix */
        nalu_payload += 3;
        tmpsize -= 3;

        nalu_header = *nalu_payload;

        nalu_payload++; /* skip the NALU header */
        tmpsize--;

        while (!end)
        {
            int payload_size;

            payload_size = FFMIN (avctx->rtp_payload_size - 2, tmpsize);
            if (tmpsize==payload_size)
                end = 1;

            memset (rtp_buffer, 0, avctx->rtp_payload_size);

            rtp_buffer[0] = (nalu_header & H264_NAL_NRI_MASK) | RTP_H264_FUA;  // FU indicator
            rtp_buffer[1] = (start<<7)|(end<<6)|(nalu_header & H264_NAL_TYPE_MASK); // FU header

            memcpy (rtp_buffer + 2, nalu_payload + first, payload_size);

            emms_c();
            avctx->rtp_callback(avctx, rtp_buffer, payload_size + 2, end ? t->mb_width * t->mb_height : 0);

            tmpsize -= payload_size;
            first += payload_size;
            start=0;
        }
    }
}


/**
 * Write out the provided data into a NAL unit.
 * @param nal_ref_idc NAL reference IDC
 * @param nal_unit_type NAL unit payload type
 * @param dest the target buffer, dst+1 == src is allowed as a special case
 * @param destsize the length of the dst array
 * @param b2 the data which should be escaped
 * @returns pointer to current position in the output buffer or NULL if an error occured
 */
static uint8_t *h264_write_nal_unit(AVCodecContext *avctx, int nal_ref_idc, int nal_unit_type, uint8_t *dest, int *destsize,
                           PutBitContext *b2)
{
    PutBitContext b;
    int i, destpos, rbsplen, escape_count;
    uint8_t *rbsp;

    if (nal_unit_type != NAL_END_STREAM)
        put_bits(b2,1,1); // rbsp_stop_bit

    // Align b2 on a byte boundary
    align_put_bits(b2);
    rbsplen = put_bits_count(b2)/8;
    flush_put_bits(b2);
    rbsp = b2->buf;

    init_put_bits(&b,dest,*destsize);

    put_bits(&b,16,0);
    put_bits(&b,16,0x01);

    put_bits(&b,1,0); // forbidden zero bit
    put_bits(&b,2,nal_ref_idc);
    put_bits(&b,5,nal_unit_type);

    flush_put_bits(&b);

    destpos = 5;
    escape_count= 0;

    for (i=0; i<rbsplen; i+=2)
    {
        if (rbsp[i]) continue;
        if (i>0 && rbsp[i-1]==0)
            i--;
        if (i+2<rbsplen && rbsp[i+1]==0 && rbsp[i+2]<=3)
        {
            escape_count++;
            i+=2;
        }
    }

    if(escape_count==0)
    {
        if(dest+destpos != rbsp)
        {
            memcpy(dest+destpos, rbsp, rbsplen);
            *destsize -= (rbsplen+destpos);
        }

        if (avctx->rtp_mode && avctx->rtp_callback)
            h264_rtp_packetize (avctx, dest, rbsplen+destpos);

        return dest+rbsplen+destpos;
    }

    if(rbsplen + escape_count + 1> *destsize)
    {
        av_log(NULL, AV_LOG_ERROR, "Destination buffer too small!\n");
        return NULL;
    }

    // this should be damn rare (hopefully)
    for (i = 0 ; i < rbsplen ; i++)
    {
        if (i + 2 < rbsplen && (rbsp[i] == 0 && rbsp[i+1] == 0 && rbsp[i+2] < 4))
        {
            dest[destpos++] = rbsp[i++];
            dest[destpos++] = rbsp[i];
            dest[destpos++] = 0x03; // emulation prevention byte
        }
        else
            dest[destpos++] = rbsp[i];
    }
    *destsize -= destpos;

    if (avctx->rtp_mode && avctx->rtp_callback)
        h264_rtp_packetize (avctx, dest, destpos);

    return dest+destpos;
}

/**
 * For a specific picture, this function sets the correct Y,U and V start addresses for each macroblock.
 *
 * @param p The AVPicture for which the macroblock map will be initialized.
 * @param mb_map The map of macroblocks providing easy access to Y, U and V.
 * @param mb_width Width of the image in macroblocks.
 * @param mb_height Height of the image in macroblocks.
 */
static void h264_assign_macroblocks(AVPicture *p, MacroBlock **mb_map, int mb_width, int mb_height, int setneighbours)
{
    int y,x,i;
    int Ylinesize = p->linesize[0];
    int Ulinesize = p->linesize[1];
    int Vlinesize = p->linesize[2];

    if (!setneighbours)
    {
        for (y = 0 ; y < mb_height ; y++)
        {
            int y16 = y << 4;
            int y8 = y << 3;

            for (x = 0 ; x < mb_width ; x++)
            {
                int x16 = x << 4;
                int x8 = x << 3;

                for (i = 0 ; i < 8 ; i++)
                {
                    int ypos = y8+i;
                    mb_map[y][x].U[i] = p->data[1]+(x8+ypos*Ulinesize);
                    mb_map[y][x].V[i] = p->data[2]+(x8+ypos*Vlinesize);
                }
                for (i = 0 ; i < 16 ; i++)
                    mb_map[y][x].Y[i] = p->data[0]+(x16+(y16+i)*Ylinesize);

                mb_map[y][x].topblock = NULL;
                mb_map[y][x].leftblock = NULL;
                mb_map[y][x].rightblock = NULL;
                mb_map[y][x].available = 0;
            }
        }
    }
    else
    {
        y = 0;
        x = 0;
        for (i = 0 ; i < 8 ; i++)
        {
            mb_map[y][x].U[i] = p->data[1]+((x<<3)+((y<<3)+i)*Ulinesize);
            mb_map[y][x].V[i] = p->data[2]+((x<<3)+((y<<3)+i)*Vlinesize);
        }
        for (i = 0 ; i < 16 ; i++)
            mb_map[y][x].Y[i] = p->data[0]+((x<<4)+((y<<4)+i)*Ylinesize);

        mb_map[y][x].topblock = NULL;
        mb_map[y][x].leftblock = NULL;

        if (x < mb_width-1)
            mb_map[y][x].rightblock = &(mb_map[y][x+1]);
        else
            mb_map[y][x].rightblock = NULL;
        mb_map[y][x].available = 0;

        y = 0;
        for (x = 1 ; x < mb_width ; x++)
        {
            for (i = 0 ; i < 8 ; i++)
            {
                mb_map[y][x].U[i] = p->data[1]+((x<<3)+((y<<3)+i)*Ulinesize);
                mb_map[y][x].V[i] = p->data[2]+((x<<3)+((y<<3)+i)*Vlinesize);
            }
            for (i = 0 ; i < 16 ; i++)
                mb_map[y][x].Y[i] = p->data[0]+((x<<4)+((y<<4)+i)*Ylinesize);

            mb_map[y][x].topblock = NULL;
            mb_map[y][x].leftblock = &(mb_map[y][x-1]);
            if (x < mb_width-1)
                mb_map[y][x].rightblock = &(mb_map[y][x+1]);
            else
                mb_map[y][x].rightblock = NULL;
            mb_map[y][x].available = 0;
        }

        x = 0;
        for (y = 1 ; y < mb_height ; y++)
        {
            for (i = 0 ; i < 8 ; i++)
            {
                mb_map[y][x].U[i] = p->data[1]+((x<<3)+((y<<3)+i)*Ulinesize);
                mb_map[y][x].V[i] = p->data[2]+((x<<3)+((y<<3)+i)*Vlinesize);
            }
            for (i = 0 ; i < 16 ; i++)
                mb_map[y][x].Y[i] = p->data[0]+((x<<4)+((y<<4)+i)*Ylinesize);

            mb_map[y][x].topblock = &(mb_map[y-1][x]);
            mb_map[y][x].leftblock = NULL;
            if (x < mb_width-1)
                mb_map[y][x].rightblock = &(mb_map[y][x+1]);
            else
                mb_map[y][x].rightblock = NULL;
            mb_map[y][x].available = 0;
        }

        for (y = 1 ; y < mb_height ; y++)
        {
            for (x = 1 ; x < mb_width ; x++)
            {
                for (i = 0 ; i < 8 ; i++)
                {
                    mb_map[y][x].U[i] = p->data[1]+((x<<3)+((y<<3)+i)*Ulinesize);
                    mb_map[y][x].V[i] = p->data[2]+((x<<3)+((y<<3)+i)*Vlinesize);
                }
                for (i = 0 ; i < 16 ; i++)
                    mb_map[y][x].Y[i] = p->data[0]+((x<<4)+((y<<4)+i)*Ylinesize);

                mb_map[y][x].topblock = &(mb_map[y-1][x]);
                mb_map[y][x].leftblock = &(mb_map[y][x-1]);
                if (x < mb_width-1)
                    mb_map[y][x].rightblock = &(mb_map[y][x+1]);
                else
                    mb_map[y][x].rightblock = NULL;
                mb_map[y][x].available = 0;
            }
        }
    }
}

static void h264_clear_nonzero_markers(MacroBlock **mb_map, int mb_width, int mb_height)
{
    int x,y;

    for (y = 0 ; y < mb_height ; y++)
    {
        for (x = 0 ; x < mb_width ; x++)
        {
            // mark as not available

            memset(&(mb_map[y][x].Y_nonzero[0][0]),0xff,sizeof(mb_map[y][x].Y_nonzero)); // set to -1
            memset(&(mb_map[y][x].U_nonzero[0][0]),0xff,sizeof(mb_map[y][x].U_nonzero)); // set to -1
            memset(&(mb_map[y][x].V_nonzero[0][0]),0xff,sizeof(mb_map[y][x].V_nonzero)); // set to -1

            mb_map[y][x].available = 0;
        }
    }
}

static void h264_init_tables()
{
    int a, b, c;
    for(a=0; a<4; a++)
        for(b=0; b<3; b++)
            for(c=0; c<2; c++)
                mbtype_map[a][b][c] = 1 + a + 4*(b + 3*c);
}

/**
 * Generate a sequence parameter set (SPS).
 *
 */
static inline int h264_generate_sps(AVCodecContext *avctx, uint8_t **dest, int destlen)
{
    H264Context *t = (H264Context *)avctx->priv_data;
    PutBitContext b;

    // sequence parameter set rbsp
    init_put_bits(&b,t->pi_data0,t->bufsize);
    put_bits(&b,8,avctx->profile);

    // we obey constraints of A.2.1 (Baseline profile)
    put_bits(&b,1,1); // constraint_set0_flag

    // @FIXME: do we obey constraints of A.2.2 (Main profile)?
    put_bits(&b,1,0); // constraint_set1_flag

    // @FIXME: do we obey constraints of A.2.3 (Extended profile)?
    put_bits(&b,1,0); // constraint_set2_flag

    // @TODO: should this one be set when level 1.1?
    put_bits(&b,1,0); // constraint_set3_flag

    put_bits(&b,4,0); // reserved_zero_bits
    put_bits(&b,8,avctx->level);

    set_ue_golomb(&b,0); // seq_parameter_set_id
    set_ue_golomb(&b,t->log2_max_frame_num_minus4);
    set_ue_golomb(&b,2); // pic_order_cnt
    set_ue_golomb(&b,1); // num_ref_frames [0, 16]

    put_bits(&b,1,0); // gaps_in_frame_num_value_allowed_flag

    set_ue_golomb(&b,t->mb_width-1); // pic_width_in_mbs_minus1
    set_ue_golomb(&b,t->mb_height-1); // pic_height_in_map_units_minus1

    put_bits(&b, 1, 1); // frame_mbs_only_flag = 1 in Baseline
    put_bits(&b, 1, 0); // direct_8x8_inference_flag
    put_bits(&b, 1, t->frame_cropping_flag);

    if (t->frame_cropping_flag)
    {
        set_ue_golomb(&b, t->frame_crop_left_offset);
        set_ue_golomb(&b, t->frame_crop_right_offset);
        set_ue_golomb(&b, t->frame_crop_top_offset);
        set_ue_golomb(&b, t->frame_crop_bottom_offset);
    }

    put_bits(&b, 1, 0); // vui_parameters_present_flag

    *dest = h264_write_nal_unit(avctx, 3,NAL_SPS,*dest,&destlen,&b);
    return destlen;
}

/**
 * Generate a picture parameter set (PPS).
 *
 */
static inline int h264_generate_pps(AVCodecContext *avctx, uint8_t **dest, int destlen)
{
    H264Context *t = (H264Context *)avctx->priv_data;
    PutBitContext b;

    // picture parameter set
    init_put_bits(&b,t->pi_data0,t->bufsize);

    set_ue_golomb(&b,0); // pic_parameter_set_id
    set_ue_golomb(&b,0); // seq_parameter_set_id
    put_bits(&b,1,0); // entropy_coding_mode             0 = CAVLC
    put_bits(&b,1,0); // pic_order_present_flag
    set_ue_golomb(&b,0); // num_slice_groups_minus1            Only one slice group
    // List0 is needed for enabling P-slices
    set_ue_golomb(&b,0); // num_ref_idx_l0_active_minus1        Using at most the previous frame for prediction
    set_ue_golomb(&b,0); // num_ref_idx_l1_active_minus1        Definitely not using list 1 in baseline
    put_bits(&b,1,0); // weighted_pred_flag                Is 0 in baseline
    put_bits(&b,2,0); // weighted_bipred_idc            Is 0 in baseline

    set_se_golomb(&b,t->PPS_QP-26); // pic_init_qp_minus26
    set_se_golomb(&b,0); // pic_init_qs_minus26

    set_se_golomb(&b,t->chroma_qp_index_offset);

#ifndef DISABLE_DEBLOCKING
    put_bits(&b,1,0); // deblocking_filter_control_present_flag
#else
    if (avctx->flags & CODEC_FLAG_LOOP_FILTER)
        put_bits(&b,1,0); // deblocking_filter_control_present_flag
    else
        put_bits(&b,1,1); // deblocking_filter_control_present_flag
#endif /* DISABLE_DEBLOCKING */

    put_bits(&b,1,0); // constrained_intra_pred_flag
    put_bits(&b,1,0); // redundant_pic_cnt_present

    *dest = h264_write_nal_unit(avctx, 3,NAL_PPS,*dest,&destlen,&b);

    return destlen;
}

static inline int h264_generate_global_headers(AVCodecContext *avctx, uint8_t *dest, int destlen)
{
    destlen = h264_generate_sps(avctx, &dest, destlen);
    destlen = h264_generate_pps(avctx, &dest, destlen);
    return destlen;
}

/**
 * Level limits taken from the H.264 spec of 200503, table A-1 on p. 254.
 */
struct h264_level_limit {
    int level_idc; /* level_idc, p. 262, 10*level number */
    int max_mbps;
    int max_fs;
    float max_dpb;
    int max_br;
} h264_level_limits[] = {
    {10,   1485,    99,   148.5,     64},
    {11,   3000,   396,   337.5,    192},
    {12,   6000,   396,   891.0,    384},
    {13,  11880,   396,   891.0,    768},
    {20,  11880,   396,   891.0,   2000},
    {21,  19800,   792,  1782.0,   4000},
    {22,  11880,   396,  3037.5,   4000},
    {30,  40500,  1620,  3037.5,  10000},
    {31, 108000,  6750,  6750.0,  14000},
    {32, 216000,  7680,  7680.0,  20000},
    {40, 245760,  8192, 12288.0,  20000},
    {41, 245760,  8192, 12288.0,  50000},
    {42, 522240,  8704, 13056.0,  50000},
    {50, 589824, 22080, 41400.0, 135000},
    {51, 983040, 36864, 69120.0, 240000}
};
static const int h264_level_limits_nr = sizeof(h264_level_limits)/sizeof(struct h264_level_limit);

/**
 * Determine the necessary level to be able to decode this stream.
 */
int h264_determine_level(AVCodecContext *avctx)
{
    H264Context *t = (H264Context *)avctx->priv_data;
    int nr_macroblocks, mbps;
    int fps = 1.0/av_q2d(avctx->time_base);
    int level_index = 0;
    int dpb, max_br;

    if (DEBUG_LEVEL_DETERMINATION)
        av_log(avctx, AV_LOG_DEBUG, "fps: %d\n", fps);

    /* @FIXME: Is this the correct way to determine the DPB? Or should the
     * reference frames be taken into account? */
    dpb = t->refframe_width * t->refframe_height * 1.5; /* YUV420 */

    while (level_index < h264_level_limits_nr)
    {
        if (h264_level_limits[level_index].max_dpb * 1024 >= dpb) /* @FIXME: Are these inclusive? */
            break;
        level_index++;
    }

    if (level_index == h264_level_limits_nr)
    {
        av_log(avctx, AV_LOG_ERROR, "Decoded picture buffer size too large: No level available that allows this.");
        return 51;
    }

    if (DEBUG_LEVEL_DETERMINATION)
        av_log(avctx, AV_LOG_DEBUG, "Needed level for MaxDPB: %d\n", h264_level_limits[level_index].level_idc);

    /* Determine the total number of macroblocks */
    nr_macroblocks = t->mb_width * t->mb_height;

    if (DEBUG_LEVEL_DETERMINATION)
        av_log(avctx, AV_LOG_DEBUG, "Nr of macroblocks: %d\n", nr_macroblocks);

    while (level_index < h264_level_limits_nr)
    {
        if (h264_level_limits[level_index].max_fs >= nr_macroblocks) /* @FIXME: Are these inclusive? */
            break;
        level_index++;
    }

    if (level_index == h264_level_limits_nr)
    {
        av_log(avctx, AV_LOG_ERROR, "Frame size too large: No level available that allows this size.");
        return 51;
    }

    if (DEBUG_LEVEL_DETERMINATION)
        av_log(avctx, AV_LOG_DEBUG, "Needed level for MaxFS: %d\n", h264_level_limits[level_index].level_idc);

    mbps = nr_macroblocks * fps;

    while (level_index < h264_level_limits_nr)
    {
        if (h264_level_limits[level_index].max_mbps >= mbps) /* @FIXME: Are these inclusive? */
            break;
        level_index++;
    }

    if (level_index == h264_level_limits_nr)
    {
        av_log(avctx, AV_LOG_ERROR, "To many macroblocks per second: No level available that allows this.");
        return 51;
    }

    if (DEBUG_LEVEL_DETERMINATION)
        av_log(avctx, AV_LOG_DEBUG, "Needed level for MaxMBPS: %d\n", h264_level_limits[level_index].level_idc);

    if (!avctx->rc_max_rate)
    {
        av_log(avctx, AV_LOG_ERROR, "No maximum bitrate specified, bitstream may exceed maximum bitrate level constraints\n");
        max_br = avctx->bit_rate;
    }
    else
        max_br = avctx->rc_max_rate;

    while (level_index < h264_level_limits_nr)
    {
        if (h264_level_limits[level_index].max_br * 1000 >= max_br ) /* @FIXME: Are these inclusive? */
            break;
        level_index++;
    }

    if (level_index == h264_level_limits_nr)
    {
        av_log(avctx, AV_LOG_ERROR, "Maximum bit rate to high: No level available that allows this.");
        return 51;
    }

    if (DEBUG_LEVEL_DETERMINATION)
        av_log(avctx, AV_LOG_DEBUG, "Needed level for MaxBR: %d\n", h264_level_limits[level_index].level_idc);

    return h264_level_limits[level_index].level_idc;
}

static int h264_encoder_init(AVCodecContext *avctx)
{
    H264Context *t = (H264Context *)avctx->priv_data;
    uint8_t *buf;
    int s, x, y, i, res;
    int width, height;

    switch(avctx->pix_fmt){
    case PIX_FMT_YUV420P:
        break;
    default:
        av_log(avctx, AV_LOG_ERROR, "format not supported\n");
        return -1;
    }

    if (!avctx->gop_size)
        /* Intra only mode */
        avctx->gop_size = 1;

    t->log2_max_frame_num_minus4 = 12;
    t->max_frame_num = 1 << (t->log2_max_frame_num_minus4 + 4);

    t->frame_cropping_flag = 0;
    t->frame_crop_left_offset = 0;
    t->frame_crop_right_offset = 0;
    t->frame_crop_top_offset = 0;
    t->frame_crop_bottom_offset = 0;

    width = avctx->width;
    height = avctx->height;

    /* Determine the width and height in macroblocks */
    t->mb_width = width/16;
    t->mb_height = height/16;

    /* If the width is not a multiple of 16, enabling cropping */
    if (( width % 16) !=0 )
    {
        t->frame_cropping_flag = 1;
        t->frame_crop_left_offset = 0;
        t->frame_crop_right_offset = (width%16)/2;
        t->mb_width++;
    }

    /* If the height is not a multiple of 16, enabling cropping */
    if (( height % 16) !=0 )
    {
        t->frame_cropping_flag = 1;
        t->frame_crop_top_offset = 0;
        t->frame_crop_bottom_offset = (height%16)/2;
        t->mb_height++;
    }

    /* Round the framesize upwards to a multiple of 16 */
    width = t->mb_width * 16;
    height = t->mb_height * 16;
    t->refframe_width = width;
    t->refframe_height = height;

    /* Calculate the size of the buffer necessary to store one frame */
    s = avpicture_get_size(avctx->pix_fmt, width, height);

    /* Allocate storage space for the input frame */
    res = avpicture_alloc((AVPicture*)&t->pi, avctx->pix_fmt, width, height);
    if (res) {
        av_log(avctx, AV_LOG_ERROR, "Problem allocating picture\n");
        return -1;
    }

    /* Allocate storage space for the output bitstream */
    res = avpicture_alloc((AVPicture*)&t->po, avctx->pix_fmt, width, height);
    if (res) {
        av_log(avctx, AV_LOG_ERROR, "Problem allocating picture\n");
        return -1;
    }

    t->pi_data0 = (uint8_t *)t->pi.data[0];
    t->po_data0 = (uint8_t *)t->po.data[0];
    t->bufsize = s*2;
    t->frame_num = 0;
    t->frame_counter = 0;

    t->mb_map = av_malloc(sizeof(MacroBlock*) * t->mb_height);
    for (y = 0 ; y < t->mb_height ; y++)
    {
        t->mb_map[y] = av_malloc(sizeof(MacroBlock) * t->mb_width);
        for (x = 0 ; x < t->mb_width ; x++)
        {
            t->mb_map[y][x].Y_width = 16;
            t->mb_map[y][x].Y_height = 16;
        }
    }

    t->framebufsize = NUMBER_OF_FRAMES;
    t->reconstructed_frames = av_malloc(sizeof(FrameInfo *)*t->framebufsize);

    for (i = 0 ; i < t->framebufsize ; i++)
    {
        t->reconstructed_frames[i] = av_malloc(sizeof(FrameInfo));

        buf = av_malloc(s);
        avpicture_fill(&(t->reconstructed_frames[i]->reconstructed_picture), buf, PIX_FMT_YUV420P, width, height);

        t->reconstructed_frames[i]->reconstructed_mb_map = av_malloc(sizeof(MacroBlock*) * t->mb_height);
        for (y = 0 ; y < t->mb_height ; y++)
        {
            t->reconstructed_frames[i]->reconstructed_mb_map[y] = av_malloc(sizeof(MacroBlock) * t->mb_width);
            for (x = 0 ; x < t->mb_width ; x++)
            {
                t->reconstructed_frames[i]->reconstructed_mb_map[y][x].Y_width = 16;
                t->reconstructed_frames[i]->reconstructed_mb_map[y][x].Y_height = 16;
            }
        }
        h264_assign_macroblocks(&(t->reconstructed_frames[i]->reconstructed_picture),t->reconstructed_frames[i]->reconstructed_mb_map,t->mb_width,t->mb_height,1);
    }

    if (!avctx->global_quality)
    {
        t->QP = av_clip(DEFAULT_QP, avctx->qmin, avctx->qmax);
        t->use_fixed_qp = 0;
    }
    else
    {
        t->QP = avctx->global_quality / FF_QP2LAMBDA;
        t->use_fixed_qp = 1;
    }
    t->PPS_QP = t->QP;

    t->chroma_qp_index_offset = avctx->chromaoffset;
    t->chroma_qp_index_offset = av_clip(t->chroma_qp_index_offset, CHROMA_QP_INDEX_OFFSET_MIN, CHROMA_QP_INDEX_OFFSET_MAX);

    t->idr_pic_id = 0;

    if (avctx->profile == FF_PROFILE_UNKNOWN)
        avctx->profile = 66; // profile_idc = 66 in Baseline
    else
        if (avctx->profile != 66)
            av_log(avctx, AV_LOG_ERROR, "Profile: %d not supported by this encoder.\n", avctx->profile);

    if (avctx->level == FF_LEVEL_UNKNOWN)
        avctx->level = h264_determine_level(avctx);
    else
        if (avctx->level < h264_determine_level(avctx))
            av_log(avctx, AV_LOG_ERROR, "Level %d not sufficient for this encoding.\n", avctx->level);

    av_log(avctx, AV_LOG_ERROR, "Profile: %dLevel: %d\n", avctx->profile, avctx->level);
    av_log(avctx, AV_LOG_ERROR, "profile-level-id=%02X%02X%02X;\n", avctx->profile, 0x80, avctx->level);

    // init dsp
    dsputil_init(&(t->dspcontext),avctx);
    t->Y_stride = t->reconstructed_frames[0]->reconstructed_picture.linesize[0];
    t->U_stride = t->reconstructed_frames[0]->reconstructed_picture.linesize[1];
    t->V_stride = t->reconstructed_frames[0]->reconstructed_picture.linesize[2];

    ff_h264_pred_init(&t->hpc, CODEC_ID_H264);

    // Create an AVPicture instance with the same dimensions as the reference pictures to hold a copy
    // of the input frame
    buf = av_malloc(s);
    avpicture_fill(&(t->input_frame_copy), buf, PIX_FMT_YUV420P, width, height);
    memset(buf,0,s);

    // Assign the macroblock map to this copy of the input image
    h264_assign_macroblocks(&(t->input_frame_copy),t->mb_map,t->mb_width,t->mb_height,0);

    // Blocksize history, we use a separate history for I and P frame
    t->milliseconds_per_frame = (1000*avctx->time_base.num)/avctx->time_base.den;
    t->blocksize_history_length = (RATECONTROLINTERVAL*avctx->time_base.den)/avctx->time_base.num;
    t->blocksize_history = av_malloc(sizeof(int64_t)*t->blocksize_history_length);
    t->blocksize_history_pos = 0;
    t->blocksize_history_num_filled = 0;
    t->blocksize_history_total_milliseconds = 0;
    t->blocksize_history_sum = 0;
    for (i = 0 ; i < t->blocksize_history_length ; i++)
        t->blocksize_history[i] = 0;

    ff_h264_cavlc_generate_tables();
    h264_init_tables();

    if ((avctx->flags & CODEC_FLAG_GLOBAL_HEADER))
    {
        int res;

        avctx->extradata= av_malloc(1024);
        res = h264_generate_global_headers(avctx, avctx->extradata, 1024);
        avctx->extradata_size = 1024 - res;
    }

    // Alloc rtp buffer
    rtp_buffer = av_mallocz(avctx->rtp_payload_size);
    return 0;
}

static void h264_encode_i_pcm(MacroBlock *mb, PutBitContext *b, MacroBlock *copy_mb)
{
    int w = mb->Y_width;
    int h = mb->Y_height;
    int x,y;

    set_ue_golomb(b, 25); // mb_type = I_PCM
    align_put_bits(b);

    // Y

    for (y = 0 ; y < h ; y++)
    {
        for (x = 0 ; x < w ; x++)
            put_bits(b,8,mb->Y[y][x]);
        for ( ; x < 16 ; x++)
            put_bits(b,8,0);
    }
    for ( ; y < 16 ; y++)
    {
        for (x = 0 ; x < 16 ; x++)
            put_bits(b,8,0);
    }

    // copy Y

    for (y = 0 ; y < h ; y++)
        for (x = 0 ; x < w ; x++)
            copy_mb->Y[y][x] = mb->Y[y][x];

    w >>= 1;
    h >>= 1;

    // U

    for (y = 0 ; y < h ; y++)
    {
        for (x = 0 ; x < w ; x++)
            put_bits(b,8,mb->U[y][x]);
        for ( ; x < 8 ; x++)
            put_bits(b,8,0);
    }
    for ( ; y < 8 ; y++)
    {
        for (x = 0 ; x < 8 ; x++)
            put_bits(b,8,0);
    }

    // V

    for (y = 0 ; y < h ; y++)
    {
        for (x = 0 ; x < w ; x++)
            put_bits(b,8,mb->V[y][x]);
        for ( ; x < 8 ; x++)
            put_bits(b,8,0);
    }
    for ( ; y < 8 ; y++)
    {
        for (x = 0 ; x < 8 ; x++)
            put_bits(b,8,0);
    }

    // copy U and V

    for (y = 0 ; y < h ; y++)
    {
        for (x = 0 ; x < w ; x++)
        {
            copy_mb->U[y][x] = mb->U[y][x];
            copy_mb->V[y][x] = mb->V[y][x];
        }
    }

    // store the nonzero counts (set to 16 for I_PCM blocks)
    memset(copy_mb->Y_nonzero, 16, sizeof(copy_mb->Y_nonzero));
    memset(copy_mb->U_nonzero, 16, sizeof(copy_mb->U_nonzero));
    memset(copy_mb->V_nonzero, 16, sizeof(copy_mb->V_nonzero));

    copy_mb->available = 1;
}

#define NEIGHBOUR_SUBTYPE_Y 0
#define NEIGHBOUR_SUBTYPE_U 1
#define NEIGHBOUR_SUBTYPE_V 2

#define H264_NEIGHBOUR_COUNT_NONZERO_PLANE(PLANE, P) \
    { \
        if (x == 0) \
        { \
            MacroBlock *leftmb = mb->leftblock; \
            if (!leftmb) \
                *nA = -1; \
            else \
                *nA = leftmb->PLANE[y][P]; \
        } \
        else \
            *nA = mb->PLANE[y][x-1]; \
        if (y == 0) \
        { \
            MacroBlock *topmb = mb->topblock; \
            if (!topmb) \
                *nB = -1; \
            else \
                *nB = topmb->PLANE[P][x]; \
        } \
        else \
            *nB = mb->PLANE[y-1][x]; \
    }

static inline void h264_neighbour_count_nonzero(MacroBlock *mb, int type, int x, int y, int *nA, int *nB)
{
    if (type == NEIGHBOUR_SUBTYPE_Y)
        H264_NEIGHBOUR_COUNT_NONZERO_PLANE(Y_nonzero, 3)
    else if (type == NEIGHBOUR_SUBTYPE_U)
        H264_NEIGHBOUR_COUNT_NONZERO_PLANE(U_nonzero, 1)
    else
        H264_NEIGHBOUR_COUNT_NONZERO_PLANE(V_nonzero, 1)
}

#define clip_inplace(x, min, max) { int val = av_clip(x, min, max); x = val; }

#define H264_COUNT_AND_CLIP(x,count) \
{\
    if (x != 0)\
        count++;\
    clip_inplace(x, -2047, 2047);\
}

#define H264_COUNT_AND_CLIP_SUBBLOCK(x,count)\
{\
    H264_COUNT_AND_CLIP(x[0][1],count);\
    H264_COUNT_AND_CLIP(x[0][2],count);\
    H264_COUNT_AND_CLIP(x[0][3],count);\
    H264_COUNT_AND_CLIP(x[1][0],count);\
    H264_COUNT_AND_CLIP(x[1][1],count);\
    H264_COUNT_AND_CLIP(x[1][2],count);\
    H264_COUNT_AND_CLIP(x[1][3],count);\
    H264_COUNT_AND_CLIP(x[2][0],count);\
    H264_COUNT_AND_CLIP(x[2][1],count);\
    H264_COUNT_AND_CLIP(x[2][2],count);\
    H264_COUNT_AND_CLIP(x[2][3],count);\
    H264_COUNT_AND_CLIP(x[3][0],count);\
    H264_COUNT_AND_CLIP(x[3][1],count);\
    H264_COUNT_AND_CLIP(x[3][2],count);\
    H264_COUNT_AND_CLIP(x[3][3],count);\
}

static const int8_t zigzagx[16] = { 0,1,0,0,1,2,3,2,1,0,1,2,3,3,2,3 };
static const int8_t zigzagy[16] = { 0,0,1,2,1,0,0,1,2,3,3,2,1,2,3,3 };

#define H264_ENCODE_INTRA16X16_RESIDUAL_COEFFICIENTS(PLANE) \
        coefficients[0] = PLANE[0][0]; \
        coefficients[1] = PLANE[0][1]; \
        coefficients[2] = PLANE[1][0]; \
        coefficients[3] = PLANE[1][1]; \
        ff_h264_cavlc_encode(b,coefficients,4,-1,-1,1); // nA and nB are not used in this case

static void h264_encode_intra16x16_residual(PutBitContext *b,DCTELEM YD[4][4],DCTELEM UD[2][2],DCTELEM VD[2][2],
                         Residual *residual, int lumamode, int chromamode, MacroBlock *mb)
{
    int lumaACcount = 0;
    int chromaDCcount = 0;
    int chromaACcount = 0;
    int CodedBlockPatternChroma = 0;
    int CodedBlockPatternLuma = 0;
    int x,y,i,j;
    DCTELEM coefficients[256];
    int nA,nB;


    for (y = 0 ; y < 4 ; y++)
        for (x = 0 ; x < 4 ; x++)
            H264_COUNT_AND_CLIP_SUBBLOCK(residual->part4x4Y[y][x],lumaACcount);

    for (y = 0 ; y < 2 ; y++)
    {
        for (x = 0 ; x < 2 ; x++)
        {
            H264_COUNT_AND_CLIP_SUBBLOCK(residual->part4x4U[y][x],chromaACcount);
            H264_COUNT_AND_CLIP_SUBBLOCK(residual->part4x4V[y][x],chromaACcount);
        }
    }

    for (y = 0 ; y < 2 ; y++)
    {
        for (x = 0 ; x < 2 ; x++)
        {
            H264_COUNT_AND_CLIP(UD[y][x],chromaDCcount);
            H264_COUNT_AND_CLIP(VD[y][x],chromaDCcount);
        }
    }

    for (y = 0 ; y < 4 ; y++)
        for (x = 0 ; x < 4 ; x++)
            clip_inplace(YD[y][x], -2047, 2047);

    if(chromaACcount)
        CodedBlockPatternChroma= 2;
    else
        CodedBlockPatternChroma= !!chromaDCcount;

    if (lumaACcount == 0)
        CodedBlockPatternLuma = 0;
    else
        CodedBlockPatternLuma = 1; // actually it is 15 in the ITU spec, but I'd like to use it as an array index

    set_ue_golomb(b, mbtype_map[lumamode][CodedBlockPatternChroma][CodedBlockPatternLuma]); // mb_type
    set_ue_golomb(b, chromamode); // intra_chroma_pred_mode
    set_se_golomb(b, 0); // mb_qp_delta

    // encode luma DC coefficients

    h264_neighbour_count_nonzero(mb,NEIGHBOUR_SUBTYPE_Y,0,0,&nA,&nB);
    for (i = 0 ; i < 16 ; i++)
        coefficients[i] = YD[zigzagy[i]][zigzagx[i]];
    ff_h264_cavlc_encode(b,coefficients,16,nA,nB,0);

    if (CodedBlockPatternLuma > 0)
    {
        for (j = 0 ; j < 4 ; j++)
        {
            int X = (j&1) << 1;
            int Y = j&2;

            for (i = 0 ; i < 4 ; i++)
            {
                int x = (i&1)+X;
                int y = (i>>1)+Y;

                int k;

                for (k = 0 ; k < 15 ; k++)
                    coefficients[k] = residual->part4x4Y[y][x][zigzagy[k+1]][zigzagx[k+1]];
                h264_neighbour_count_nonzero(mb,NEIGHBOUR_SUBTYPE_Y,x,y,&nA,&nB);
                mb->Y_nonzero[y][x] = ff_h264_cavlc_encode(b,coefficients,15,nA,nB,0);
            }
        }
    }
    else
        memset(mb->Y_nonzero, 0, sizeof(mb->Y_nonzero));

    if (CodedBlockPatternChroma == 0)
    {
        memset(mb->U_nonzero, 0, sizeof(mb->U_nonzero));
        memset(mb->V_nonzero, 0, sizeof(mb->V_nonzero));
        return;
    }

    if (CodedBlockPatternChroma != 0)
    {
        H264_ENCODE_INTRA16X16_RESIDUAL_COEFFICIENTS(UD);
        H264_ENCODE_INTRA16X16_RESIDUAL_COEFFICIENTS(VD);
    }

    if (CodedBlockPatternChroma == 2)
    {
        for (i = 0 ; i < 4 ; i++)
        {
            int x = i&1;
            int y = i>>1;

            int k;

            for (k = 0 ; k < 15 ; k++)
                coefficients[k] = residual->part4x4U[y][x][zigzagy[k+1]][zigzagx[k+1]];
            h264_neighbour_count_nonzero(mb,NEIGHBOUR_SUBTYPE_U,x,y,&nA,&nB);
            mb->U_nonzero[y][x] = ff_h264_cavlc_encode(b,coefficients,15,nA,nB,0);
        }

        for (i = 0 ; i < 4 ; i++)
        {
            int x = i&1;
            int y = i>>1;

            int k;

            for (k = 0 ; k < 15 ; k++)
                coefficients[k] = residual->part4x4V[y][x][zigzagy[k+1]][zigzagx[k+1]];
            h264_neighbour_count_nonzero(mb,NEIGHBOUR_SUBTYPE_V,x,y,&nA,&nB);
            mb->V_nonzero[y][x] = ff_h264_cavlc_encode(b,coefficients,15,nA,nB,0);
        }
    }
    else
    {
        memset(mb->U_nonzero, 0, sizeof(mb->U_nonzero));
        memset(mb->V_nonzero, 0, sizeof(mb->V_nonzero));
    }
}

#define COPY_SIGN(A, B) ((A ^ (B>>31)) - (B>>31))

#define H264_DCT_QUANT_C_ELEMENT(X, Y) \
    block[X][Y] = COPY_SIGN(((FFABS((int32_t)block[X][Y])*MF[mod][X][Y]+f+bias) >> qbits), block[X][Y])

#define H264_DCT_QUANT_C_LINE(X) \
    H264_DCT_QUANT_C_ELEMENT(X,0); \
    H264_DCT_QUANT_C_ELEMENT(X,1); \
    H264_DCT_QUANT_C_ELEMENT(X,2); \
    H264_DCT_QUANT_C_ELEMENT(X,3);

// we'll always work with transposed input blocks, to avoid having to make a distinction between
// C and mmx implementations
static void h264_dct_quant(AVCodecContext *avctx, DCTELEM block[4][4], int QP, int dontscaleDC, int intra) // y,x indexing
{
    H264Context *t = (H264Context *)avctx->priv_data;
    DSPContext *dsp = &t->dspcontext;
    static const DCTELEM MF[6][4][4] =
    {
        { { 13107, 8066, 13107, 8066}, {  8066, 5243,  8066, 5243}, { 13107, 8066, 13107, 8066}, {  8066, 5243,  8066, 5243} },
        { { 11916, 7490, 11916, 7490}, {  7490, 4660,  7490, 4660}, { 11916, 7490, 11916, 7490}, {  7490, 4660,  7490, 4660} },
        { { 10082, 6554, 10082, 6554}, {  6554, 4194,  6554, 4194}, { 10082, 6554, 10082, 6554}, {  6554, 4194,  6554, 4194} },
        { {  9362, 5825,  9362, 5825}, {  5825, 3647,  5825, 3647}, {  9362, 5825,  9362, 5825}, {  5825, 3647,  5825, 3647} },
        { {  8192, 5243,  8192, 5243}, {  5243, 3355,  5243, 3355}, {  8192, 5243,  8192, 5243}, {  5243, 3355,  5243, 3355} },
        { {  7282, 4559,  7282, 4559}, {  4559, 2893,  4559, 2893}, {  7282, 4559,  7282, 4559}, {  4559, 2893,  4559, 2893} }
    };
    int32_t qbits = 15 + ff_div6[QP];
    int32_t f;
    int mod = ff_rem6[QP];
    int bias;
    if (intra) {
         f =(1<<qbits)/3;
         bias = avctx->intra_quant_bias == FF_DEFAULT_QUANT_BIAS ? 0 : avctx->intra_quant_bias;
    }
    else {
        f = (1<<qbits)/6;
        bias = avctx->inter_quant_bias == FF_DEFAULT_QUANT_BIAS ? 0 : avctx->inter_quant_bias;
    }

    dsp->h264_dct(block);

    if (!dontscaleDC)
        H264_DCT_QUANT_C_ELEMENT(0,0);
    H264_DCT_QUANT_C_ELEMENT(0,1);
    H264_DCT_QUANT_C_ELEMENT(0,2);
    H264_DCT_QUANT_C_ELEMENT(0,3);

    H264_DCT_QUANT_C_LINE(1);
    H264_DCT_QUANT_C_LINE(2);
    H264_DCT_QUANT_C_LINE(3);
}

#define H264_INV_QUANT_DCT_ADD_C_ELEMENT(X, Y) \
        elem[X][Y] = ((int32_t)block[X][Y]*V[mod][X][Y]) << shift;

#define H264_INV_QUANT_DCT_ADD_C_LINE(X) \
    H264_INV_QUANT_DCT_ADD_C_ELEMENT(X, 0) \
    H264_INV_QUANT_DCT_ADD_C_ELEMENT(X, 1) \
    H264_INV_QUANT_DCT_ADD_C_ELEMENT(X, 2) \
    H264_INV_QUANT_DCT_ADD_C_ELEMENT(X, 3)

#define H264_INV_QUANT_DCT_ADD_C_ELEMENT2(X, Y) \
    elem[X][Y] = ((int32_t)block[X][Y]*V[mod][X][Y]+add) >> shift;

#define H264_INV_QUANT_DCT_ADD_C_LINE2(X) \
    H264_INV_QUANT_DCT_ADD_C_ELEMENT2(X, 0) \
    H264_INV_QUANT_DCT_ADD_C_ELEMENT2(X, 1) \
    H264_INV_QUANT_DCT_ADD_C_ELEMENT2(X, 2) \
    H264_INV_QUANT_DCT_ADD_C_ELEMENT2(X, 3)

static void h264_inv_quant_dct_add(DCTELEM block[4][4], int QP, int dontscaleDC, uint8_t *dst, int stride) // y,x indexing
{
    static const DCTELEM V[6][4][4] =
    {
        { { 10*16, 13*16, 10*16, 13*16}, { 13*16, 16*16, 13*16, 16*16}, { 10*16, 13*16, 10*16, 13*16}, { 13*16, 16*16, 13*16, 16*16} },
        { { 11*16, 14*16, 11*16, 14*16}, { 14*16, 18*16, 14*16, 18*16}, { 11*16, 14*16, 11*16, 14*16}, { 14*16, 18*16, 14*16, 18*16} },
        { { 13*16, 16*16, 13*16, 16*16}, { 16*16, 20*16, 16*16, 20*16}, { 13*16, 16*16, 13*16, 16*16}, { 16*16, 20*16, 16*16, 20*16} },
        { { 14*16, 18*16, 14*16, 18*16}, { 18*16, 23*16, 18*16, 23*16}, { 14*16, 18*16, 14*16, 18*16}, { 18*16, 23*16, 18*16, 23*16} },
        { { 16*16, 20*16, 16*16, 20*16}, { 20*16, 25*16, 20*16, 25*16}, { 16*16, 20*16, 16*16, 20*16}, { 20*16, 25*16, 20*16, 25*16} },
        { { 18*16, 23*16, 18*16, 23*16}, { 23*16, 29*16, 23*16, 29*16}, { 18*16, 23*16, 18*16, 23*16}, { 23*16, 29*16, 23*16, 29*16} }
    };
    DCTELEM elem[4][4];
    int mod = ff_rem6[QP];

    if (QP >= 24)
    {
        int shift = ff_div6[QP]-4;

        if (dontscaleDC)
            elem[0][0] = block[0][0];
        else
            H264_INV_QUANT_DCT_ADD_C_ELEMENT(0, 0);
        H264_INV_QUANT_DCT_ADD_C_ELEMENT(0, 1);
        H264_INV_QUANT_DCT_ADD_C_ELEMENT(0, 2);
        H264_INV_QUANT_DCT_ADD_C_ELEMENT(0, 3);

        H264_INV_QUANT_DCT_ADD_C_LINE(1);
        H264_INV_QUANT_DCT_ADD_C_LINE(2);
        H264_INV_QUANT_DCT_ADD_C_LINE(3);
    }
    else
    {
        int add = (1<<(3-ff_div6[QP]));
        int shift = (4-ff_div6[QP]);
        if (dontscaleDC)
            elem[0][0] = block[0][0];
        else
            H264_INV_QUANT_DCT_ADD_C_ELEMENT2(0, 0);
        H264_INV_QUANT_DCT_ADD_C_ELEMENT2(0, 1);
        H264_INV_QUANT_DCT_ADD_C_ELEMENT2(0, 2);
        H264_INV_QUANT_DCT_ADD_C_ELEMENT2(0, 3);
        H264_INV_QUANT_DCT_ADD_C_LINE2(1);
        H264_INV_QUANT_DCT_ADD_C_LINE2(2);
        H264_INV_QUANT_DCT_ADD_C_LINE2(3);
        if (dontscaleDC)
            elem[0][0] = block[0][0];
    }

    ff_h264_idct_add_c(dst,&(elem[0][0]),stride);
}

#define H264_HADAMARD_QUANT_2X2_C_ELEMENT(A, B) \
    Y[A][B] = COPY_SIGN(((FFABS(Y[A][B])*MF + f2) >> shift),Y[A][B])

/**
 * |ZD(i,j)| = (|YD(i,j)| MF(0,0) + 2 f) >> (qbits + 1)
 *
 */
static void h264_hadamard_quant_2x2(DCTELEM Y[2][2], int QP)
{
    int qbits = 15 + ff_div6[QP];
    int f2 = ((1 << qbits) / 3)*2;
    int shift = qbits+1;
    int32_t MF = h264_MF00[ff_rem6[QP]];

    H264_HADAMARD_QUANT_2X2_C_ELEMENT(0, 0);
    H264_HADAMARD_QUANT_2X2_C_ELEMENT(0, 1);
    H264_HADAMARD_QUANT_2X2_C_ELEMENT(1, 0);
    H264_HADAMARD_QUANT_2X2_C_ELEMENT(1, 1);
}

inline void ff_h264_hadamard_inv_quant_2x2(DCTELEM Y[2][2], int QP)
{
    int32_t V = h264_V00[QP%6];
    int div = QP/6;

    V <<= div;
    Y[0][0] = (Y[0][0]*V) >> 5;
    Y[0][1] = (Y[0][1]*V) >> 5;
    Y[1][0] = (Y[1][0]*V) >> 5;
    Y[1][1] = (Y[1][1]*V) >> 5;
}

#define H264_HADAMARD_QUANT_4X4_C_ELEMENT(A, B) \
    Y[A][B] = COPY_SIGN((((FFABS(Y[A][B])>>1) * MF + f2) >> shift), Y[A][B]);

#define H264_HADAMARD_QUANT_4X4_C_LINE(X) \
    H264_HADAMARD_QUANT_4X4_C_ELEMENT(X, 0); \
    H264_HADAMARD_QUANT_4X4_C_ELEMENT(X, 1); \
    H264_HADAMARD_QUANT_4X4_C_ELEMENT(X, 2); \
    H264_HADAMARD_QUANT_4X4_C_ELEMENT(X, 3);

/**
 * |ZD(i,j)| = (|YD(i,j)| MF(0,0) + 2 f) >> (qbits + 1)
 *
 */
static void h264_hadamard_quant_4x4(DCTELEM Y[4][4], int QP)
{
    int qbits = 15 + ff_div6[QP];
    int f2 = (1 << qbits) * (2/3);
    int shift = (qbits + 1);
    int mod = ff_rem6[QP];

    int32_t MF = h264_MF00[mod];

    H264_HADAMARD_QUANT_4X4_C_LINE(0);
    H264_HADAMARD_QUANT_4X4_C_LINE(1);
    H264_HADAMARD_QUANT_4X4_C_LINE(2);
    H264_HADAMARD_QUANT_4X4_C_LINE(3);
}

#define H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_ELEMENT(A, B) \
    Y[A][B] = (Y[A][B]*V + f) >> shift;

#define H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_LINE(A) \
    H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_ELEMENT(A, 0) \
    H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_ELEMENT(A, 1) \
    H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_ELEMENT(A, 2) \
    H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_ELEMENT(A, 3)

#define H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_ELEMENT(A,B) \
    Y[A][B] = (Y[A][B]*V) << shift ;

#define H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_LINE(A) \
    H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_ELEMENT(A, 0) \
    H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_ELEMENT(A, 1) \
    H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_ELEMENT(A, 2) \
    H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_ELEMENT(A, 3)

/*
 * Only if qpprime_y_zero_transform_bypass_flag == 0
 */
static void h264_hadamard_inv_quant_4x4(DCTELEM Y[4][4], int QP)
{
    int mod = ff_rem6[QP];

    if (QP < 36)
    {
        int qbits = ff_div6[QP];
        int shift = 6-qbits;
        int f = (1 << (5-qbits));

        int32_t V = h264_V00[mod];

        H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_LINE(0);
        H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_LINE(1);
        H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_LINE(2);
        H264_HADAMARD_INV_QUANT_4X4_C_LOWQP_LINE(3);
    }
    else
    {
        int shift = ff_div6[QP] - 6;
        int32_t V = h264_V00[mod];

        H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_LINE(0);
        H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_LINE(1);
        H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_LINE(2);
        H264_HADAMARD_INV_QUANT_4X4_C_HIGHQP_LINE(3);
    }
}

static int h264_encode_intra_16x16(AVCodecContext *avctx, MacroBlock *targetmb, PutBitContext *b,
                                 MacroBlock *destmb)
{
    H264Context *t = (H264Context *)avctx->priv_data;
    DSPContext dsp = t->dspcontext;
    int x,y;
    int w,h,w2,h2;
    DCTELEM YD[4][4];
    DCTELEM UD[2][2];
    DCTELEM VD[2][2];
    int qPI;
    int QPc;
    int QPy = t->QP;
    int lumapredmode = 2;
    int chromapredmode = 0;
    int leftavail = 0;
    int topavail = 0;
    int topleftavail = 0;
    enum {
        H264_INTRA_DC = 0,
        H264_INTRA_VERTICAL,
        H264_INTRA_HORIZONTAL,
        H264_INTRA_PLANE
    };
    int available_prediction_strategies = 0;
    int prediction_strategy = -1;
    int sad;
    int lowest_sad = INT_MAX;

    qPI = t->QP + t->chroma_qp_index_offset;
    qPI = av_clip(qPI, 0, 51);
    QPc = chroma_qp[qPI];

    w = targetmb->Y_width;
    h = targetmb->Y_height;
    w2 = w>>1;
    h2 = h>>1;

    /* Determine which blocks are available */
    if (destmb->leftblock != NULL && destmb->leftblock->available)
        leftavail = 1;
    if (destmb->topblock != NULL && destmb->topblock->available)
        topavail = 1;
    if (destmb->leftblock != NULL && destmb->leftblock->topblock && destmb->leftblock->topblock->available)
        topleftavail = 1;

    /* Determine which strategies can be used */

    /* DC prediction is always possible */
    available_prediction_strategies |= (1 << H264_INTRA_DC );

    /* Is horizontal prediction available? */
    if (leftavail)
        available_prediction_strategies |= (1 << H264_INTRA_HORIZONTAL);

    /* Is vertical prediction available? */
    if (topavail)
        available_prediction_strategies |= (1 << H264_INTRA_VERTICAL);

    /* Is plane prediction available? */
    if (leftavail && topavail && topleftavail)
        available_prediction_strategies |= (1 << H264_INTRA_PLANE);

    /* Try all available intra coding methods on this macroblock,
     * to determine which method results in the lowest residual
     * values.
     */

    if (available_prediction_strategies & (1 << H264_INTRA_PLANE)) {
        t->hpc.pred16x16[PLANE_PRED8x8](destmb->Y[0], t->refframe_width);
        /* Calculate the SAD using this prediction */
        sad = t->dspcontext.pix_abs[0][0](0, targetmb->Y[0], destmb->Y[0], t->Y_stride, 16);
        if (sad < lowest_sad) {
            lowest_sad = sad;
            prediction_strategy = H264_INTRA_PLANE;
        }
        if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "plane sad: %d\n", sad);
    }

    if (available_prediction_strategies & (1 << H264_INTRA_HORIZONTAL)) {
        t->hpc.pred16x16[HOR_PRED8x8](destmb->Y[0], t->refframe_width);
        /* Calculate the SAD using this prediction */
        sad = t->dspcontext.pix_abs[0][0](0, targetmb->Y[0], destmb->Y[0], t->Y_stride, 16);
        if (sad < lowest_sad) {
            lowest_sad = sad;
            prediction_strategy = H264_INTRA_HORIZONTAL;
        }
        if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "horizontal sad: %d\n", sad);
    }

    if (available_prediction_strategies & (1 << H264_INTRA_VERTICAL)) {
        t->hpc.pred16x16[VERT_PRED8x8](destmb->Y[0], t->refframe_width);
        /* Calculate the SAD using this prediction */
        sad = t->dspcontext.pix_abs[0][0](0, targetmb->Y[0], destmb->Y[0], t->Y_stride, 16);
        if (sad < lowest_sad) {
            lowest_sad = sad;
            prediction_strategy = H264_INTRA_VERTICAL;
        }
        if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "vertical sad: %d\n", sad);
    }


    if (available_prediction_strategies & (1 << H264_INTRA_DC)) {
        if(topavail) {
            if(leftavail) {
                if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "all avail, using average\n");
                t->hpc.pred16x16[DC_PRED8x8](destmb->Y[0], t->refframe_width);
            }
            else {
                if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "top avail, using average of top\n");
                t->hpc.pred16x16[TOP_DC_PRED8x8](destmb->Y[0], t->refframe_width);
            }
        }
        else {
            if(leftavail) {
                if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "left avail, using average of left\n");
                t->hpc.pred16x16[LEFT_DC_PRED8x8](destmb->Y[0], t->refframe_width);
            }
            else {
                if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "nothing avail, using 128\n");
                t->hpc.pred16x16[DC_128_PRED8x8](destmb->Y[0], t->refframe_width);
            }
        }
        /* Calculate the SAD using this prediction */
        sad = t->dspcontext.pix_abs[0][0](0, targetmb->Y[0], destmb->Y[0], t->Y_stride, 16);
        if (sad < lowest_sad) {
            lowest_sad = sad;
            prediction_strategy = H264_INTRA_DC;
        }
        if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "DC sad: %d\n", sad);
    }

    if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "lowest sad: %d strategy: %d\n", lowest_sad, prediction_strategy);

    /* FIXME: Determine a good value to switch to PCM mode */
    if (lowest_sad >= 24000)
    {
        if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "lowest sad: %d is still to high\n", lowest_sad);
        return -1;
    }

    if (prediction_strategy == -1) {
        av_log(avctx, AV_LOG_ERROR, "Error: Did not find a strategy... this should be impossible!!!\n");
        prediction_strategy = H264_INTRA_DC;
    }

    /* Use the prediction strategy which resulted in the lowest SAD */
    switch(prediction_strategy) {
        case H264_INTRA_PLANE:
            if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "INTRA_PLANE\n");
            t->hpc.pred16x16[PLANE_PRED8x8](destmb->Y[0], t->refframe_width);
            t->hpc.pred8x8[PLANE_PRED8x8](destmb->U[0], t->refframe_width>>1);
            t->hpc.pred8x8[PLANE_PRED8x8](destmb->V[0], t->refframe_width>>1);
            lumapredmode = PLANE_PRED8x8;
            chromapredmode = PLANE_PRED8x8;
            break;

        case H264_INTRA_HORIZONTAL:
            if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "INTRA_HOR\n");
            // Horizontal prediction
            t->hpc.pred16x16[HOR_PRED8x8](destmb->Y[0], t->refframe_width);
            t->hpc.pred8x8[HOR_PRED8x8](destmb->U[0], t->refframe_width>>1);
            t->hpc.pred8x8[HOR_PRED8x8](destmb->V[0], t->refframe_width>>1);
            lumapredmode = HOR_PRED8x8;
            chromapredmode = HOR_PRED8x8;
            break;

        case H264_INTRA_VERTICAL:
            if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "INTRA_VERTICAL\n");
            // Vertical prediction
            t->hpc.pred16x16[VERT_PRED8x8](destmb->Y[0], t->refframe_width);
            t->hpc.pred8x8[VERT_PRED8x8](destmb->U[0], t->refframe_width>>1);
            t->hpc.pred8x8[VERT_PRED8x8](destmb->V[0], t->refframe_width>>1);
            lumapredmode = VERT_PRED;
            chromapredmode = VERT_PRED8x8;
            break;


        case H264_INTRA_DC:
            if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "INTRA_DC\n");
            if ((!topavail) && (!leftavail))
            //if (1) // XXX: Illegal! Decoder will not know that 128 was used,
            //but will use prediction based on previous blocks.
            {
                if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "nothing avail, using 128\n");
                t->hpc.pred16x16[DC_128_PRED8x8](destmb->Y[0], t->refframe_width);
                t->hpc.pred8x8[DC_128_PRED8x8](destmb->U[0], t->refframe_width>>1);
                t->hpc.pred8x8[DC_128_PRED8x8](destmb->V[0], t->refframe_width>>1);
            }
            else
            {
                if(topavail)
                {
                    if(leftavail) {
                        if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "all avail, using average\n");
                        t->hpc.pred16x16[DC_PRED8x8](destmb->Y[0], t->refframe_width);
                        t->hpc.pred8x8[DC_PRED8x8](destmb->U[0], t->refframe_width>>1);
                        t->hpc.pred8x8[DC_PRED8x8](destmb->V[0], t->refframe_width>>1);
                    }
                    else {
                        if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "top avail, using average of top\n");
                        t->hpc.pred16x16[TOP_DC_PRED8x8](destmb->Y[0], t->refframe_width);
                        t->hpc.pred8x8[TOP_DC_PRED8x8](destmb->U[0], t->refframe_width>>1);
                        t->hpc.pred8x8[TOP_DC_PRED8x8](destmb->V[0], t->refframe_width>>1);
                    }
                }
                else
                {
                    if(leftavail) {
                        if (DEBUG_INTRA_PREDICTION) av_log(avctx, AV_LOG_DEBUG, "left avail, using average of left\n");
                        t->hpc.pred16x16[LEFT_DC_PRED8x8](destmb->Y[0], t->refframe_width);
                        t->hpc.pred8x8[LEFT_DC_PRED8x8](destmb->U[0], t->refframe_width>>1);
                        t->hpc.pred8x8[LEFT_DC_PRED8x8](destmb->V[0], t->refframe_width>>1);
                    }
                    else
                    {
                        av_log(avctx, AV_LOG_ERROR, "error! we should not get here!\n");

                    }

                }

            }
            lumapredmode = DC_PRED;
            chromapredmode = DC_PRED8x8;
            break;
    }

    H264_COPY_16X16BLOCK(t->residual.part4x4Y,(int16_t)targetmb->Y,(int16_t)destmb->Y);
    H264_COPY_8X8BLOCK(t->residual.part4x4U,(int16_t)targetmb->U,(int16_t)destmb->U);
    H264_COPY_8X8BLOCK(t->residual.part4x4V,(int16_t)targetmb->V,(int16_t)destmb->V);

    // Transform residual: DCT

    for (y = 0 ; y < 4 ; y++)
    {
        for (x = 0 ; x < 4 ; x++)
        {
            h264_dct_quant(avctx, t->residual.part4x4Y[y][x],QPy,1,1);
        }
    }
    for (y = 0 ; y < 2 ; y++)
    {
        for (x = 0 ; x < 2 ; x++)
        {
            h264_dct_quant(avctx, t->residual.part4x4U[y][x],QPc,1,1);
            h264_dct_quant(avctx, t->residual.part4x4V[y][x],QPc,1,1);
        }
    }

    // Hadamard

    // For luma
    for (y = 0 ; y < 4 ; y++)
        for (x = 0 ; x < 4 ; x++)
            YD[y][x] = t->residual.part4x4Y[y][x][0][0];

    dsp.h264_hadamard_mult_4x4(YD);
    h264_hadamard_quant_4x4(YD,QPy);

    // For U
    for (y = 0 ; y < 2 ; y++)
        for (x = 0 ; x < 2 ; x++)
            UD[y][x] = t->residual.part4x4U[y][x][0][0];
    ff_h264_hadamard_mult_2x2(UD);
    h264_hadamard_quant_2x2(UD,QPc);

    // For V
    for (y = 0 ; y < 2 ; y++)
        for (x = 0 ; x < 2 ; x++)
            VD[y][x] = t->residual.part4x4V[y][x][0][0];
    ff_h264_hadamard_mult_2x2(VD);
    h264_hadamard_quant_2x2(VD,QPc);
    // Encode macroblock

    h264_encode_intra16x16_residual(b,YD,UD,VD,&(t->residual),lumapredmode,chromapredmode,destmb);

    // Inverse hadamard

    // For luma
    dsp.h264_hadamard_mult_4x4(YD);
    h264_hadamard_inv_quant_4x4(YD,QPy);
    for (y = 0 ; y < 4 ; y++)
        for (x = 0 ; x < 4 ; x++)
            t->residual.part4x4Y[y][x][0][0] = YD[y][x];

    // For U
    ff_h264_hadamard_mult_2x2(UD);
    ff_h264_hadamard_inv_quant_2x2(UD,QPc);
    for (y = 0 ; y < 2 ; y++)
        for (x = 0 ; x < 2 ; x++)
            t->residual.part4x4U[y][x][0][0] = UD[y][x];
    // For V
    ff_h264_hadamard_mult_2x2(VD);
    ff_h264_hadamard_inv_quant_2x2(VD,QPc);
    for (y = 0 ; y < 2 ; y++)
        for (x = 0 ; x < 2 ; x++)
            t->residual.part4x4V[y][x][0][0] = VD[y][x];

    // Inverse DCT and add

    for (y = 0 ; y < 4 ; y++)
    {
        for (x = 0 ; x < 4 ; x++)
        {
            h264_inv_quant_dct_add(t->residual.part4x4Y[y][x],QPy,1,&(destmb->Y[y*4][x*4]),t->Y_stride);
        }
    }
    for (y = 0 ; y < 2 ; y++)
    {
        for (x = 0 ; x < 2 ; x++)
        {
            h264_inv_quant_dct_add(t->residual.part4x4U[y][x],QPc,1,&(destmb->U[y*4][x*4]),t->U_stride);
            h264_inv_quant_dct_add(t->residual.part4x4V[y][x],QPc,1,&(destmb->V[y*4][x*4]),t->V_stride);
        }
    }

    destmb->available = 1;
    return 0;
}

#define H264_CODEDBLOCKPATTERN_4X4CHECK(a,b) \
    for (y = 0 ; !done && y < 4 ; y++)\
        for (x = 0 ; !done && x < 4 ; x++)\
            if (residual->part4x4Y[a][b][y][x] != 0) \
                done = 1;
#define H264_CODEDBLOCKPATTERN_8X8CHECK(i,j,shift) \
    done = 0;\
    H264_CODEDBLOCKPATTERN_4X4CHECK(i+0,j+0)\
    H264_CODEDBLOCKPATTERN_4X4CHECK(i+0,j+1)\
    H264_CODEDBLOCKPATTERN_4X4CHECK(i+1,j+0)\
    H264_CODEDBLOCKPATTERN_4X4CHECK(i+1,j+1)\
    if (done)\
        CodedBlockPatternLuma |= (1 << shift);

static void h264_encode_inter16x16_residual(H264Context *t, PutBitContext *b,int mv_x,int mv_y,int mv_x2,int mv_y2,
                         Residual *residual,
                         DCTELEM UD[2][2],DCTELEM VD[2][2],int pred_frame_index,MacroBlock *mb,
                         int last_macroblock)
{
    static const int8_t me_map[] = { 0, 2, 3, 7, 4, 8,17,13, 5,18, 9,14,10,15,16,
                             11, 1,32,33,36,34,37,44,40,35,45,38,41,39,42,
                     43,19, 6,24,25,20,26,21,46,28,27,47,22,29,23,
                     30,31,12};
    int coded_block_pattern;
    int CodedBlockPatternLuma;
    int CodedBlockPatternChroma;
    int16_t coefficients[256];
    int x,y,i,j;
    int done;
    int chromaACcount;
    int chromaDCcount;
    int nA,nB;

    // coded_block_pattern

    CodedBlockPatternLuma = 0;

    // first 8x8 block
    H264_CODEDBLOCKPATTERN_8X8CHECK(0,0,0)

    // second 8x8 block
    H264_CODEDBLOCKPATTERN_8X8CHECK(0,2,1)

    // third 8x8 block
    H264_CODEDBLOCKPATTERN_8X8CHECK(2,0,2)

    // fourth 8x8 block
    H264_CODEDBLOCKPATTERN_8X8CHECK(2,2,3)

    // check for too large values in luma
    for (y = 0 ; y < 4 ; y++)
    {
        for (x = 0 ; x < 4 ; x++)
        {
            clip_inplace(residual->part4x4Y[y][x][0][0], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][0][1], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][0][2], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][0][3], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][1][0], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][1][1], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][1][2], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][1][3], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][2][0], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][2][1], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][2][2], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][2][3], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][3][0], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][3][1], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][3][2], -2047, 2047);
            clip_inplace(residual->part4x4Y[y][x][3][3], -2047, 2047);
        }
    }

    chromaDCcount = 0;
    chromaACcount = 0;
    for (y = 0 ; y < 2 ; y++)
    {
        for (x = 0 ; x < 2 ; x++)
        {
            H264_COUNT_AND_CLIP_SUBBLOCK(residual->part4x4U[y][x],chromaACcount);
            H264_COUNT_AND_CLIP_SUBBLOCK(residual->part4x4V[y][x],chromaACcount);
        }
    }
    for (y = 0 ; y < 2 ; y++)
    {
        for (x = 0 ; x < 2 ; x++)
        {
            H264_COUNT_AND_CLIP(UD[y][x],chromaDCcount);
            H264_COUNT_AND_CLIP(VD[y][x],chromaDCcount);
        }
    }

    if (chromaACcount)
        CodedBlockPatternChroma= 2;
    else
        CodedBlockPatternChroma= !!chromaDCcount;

    if (mv_x2 == 0 && mv_y2 == 0 && CodedBlockPatternChroma == 0 && CodedBlockPatternLuma == 0) // entirely predictable
    {
        t->mb_skip_run++;
        if (last_macroblock)
            set_se_golomb(b, t->mb_skip_run);
    }
    else
    {
        set_ue_golomb(b, t->mb_skip_run); // mb_skip_run
        t->mb_skip_run = 0;

        set_ue_golomb(b, 0); // mb_type = P_L0_16x16

        // mb_pred()

        set_se_golomb(b, mv_x);
        set_se_golomb(b, mv_y);

        coded_block_pattern = (CodedBlockPatternChroma << 4)|CodedBlockPatternLuma;
        set_ue_golomb(b,me_map[coded_block_pattern]);
    }

    // residual()

    if (CodedBlockPatternLuma == 0 && CodedBlockPatternChroma == 0) // nothing left to do
    {
        memset(mb->Y_nonzero, 0, sizeof(mb->Y_nonzero));
        memset(mb->U_nonzero, 0, sizeof(mb->U_nonzero));
        memset(mb->V_nonzero, 0, sizeof(mb->V_nonzero));
        return;
    }

    set_se_golomb(b, 0); // mb_qp_delta

    // encode luma levels
    for (j = 0 ; j < 4 ; j++)
    {
        int X = (j&1) << 1;
        int Y = j&2;

        if ((CodedBlockPatternLuma >> j)&1)
        {
            for (i = 0 ; i < 4 ; i++)
            {
                int x = (i&1)+X;
                int y = (i>>1)+Y;

                int k;

                for (k = 0 ; k < 16 ; k++)
                    coefficients[k] = residual->part4x4Y[y][x][zigzagy[k]][zigzagx[k]];
                h264_neighbour_count_nonzero(mb,NEIGHBOUR_SUBTYPE_Y,x,y,&nA,&nB);
                mb->Y_nonzero[y][x] = ff_h264_cavlc_encode(b,coefficients,16,nA,nB,0);
            }
        }
        else
        {
            for (i = 0 ; i < 4 ; i++)
            {
                int x = (i&1)+X;
                int y = (i>>1)+Y;
                mb->Y_nonzero[y][x] = 0;
            }
        }
    }

    // chroma DC levels
    if (CodedBlockPatternChroma != 0)
    {
        coefficients[0] = UD[0][0];
        coefficients[1] = UD[0][1];
        coefficients[2] = UD[1][0];
        coefficients[3] = UD[1][1];
        ff_h264_cavlc_encode(b,coefficients,4,-1,-1,1); // nA and nB are not used in this case

        coefficients[0] = VD[0][0];
        coefficients[1] = VD[0][1];
        coefficients[2] = VD[1][0];
        coefficients[3] = VD[1][1];
        ff_h264_cavlc_encode(b,coefficients,4,-1,-1,1); // nA and nB are not used in this case
    }

    if (CodedBlockPatternChroma == 2)
    {
        for (i = 0 ; i < 4 ; i++)
        {
            int x = i&1;
            int y = i>>1;

            int k;

            for (k = 0 ; k < 15 ; k++)
                coefficients[k] = residual->part4x4U[y][x][zigzagy[k+1]][zigzagx[k+1]];
            h264_neighbour_count_nonzero(mb,NEIGHBOUR_SUBTYPE_U,x,y,&nA,&nB);
            mb->U_nonzero[y][x] = ff_h264_cavlc_encode(b,coefficients,15,nA,nB,0);
        }

        for (i = 0 ; i < 4 ; i++)
        {
            int x = i&1;
            int y = i>>1;

            int k;

            for (k = 0 ; k < 15 ; k++)
                coefficients[k] = residual->part4x4V[y][x][zigzagy[k+1]][zigzagx[k+1]];
            h264_neighbour_count_nonzero(mb,NEIGHBOUR_SUBTYPE_V,x,y,&nA,&nB);
            mb->V_nonzero[y][x] = ff_h264_cavlc_encode(b,coefficients,15,nA,nB,0);
        }
    }
    else
    {
        int x,y;

        for (y = 0 ; y < 2 ; y++)
        {
            for (x = 0 ; x < 2 ; x++)
            {
                mb->U_nonzero[y][x] = 0;
                mb->V_nonzero[y][x] = 0;
            }
        }
    }
}

static void h264_predict(H264Context *t, MacroBlock *destmb, FrameInfo *refframe, int mbx, int mby, int mvx, int mvy)
{
    int x = mbx << 4;
    int y = mby << 4;
    AVPicture *refpic = &(refframe->reconstructed_picture);
    uint8_t *data;
    int linesize;
    int i,j;
    int startx,starty;
    int w,h,w2,h2;
    int xmod,ymod;

    w = destmb->Y_width;
    h = destmb->Y_height;
    w2 = w>>1;
    h2 = h>>1;

    startx = x+(mvx/4);
    starty = y+(mvy/4);

    linesize = refpic->linesize[0];
    data = refpic->data[0]+starty*linesize+startx;

    for (i = 0 ; i < h ; i++)
    {
        for (j = 0 ; j < w ; j++)
            destmb->Y[i][j] = data[j];
        data += linesize;
    }

    linesize = refpic->linesize[1];
    data = refpic->data[1]+(starty/2)*linesize+startx/2;

    xmod = startx & 1;
    ymod = starty & 1;

    if (xmod == 0 && ymod == 0)
    {
        for (i = 0 ; i < h2 ; i++)
        {
            for (j = 0 ; j < w2 ; j++)
                destmb->U[i][j] = data[j];
            data += linesize;
        }

        linesize = refpic->linesize[2];
        data = refpic->data[2]+(starty/2)*linesize+startx/2;
        for (i = 0 ; i < h2 ; i++)
        {
            for (j = 0 ; j < w2 ; j++)
                destmb->V[i][j] = data[j];
            data += linesize;
        }
    }
    else if (xmod == 0 && ymod != 0)
    {
        for (i = 0 ; i < h2 ; i++)
        {
            for (j = 0 ; j < w2 ; j++)
                destmb->U[i][j] = (uint8_t)(((int)data[j]+(int)data[j+linesize]+1)/2);
            data += linesize;
        }

        linesize = refpic->linesize[2];
        data = refpic->data[2]+(starty/2)*linesize+startx/2;
        for (i = 0 ; i < h2 ; i++)
        {
            for (j = 0 ; j < w2 ; j++)
                destmb->V[i][j] = (uint8_t)(((int)data[j]+(int)data[j+linesize]+1)/2);
            data += linesize;
        }
    }
    else if (xmod != 0 && ymod == 0)
    {
        for (i = 0 ; i < h2 ; i++)
        {
            for (j = 0 ; j < w2 ; j++)
                destmb->U[i][j] = (uint8_t)(((int)data[j]+(int)data[j+1]+1)/2);
            data += linesize;
        }

        linesize = refpic->linesize[2];
        data = refpic->data[2]+(starty/2)*linesize+startx/2;
        for (i = 0 ; i < h2 ; i++)
        {
            for (j = 0 ; j < w2 ; j++)
                destmb->V[i][j] = (uint8_t)(((int)data[j]+(int)data[j+1]+1)/2);
            data += linesize;
        }
    }
    else // xmod != 0 && ymod != 0
    {
        for (i = 0 ; i < h2 ; i++)
        {
            for (j = 0 ; j < w2 ; j++)
                destmb->U[i][j] = (uint8_t)(((int)data[j]+(int)data[j+1]+(int)data[j+linesize+1]+(int)data[j+linesize]+2)/4);
            data += linesize;
        }

        linesize = refpic->linesize[2];
        data = refpic->data[2]+(starty/2)*linesize+startx/2;
        for (i = 0 ; i < h2 ; i++)
        {
            for (j = 0 ; j < w2 ; j++)
                destmb->V[i][j] = (uint8_t)(((int)data[j]+(int)data[j+1]+(int)data[j+linesize+1]+(int)data[j+linesize]+2)/4);
            data += linesize;
        }
    }
}

#define MAXSEARCHSTEPS 8
#define SEARCHWIDTH 4

/* According to p. 253 and 254 the horizontal motion vectors should be in the
 * range of [-2048, 2047.75] in units of luma samples.
 * The vertical motion vector ranges depend on the level and are specified in
 * table A-1.
 */
static void h264_find_motion_vector_and_prediction(H264Context *t, MacroBlock *targetmb, FrameInfo *refframe,
                                                 int mbx, int mby, int *mvx, int *mvy,
                             int pred_mvx, int pred_mvy, MacroBlock *destmb)
{
    int x = mbx << 4;
    int y = mby << 4;
    int bestx, besty;
    int curx, cury;
    int minbitsize = INT_MAX;
    int QP = t->QP;
    int done = 0;
    int numsteps = 0;

#if 0
    /* Force motion vector (0,0) */
    *mvx = 0;
    *mvy = 0;
    h264_predict(t, destmb, refframe, mbx, mby, *mvx, *mvy);
    return;
#endif

    bestx = x;
    besty = y;
    curx = x;
    cury = y;

    {
        int scanx = x;
        int scany = y;
        int weight;
        int xvec = -pred_mvx; // it's actually this difference which will be encoded!
        int yvec = -pred_mvy;
        int sae = t->dspcontext.pix_abs[0][0](0,targetmb->Y[0],
            refframe->reconstructed_picture.data[0]    + scany * refframe->reconstructed_picture.linesize[0] + scanx,
            refframe->reconstructed_picture.linesize[0], 16);
        sae += t->dspcontext.pix_abs[1][0](0,targetmb->U[0],
            refframe->reconstructed_picture.data[1]    + (scany/2) * refframe->reconstructed_picture.linesize[1] + scanx/2,
            refframe->reconstructed_picture.linesize[1], 8);
        sae += t->dspcontext.pix_abs[1][0](0,targetmb->V[0],
            refframe->reconstructed_picture.data[2]    + (scany/2) * refframe->reconstructed_picture.linesize[2] + scanx/2,
            refframe->reconstructed_picture.linesize[2], 8);
        sae = FFMIN(sae>>4, 2047);
        minbitsize = mv_len_table[xvec+MVTABLE_OFFSET]
            + mv_len_table[yvec+MVTABLE_OFFSET];
        weight = sae_codeblocksize_relation[QP>>2][sae>>8];
        weight += (sae_codeblocksize_relation[QP>>2][FFMIN(((sae>>8)+1), 8)]
            - sae_codeblocksize_relation[QP>>2][sae>>8] )
            * (sae -  ((sae>>8) << 8)) / ( (FFMIN(((sae>>8)+1), 8) << 8)
            -  ((sae>>8) << 8) );
        minbitsize += weight;
    }

    while (!done && numsteps < MAXSEARCHSTEPS)
    {
        int startx = curx - SEARCHWIDTH;
        int starty = cury - SEARCHWIDTH;
        int stopx = curx + SEARCHWIDTH + 1;
        int stopy = cury + SEARCHWIDTH + 1;
        int foundbetter = 0;
        int scanx, scany;

        if (startx < 0)
            startx = 0;
        if (starty < 0)
            starty = 0;
        if (stopx > t->refframe_width - 16 + 1)
            stopx = t->refframe_width - 16 + 1;
        if (stopy > t->refframe_height - 16 + 1)
            stopy = t->refframe_height -16 + 1;

        for(scany = starty; scany < stopy; scany++)
        {
            for(scanx = startx; scanx < stopx; scanx++)
            {
                if (!(curx == scanx && cury == scany))
                {
                    int xvec = (scanx-x)*4-pred_mvx; // it's actually this difference which will be encoded!
                    int yvec = (scany-y)*4-pred_mvy;
                    int bitsize;
                    int weight;
                    int xmod = scanx%2;
                    int ymod = scany%2;
                    int absnum = xmod+ymod*2;
                    int sae = t->dspcontext.pix_abs[0][0](0,targetmb->Y[0],
                        refframe->reconstructed_picture.data[0]    + scany * refframe->reconstructed_picture.linesize[0] + scanx,
                        refframe->reconstructed_picture.linesize[0], 16);

                    sae += t->dspcontext.pix_abs[1][absnum](0,targetmb->U[0],
                        refframe->reconstructed_picture.data[1]    + (scany/2) * refframe->reconstructed_picture.linesize[1] + scanx/2,
                        refframe->reconstructed_picture.linesize[1], 8);
                    sae += t->dspcontext.pix_abs[1][absnum](0,targetmb->V[0],
                        refframe->reconstructed_picture.data[2]    + (scany/2) * refframe->reconstructed_picture.linesize[2] + scanx/2,
                        refframe->reconstructed_picture.linesize[2], 8);
                    sae = FFMIN(sae>>4, 2047);
                    bitsize = mv_len_table[xvec+MVTABLE_OFFSET]
                        + mv_len_table[yvec+MVTABLE_OFFSET];
                    weight = sae_codeblocksize_relation[QP>>2][sae>>8];
                    weight += (sae_codeblocksize_relation[QP>>2][FFMIN(((sae>>8)+1), 8)]
                        - sae_codeblocksize_relation[QP>>2][sae>>8] )
                        * (sae -  ((sae>>8) << 8)) / ( (FFMIN(((sae>>8)+1), 8) << 8)
                        -  ((sae>>8) << 8) );
                    bitsize += weight;
                    if (bitsize < minbitsize)
                    {
                        minbitsize = bitsize;
                        bestx = scanx;
                        besty = scany;
                        foundbetter = 1;
                    }
                }
            }
        }

        if (foundbetter)
        {
            curx = bestx;
            cury = besty;
            numsteps++;
        }
        else
            done = 1;
    }
    {
        int mvx = (bestx - x) * 4;
        int mvy = (besty - y) * 4;
#ifdef DEBUG_DETERMINE_MAX_MV
        if (mvx > max_mvx)
            max_mvx = mvx;
        if (mvy > max_mvy)
            max_mvy = mvy;
        if (mvx < min_mvx)
            min_mvx = mvx;
        if (mvy < min_mvy)
            min_mvy = mvy;
#endif
        h264_predict(t, destmb, refframe, mbx, mby, mvx, mvy);
    }

    *mvx = (bestx - x) * 4;
    *mvy = (besty - y) * 4;
}

// Adjust the values of mvx and mvy based on the prediction from the neighbouring macroblocks
static void h264_estimate_motion_vectors(MacroBlock *destmb, int *mvpred_x, int *mvpred_y, int *mvpred_x2, int *mvpred_y2)
{
    int mvAx = 0, mvAy = 0;
    int mvBx = 0, mvBy = 0;
    int mvCx = 0, mvCy = 0;
    int mvDx = 0, mvDy = 0;
    int Aavail = 0;
    int Bavail = 0;
    int Cavail = 0;
    int Davail = 0;

    if (destmb->leftblock != NULL && destmb->leftblock->available)
    {
        Aavail = 1;
        mvAx = destmb->leftblock->mv_x;
        mvAy = destmb->leftblock->mv_y;
    }
    if (destmb->topblock != NULL)
    {
        MacroBlock *topblock = destmb->topblock;

        if (topblock->available)
        {
            Bavail = 1;
            mvBx = topblock->mv_x;
            mvBy = topblock->mv_y;
        }
        if (topblock->leftblock != NULL && topblock->leftblock->available)
        {
            Davail = 1;
            mvDx = topblock->leftblock->mv_x;
            mvDy = topblock->leftblock->mv_y;
        }
        if (topblock->rightblock != NULL && topblock->rightblock->available)
        {
            Cavail = 1;
            mvCx = topblock->rightblock->mv_x;
            mvCy = topblock->rightblock->mv_y;
        }
    }

    if (!Cavail)
    {
        Cavail = Davail;
        mvCx = mvDx;
        mvCy = mvDy;
    }

    if (!Bavail && !Cavail && Aavail)
    {
        mvBx = mvAx;
        mvBy = mvAy;
        mvCx = mvAx;
        mvCy = mvAy;
    }

    *mvpred_x = mid_pred(mvAx,mvBx,mvCx);
    *mvpred_y = mid_pred(mvAy,mvBy,mvCy);

    if (!Aavail || !Bavail || (Aavail && mvAx == 0 && mvAy == 0) || (Bavail && mvBx == 0 && mvBy == 0))
    {
        *mvpred_x2 = 0;
        *mvpred_y2 = 0;
    }
    else
    {
        *mvpred_x2 = *mvpred_x;
        *mvpred_y2 = *mvpred_y;
    }
}

static void h264_encode_inter_16x16(AVCodecContext *avctx, MacroBlock *targetmb, PutBitContext *b,
                                 MacroBlock *destmb, FrameInfo **previous_frames,
                     int num_prev_frames, int mbx, int mby)
{
    H264Context *t = (H264Context *)avctx->priv_data;
    int y,h,x,w;
    int w2,h2;
    int qPI;
    int QPc;
    int QPy = t->QP;
    DCTELEM UD[2][2];
    DCTELEM VD[2][2];
    int mvx = 0;
    int mvy = 0;
    int pred_mvx = 0;
    int pred_mvy = 0;
    int pred_mvx2 = 0;
    int pred_mvy2 = 0;

    qPI = t->QP + t->chroma_qp_index_offset;
    qPI = av_clip(qPI, 0, 51);
    QPc = chroma_qp[qPI];

    w = targetmb->Y_width;
    h = targetmb->Y_height;
    w2 = w>>1;
    h2 = h>>1;

    // Find motion vector and prediction

    h264_estimate_motion_vectors(destmb, &pred_mvx, &pred_mvy, &pred_mvx2, &pred_mvy2);
    h264_find_motion_vector_and_prediction(t, targetmb, previous_frames[0], mbx, mby, &mvx, &mvy,
                                         pred_mvx, pred_mvy, destmb);

    // Calculate residual

    H264_COPY_16X16BLOCK(t->residual.part4x4Y,(int16_t)targetmb->Y,(int16_t)destmb->Y);
    H264_COPY_8X8BLOCK(t->residual.part4x4U,(int16_t)targetmb->U,(int16_t)destmb->U);
    H264_COPY_8X8BLOCK(t->residual.part4x4V,(int16_t)targetmb->V,(int16_t)destmb->V);

    // Transform residual: DCT

    for (y = 0 ; y < 4 ; y++)
    {
        for (x = 0 ; x < 4 ; x++)
        {
            h264_dct_quant(avctx, t->residual.part4x4Y[y][x],QPy,0,0);
        }
    }
    for (y = 0 ; y < 2 ; y++)
    {
        for (x = 0 ; x < 2 ; x++)
        {
            h264_dct_quant(avctx, t->residual.part4x4U[y][x],QPc,1,0);
            h264_dct_quant(avctx, t->residual.part4x4V[y][x],QPc,1,0);
        }
    }
    // For U
    for (y = 0 ; y < 2 ; y++)
        for (x = 0 ; x < 2 ; x++)
            UD[y][x] = t->residual.part4x4U[y][x][0][0];
    ff_h264_hadamard_mult_2x2(UD);
    h264_hadamard_quant_2x2(UD, QPc);

    // For V
    for (y = 0 ; y < 2 ; y++)
        for (x = 0 ; x < 2 ; x++)
            VD[y][x] = t->residual.part4x4V[y][x][0][0];
    ff_h264_hadamard_mult_2x2(VD);
    h264_hadamard_quant_2x2(VD,QPc);

    // Encode motion vectors, residual, ...

    destmb->mv_x = mvx;
    destmb->mv_y = mvy;

    h264_encode_inter16x16_residual(t, b, mvx-pred_mvx, mvy-pred_mvy, mvx-pred_mvx2, mvy-pred_mvy2,
                                  &(t->residual), UD, VD, 0, destmb, (mbx == t->mb_width-1 && mby == t->mb_height-1));

    // Inverse hadamard

    // For U
    ff_h264_hadamard_mult_2x2(UD);
    ff_h264_hadamard_inv_quant_2x2(UD,QPc);
    for (y = 0 ; y < 2 ; y++)
        for (x = 0 ; x < 2 ; x++)
            t->residual.part4x4U[y][x][0][0] = UD[y][x];
    // For V
    ff_h264_hadamard_mult_2x2(VD);
    ff_h264_hadamard_inv_quant_2x2(VD,QPc);
    for (y = 0 ; y < 2 ; y++)
        for (x = 0 ; x < 2 ; x++)
            t->residual.part4x4V[y][x][0][0] = VD[y][x];

    // Inverse DCT and add

    for (y = 0 ; y < 4 ; y++)
    {
        for (x = 0 ; x < 4 ; x++)
        {
            h264_inv_quant_dct_add(t->residual.part4x4Y[y][x],QPy,0,&(destmb->Y[y*4][x*4]),t->Y_stride);
        }
    }
    for (y = 0 ; y < 2 ; y++)
    {
        for (x = 0 ; x < 2 ; x++)
        {
            h264_inv_quant_dct_add(t->residual.part4x4U[y][x],QPc,1,&(destmb->U[y*4][x*4]),t->V_stride);
            h264_inv_quant_dct_add(t->residual.part4x4V[y][x],QPc,1,&(destmb->V[y*4][x*4]),t->U_stride);
        }
    }

    destmb->available = 1;
}

static void h264_control_bitrate(AVCodecContext *avctx)
{
    H264Context *t = (H264Context *)avctx->priv_data;
    if (t->blocksize_history_total_milliseconds)
    {
        int64_t bitrate = (t->blocksize_history_sum*1000)/t->blocksize_history_total_milliseconds;

        if (avctx->bit_rate > bitrate) // increase quality
        {
            if ((t->QP > 0) && (t->QP > avctx->qmin))
                t->QP--;
        }
        else // decrease quality
        {
            if ((t->QP < 51) && (t->QP < avctx->qmax))
                t->QP++;
        }
    }
}

static inline void h264_generate_slice_header(AVCodecContext *avctx, PutBitContext *b, int isIDR, int first_mb_in_slice)
{
    H264Context *t = (H264Context *)avctx->priv_data;

    // Slice header
    set_ue_golomb(b, first_mb_in_slice); // first_mb_in_slice

    if (isIDR)
        set_ue_golomb(b, 7); // slice_type
    else
        set_ue_golomb(b, 5); // slice_type
    // 0: current slice is P-slice
    // 2: current slice is I-slice
    // 5: current and all other slices are P-slices (0 or 5)
    // 7: current and all other slices are I-slices (2 or 7)

    set_ue_golomb(b, 0); // pic_parameter_set_id
    put_bits(b, t->log2_max_frame_num_minus4 + 4, t->frame_num % avctx->gop_size); // frame_num

    if (isIDR)
        set_ue_golomb(b, t->idr_pic_id); // idr_pic_id
    else
        put_bits(b, 1, 0); // num_ref_idx_active_override_flag

    // dec_ref_pic_marking() ...
    put_bits(b, 1, 0); // no_output_of_prior_pics_flag
    put_bits(b, 1, 0); // long_term_reference_flag
    // ... dec_ref_pic_marking()
    set_se_golomb(b, t->QP - t->PPS_QP); // slice_qp_delta
}

static int h264_encode(AVCodecContext *avctx, uint8_t *buf, int buf_size, void *data)
{
    H264Context *t = (H264Context *)avctx->priv_data;
    PutBitContext b;
    int mbx, mby;
    uint8_t *dest;
    int destlen, i;
    FrameInfo *tmp;
    int QPy, QPc, qPI, isIDR = 0;

    if (t->frame_num % avctx->gop_size == 0)
        isIDR = 1;

    destlen = t->bufsize;
    dest = t->po_data0;

    // Copy the input image. Macroblocks were already assigned in the initialization step
    av_picture_copy(&(t->input_frame_copy),(AVPicture *)data,PIX_FMT_YUV420P,avctx->width,avctx->height);

    // reconstructed_frames[0] will be used to reconstruct the image
    h264_clear_nonzero_markers(t->reconstructed_frames[0]->reconstructed_mb_map,t->mb_width,t->mb_height);

    if (isIDR && !(avctx->flags & CODEC_FLAG_GLOBAL_HEADER))
    {
        int res;

        res = h264_generate_global_headers(avctx, dest, destlen);
        dest += (destlen - res);
        destlen = res;
    }

    // IDR slice or P slice

    init_put_bits(&b,t->pi_data0,t->bufsize);

    h264_generate_slice_header(avctx, &b, isIDR, 0);


#ifdef DISABLE_DEBLOCKING
    if (!(avctx->flags & CODEC_FLAG_LOOP_FILTER))
        set_ue_golomb(&b, 1); // disable_deblocking_filter_idc
#endif /* DISABLE_DEBLOCKING */

    // Slice data
    if (isIDR)
    {
        t->po.pict_type = FF_I_TYPE;
        t->po.key_frame = 1;
        for(mby = 0; mby < t->mb_height; mby++)
            for(mbx = 0; mbx < t->mb_width; mbx++)
                if (h264_encode_intra_16x16(avctx,&(t->mb_map[mby][mbx]),&b,&(t->reconstructed_frames[0]->reconstructed_mb_map[mby][mbx]))<0)
                    h264_encode_i_pcm(&(t->mb_map[mby][mbx]),&b,&(t->reconstructed_frames[0]->reconstructed_mb_map[mby][mbx]));
    }
    else // Inter encoded frame
    {
        t->mb_skip_run = 0;
        t->po.pict_type = FF_P_TYPE;

        for(mby = 0; mby < t->mb_height ; mby++)
            for(mbx = 0 ; mbx < t->mb_width ; mbx++)
                h264_encode_inter_16x16(avctx,&(t->mb_map[mby][mbx]),&b,&(t->reconstructed_frames[0]->reconstructed_mb_map[mby][mbx]),&(t->reconstructed_frames[1]),t->framebufsize-1,mbx,mby);
    }

    if (avctx->flags & CODEC_FLAG_PSNR)
    {
        int plane_index;
        for (plane_index=0; plane_index<3; plane_index++)
        {
            int64_t error = 0;
            int x, y, f;
            if (plane_index==0)
                f = 1;
            else
                f = 2;
            for (y=0; y < t->mb_height*16/f; y++)
            {
                for (x=0; x < t->mb_width*16/f; x++)
                {
                    int d = t->reconstructed_frames[0]->reconstructed_picture.data[plane_index][y*t->reconstructed_frames[0]->reconstructed_picture.linesize[plane_index] + x] - t->input_frame_copy.data[plane_index][y*t->input_frame_copy.linesize[plane_index] + x];
                    error += d*d;
                }
            }
            avctx->error[plane_index] += error;
            t->po.error[plane_index] = error;
        }
    }

#ifndef DISABLE_DEBLOCKING
    if (avctx->flags & CODEC_FLAG_LOOP_FILTER)
    {
        QPy = t->QP;

        qPI = t->QP + t->chroma_qp_index_offset;
        qPI = av_clip(qPI, 0, 51);
        QPc = chroma_qp[qPI];

        ff_h264_deblock(t,t->reconstructed_frames[0],isIDR,QPy,QPc);
    }
#endif /* DISABLE_DEBLOCKING */

    if (isIDR)
        dest = h264_write_nal_unit(avctx, 3,NAL_IDR_SLICE,dest,&destlen,&b);
    else
        dest = h264_write_nal_unit(avctx, 2,NAL_SLICE,dest,&destlen,&b);

#ifdef DEBUG_WRITE_DECODED_IMAGE
    h264_append_image(t->reconstructed_frames[0]->reconstructed_picture.data[0], avctx);
#endif

    // cycle frame buffer

    tmp = t->reconstructed_frames[t->framebufsize-1];
    for (i = t->framebufsize-1 ; i > 0 ; i--)
        t->reconstructed_frames[i] = t->reconstructed_frames[i-1];
    t->reconstructed_frames[0] = tmp;

    // copy the encoded bytes
    memcpy(buf,t->po_data0,t->bufsize-destlen);

    // update history information
    t->blocksize_history_sum -= t->blocksize_history[t->blocksize_history_pos];
    t->blocksize_history_sum += (t->bufsize-destlen)*8;
    t->blocksize_history[t->blocksize_history_pos] = (t->bufsize-destlen)*8;

    t->blocksize_history_pos++;
    if (t->blocksize_history_pos == t->blocksize_history_length)
        t->blocksize_history_pos = 0;
    if (t->blocksize_history_num_filled < t->blocksize_history_length)
    {
        t->blocksize_history_num_filled++;
        t->blocksize_history_total_milliseconds += t->milliseconds_per_frame;
    }

    if (!t->use_fixed_qp)
        h264_control_bitrate(avctx);

    // adjust frame numbers
    t->frame_counter++;
    t->frame_num++;
//    t->frame_num %= t->max_frame_num;

    if (isIDR)
        t->idr_pic_id++;

    // set this here as we use it in the callback
    avctx->coded_frame = &t->po;
    avctx->coded_frame->pts = ((AVFrame *)data)->pts;
    avctx->coded_frame->quality = t->QP * FF_QP2LAMBDA;
    avctx->frame_bits = (t->bufsize-destlen)<<3;

    return (t->bufsize-destlen);
}

static int h264_encoder_close(AVCodecContext *avctx)
{
    PutBitContext b;
    H264Context *t = (H264Context *)avctx->priv_data;
    uint8_t *dest;
    int destlen;
    int y,i;

    destlen = t->bufsize;
    dest = t->po_data0;

    init_put_bits(&b,t->pi_data0,t->bufsize);

    // write end of stream

    dest = h264_write_nal_unit(avctx, 0,NAL_END_STREAM,dest,&destlen,&b);

    *dest = 0;
    dest++;
    destlen--;

    // clean up

    avpicture_free((AVPicture*)&t->pi);
    avpicture_free((AVPicture*)&t->po);

    for (y = 0 ; y < t->mb_height ; y++)
        av_free(t->mb_map[y]);

    av_free(t->mb_map);

    for (i = 0 ; i < t->framebufsize ; i++)
    {
        av_free(t->reconstructed_frames[i]->reconstructed_picture.data[0]);

        for (y = 0 ; y < t->mb_height ; y++)
            av_free(t->reconstructed_frames[i]->reconstructed_mb_map[y]);

        av_free(t->reconstructed_frames[i]->reconstructed_mb_map);
        av_free(t->reconstructed_frames[i]);
    }

    av_free(t->reconstructed_frames);

    av_free(t->input_frame_copy.data[0]);

    av_free(t->blocksize_history);

    //av_free(t->rtp_buffer);
    av_free(rtp_buffer);

#ifdef DEBUG_DETERMINE_MAX_MV
    av_log(avctx, AV_LOG_ERROR, "Motion vector range x: [%d, %d] y: [%d, %d]\n", min_mvx, max_mvx, min_mvy, max_mvy);
#endif
    return 0;
}

#ifdef CONFIG_ENCODERS
AVCodec h264_encoder = {
    "ffh264",
    CODEC_TYPE_VIDEO,
    CODEC_ID_FFH264,
    sizeof(H264Context),
    h264_encoder_init,
    h264_encode,
    h264_encoder_close,
    .pix_fmts= (enum PixelFormat[]){PIX_FMT_YUV420P, -1},
};
#endif
