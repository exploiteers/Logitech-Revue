/*
      Copyright (C) 2008  Reimar Döffinger

      Permission is hereby granted, free of charge, to any person
      obtaining a copy of this software and associated documentation
      files (the "Software"), to deal in the Software without
      restriction, including without limitation the rights to use, copy,
      modify, merge, publish, distribute, sublicense, and/or sell copies
      of the Software, and to permit persons to whom the Software is
      furnished to do so, subject to the following conditions:

      The above copyright notice and this permission notice shall be
      included in all copies or substantial portions of the Software.

      THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
      EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
      MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
      NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
      HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
      WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
      OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
      DEALINGS IN THE SOFTWARE.
**/

#include <stdlib.h>
#include "libavutil/bswap.h"
#include "libavutil/avstring.h"
#include "libavcodec/get_bits.h"
#include "libavcodec/bytestream.h"
#include "avformat.h"
#include "oggdec.h"

static int speex_header(AVFormatContext *s, int idx) {
    struct ogg *ogg = s->priv_data;
    struct ogg_stream *os = ogg->streams + idx;
    AVStream *st = s->streams[idx];
    uint8_t *p = os->buf + os->pstart;

    if (os->seq > 1)
        return 0;

    if (os->seq == 0) {
        int frames_per_packet;
        st->codec->codec_type = CODEC_TYPE_AUDIO;
        st->codec->codec_id = CODEC_ID_SPEEX;

        st->codec->sample_rate = AV_RL32(p + 36);
        st->codec->channels = AV_RL32(p + 48);

        /* We treat the whole Speex packet as a single frame everywhere Speex
           is handled in FFmpeg.  This avoids the complexities of splitting
           and joining individual Speex frames, which are not always
           byte-aligned. */
        st->codec->frame_size = AV_RL32(p + 56);
        frames_per_packet     = AV_RL32(p + 64);
        if (frames_per_packet)
            st->codec->frame_size *= frames_per_packet;

        st->codec->extradata_size = os->psize;
        st->codec->extradata = av_malloc(st->codec->extradata_size
                                         + FF_INPUT_BUFFER_PADDING_SIZE);
        memcpy(st->codec->extradata, p, st->codec->extradata_size);

        st->time_base.num = 1;
        st->time_base.den = st->codec->sample_rate;
    } else
        vorbis_comment(s, p, os->psize);

    return 1;
}

const struct ogg_codec ff_speex_codec = {
    .magic = "Speex   ",
    .magicsize = 8,
    .header = speex_header
};
