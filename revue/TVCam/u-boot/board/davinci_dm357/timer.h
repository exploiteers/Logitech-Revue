/*
 *
 *  Copyright (C) 2004 Texas Instruments.
 *
 *  This program is free software; you can redistribute  it and/or modify it
 *  under  the terms of  the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the  License, or (at your
 *  option) any later version.
 *
 *  THIS  SOFTWARE  IS PROVIDED   ``AS  IS'' AND   ANY  EXPRESS OR IMPLIED
 *  WARRANTIES,   INCLUDING, BUT NOT  LIMITED  TO, THE IMPLIED WARRANTIES OF
 *  MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN
 *  NO  EVENT  SHALL   THE AUTHOR  BE    LIABLE FOR ANY   DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 *  NOT LIMITED   TO, PROCUREMENT OF  SUBSTITUTE GOODS  OR SERVICES; LOSS OF
 *  USE, DATA,  OR PROFITS; OR  BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
 *  ANY THEORY OF LIABILITY, WHETHER IN  CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 *  THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *
 * Modifications:
 * ver. 1.0: Oct 2005, Swaminathan S
 *
 */
#ifndef __TIMER_H__
#define __TIMER_H__

typedef volatile struct davinci_timer_reg_t
{
    unsigned int pid12;         /* 0x0 */
    unsigned int emumgt_clksped;/* 0x4 */
    unsigned int gpint_en;      /* 0x8 */
    unsigned int gpdir_dat;     /* 0xC */
    unsigned int tim12;         /* 0x10 */
    unsigned int tim34;         /* 0x14 */
    unsigned int prd12;         /* 0x18 */
    unsigned int prd34;         /* 0x1C */
    unsigned int tcr;           /* 0x20 */
    unsigned int tgcr;          /* 0x24 */
    unsigned int wdtcr;         /* 0x28 */
    unsigned int tlgc;          /* 0x2C */
    unsigned int tlmr;          /* 0x30 */
} davinci_timer_reg;

#endif  /* __TIMER_H__ */


