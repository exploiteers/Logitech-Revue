#ifndef DM365_FACEDETECT_HW
#define DM365_FACEDETECT_HW

#define FACEDETECT_IOBASE_ADDR  (0x01c71800)

/* Macro for bit set and clear */
#define SETBIT(reg, bit)   (reg = ((reg) | ((0x001)<<(bit))))
#define RESETBIT(reg, bit) (reg = ((reg) & (~(0x001<<(bit)))))

/* Register read/write */
#define regw_if(val, reg)    davinci_writel(val, FACEDETECT_IOBASE_ADDR + reg)
#define regr_if(reg)         davinci_readl(reg + FACEDETECT_IOBASE_ADDR)

/* Control register bits */
#define SRST_CONTROL   0
#define RUN_CONTROL    1
#define FINISH_CONTROL 2

#define FACE_DETECT_SIZE       0x07F
#define FACE_DETECT_CONFIDENCE 0xF00
#define OUTPUT_RESULT_OFFSET   16

/* Interface control registers */
#define FDIF_PID        0x000
#define FDIF_CTRL       0x004
#define FDIF_INTEN      0x008
#define FDIF_PICADDR    0x00C
#define FDIF_WKADDR     0x010
#define FDIF_TESTMON    0x014

/* Control registers */
#define FD_CTRL         0x020
#define FD_DNUM         0x024
#define FD_DCOND        0x028
#define FD_STARTX       0x02C
#define FD_STARTY       0x030
#define FD_SIZEX        0x034
#define FD_SIZEY        0x038
#define FD_LHIT         0x03C

/* Detection result for face 1 */
#define FD_CENTERX1     0x100
#define FD_CENTERY1     0x104
#define FD_CONFSIZE1    0x108
#define FD_ANGLE1       0x10C

/* Detection result for face 2 */
#define FD_CENTERX2     0x110
#define FD_CENTERY2     0x114
#define FD_CONFSIZE2    0x118
#define FD_ANGLE2       0x11C

/* Detection result for face 3 */
#define FD_CENTERX3     0x120
#define FD_CENTERY3     0x124
#define FD_CONFSIZE3    0x128
#define FD_ANGLE3       0x12C

/* Detection result for face 4 */
#define FD_CENTERX4     0x130
#define FD_CENTERY4     0x134
#define FD_CONFSIZE4    0x138
#define FD_ANGLE4       0x13C

/* Detection result for face 5 */
#define FD_CENTERX5     0x140
#define FD_CENTERY5     0x144
#define FD_CONFSIZE5    0x148
#define FD_ANGLE5       0x14C

/* Detection result for face 6 */
#define FD_CENTERX6     0x150
#define FD_CENTERY6     0x154
#define FD_CONFSIZE6    0x158
#define FD_ANGLE6       0x15C

/* Detection result for face 7 */
#define FD_CENTERX7     0x160
#define FD_CENTERY7     0x164
#define FD_CONFSIZE7    0x168
#define FD_ANGLE7       0x16C

/* Detection result for face 8 */
#define FD_CENTERX8     0x170
#define FD_CENTERY8     0x174
#define FD_CONFSIZE8    0x178
#define FD_ANGLE8       0x17C

/* Detection result for face 9 */
#define FD_CENTERX9     0x180
#define FD_CENTERY9     0x184
#define FD_CONFSIZE9    0x188
#define FD_ANGLE9       0x18C

/* Detection result for face 10 */
#define FD_CENTERX10    0x190
#define FD_CENTERY10    0x194
#define FD_CONFSIZE10   0x198
#define FD_ANGLE10      0x19C

/* Detection result for face 11 */
#define FD_CENTERX11    0x1A0
#define FD_CENTERY11    0x1A4
#define FD_CONFSIZE11   0x1A8
#define FD_ANGLE11      0x1AC

/* Detection result for face 12 */
#define FD_CENTERX12    0x1B0
#define FD_CENTERY12    0x1B4
#define FD_CONFSIZE12   0x1B8
#define FD_ANGLE12      0x1BC

/* Detection result for face 13 */
#define FD_CENTERX13    0x1C0
#define FD_CENTERY13    0x1C4
#define FD_CONFSIZE13   0x1C8
#define FD_ANGLE13      0x1CC

/* Detection result for face 14 */
#define FD_CENTERX14    0x1D0
#define FD_CENTERY14    0x1D4
#define FD_CONFSIZE14   0x1D8
#define FD_ANGLE14      0x1DC

/* Detection result for face 15 */
#define FD_CENTERX15    0x1E0
#define FD_CENTERY15    0x1E4
#define FD_CONFSIZE15   0x1E8
#define FD_ANGLE15      0x1EC

/* Detection result for face 16 */
#define FD_CENTERX16    0x1F0
#define FD_CENTERY16    0x1F4
#define FD_CONFSIZE16   0x1F8
#define FD_ANGLE16      0x1FC

/* Detection result for face 17 */
#define FD_CENTERX17    0x200
#define FD_CENTERY17    0x204
#define FD_CONFSIZE17   0x208
#define FD_ANGLE17      0x20C

/* Detection result for face 18 */
#define FD_CENTERX18    0x210
#define FD_CENTERY18    0x214
#define FD_CONFSIZE18   0x218
#define FD_ANGLE18      0x21C

/* Detection result for face 19 */
#define FD_CENTERX19    0x220
#define FD_CENTERY19    0x224
#define FD_CONFSIZE19   0x228
#define FD_ANGLE19      0x22C

/* Detection result for face 20 */
#define FD_CENTERX20    0x230
#define FD_CENTERY20    0x234
#define FD_CONFSIZE20   0x238
#define FD_ANGLE20      0x23C

/* Detection result for face 21 */
#define FD_CENTERX21    0x240
#define FD_CENTERY21    0x244
#define FD_CONFSIZE21   0x248
#define FD_ANGLE21      0x24C

/* Detection result for face 22 */
#define FD_CENTERX22    0x250
#define FD_CENTERY22    0x254
#define FD_CONFSIZE22   0x258
#define FD_ANGLE22      0x25C

/* Detection result for face 23 */
#define FD_CENTERX23    0x260
#define FD_CENTERY23    0x264
#define FD_CONFSIZE23   0x268
#define FD_ANGLE23      0x26C

/* Detection result for face 24 */
#define FD_CENTERX24    0x270
#define FD_CENTERY24    0x274
#define FD_CONFSIZE24   0x278
#define FD_ANGLE24      0x27C

/* Detection result for face 25 */
#define FD_CENTERX25    0x280
#define FD_CENTERY25    0x284
#define FD_CONFSIZE25   0x288
#define FD_ANGLE25      0x28C

/* Detection result for face 26 */
#define FD_CENTERX26    0x290
#define FD_CENTERY26    0x294
#define FD_CONFSIZE26   0x298
#define FD_ANGLE26      0x29C

/* Detection result for face 27 */
#define FD_CENTERX27    0x2A0
#define FD_CENTERY27    0x2A4
#define FD_CONFSIZE27   0x2A8
#define FD_ANGLE27      0x2AC

/* Detection result for face 28 */
#define FD_CENTERX28    0x2B0
#define FD_CENTERY28    0x2B4
#define FD_CONFSIZE28   0x2B8
#define FD_ANGLE28      0x2BC

/* Detection result for face 29 */
#define FD_CENTERX29    0x2C0
#define FD_CENTERY29    0x2C4
#define FD_CONFSIZE29   0x2C8
#define FD_ANGLE29      0x2CC

/* Detection result for face 30 */
#define FD_CENTERX30    0x2D0
#define FD_CENTERY30    0x2D4
#define FD_CONFSIZE30   0x2D8
#define FD_ANGLE30      0x2DC

/* Detection result for face 31 */
#define FD_CENTERX31    0x2E0
#define FD_CENTERY31    0x2E4
#define FD_CONFSIZE31   0x2E8
#define FD_ANGLE31      0x2EC

/* Detection result for face 32 */
#define FD_CENTERX32    0x2F0
#define FD_CENTERY32    0x2F4
#define FD_CONFSIZE32   0x2F8
#define FD_ANGLE32      0x2FC

/* Detection result for face 33 */
#define FD_CENTERX33    0x300
#define FD_CENTERY33    0x304
#define FD_CONFSIZE33   0x308
#define FD_ANGLE33      0x30C

/* Detection result for face 34 */
#define FD_CENTERX34    0x310
#define FD_CENTERY34    0x314
#define FD_CONFSIZE34   0x318
#define FD_ANGLE34      0x31C

/* Detection result for face 35 */
#define FD_CENTERX35    0x320
#define FD_CENTERY35    0x324
#define FD_CONFSIZE35   0x328
#define FD_ANGLE35      0x32C

int facedetect_init_hw_setup(void);
int facedetect_set_hw_param(struct facedetect_params_t *);
int facedetect_get_hw_param(struct facedetect_params_t *);
int facedetect_execute(struct facedetect_params_t *);
int facedetect_set_buffer(struct facedetect_params_t *);

int facedetect_get_status(char);
int facedetect_int_wait(char);
int facedetect_int_enable(char);
int facedetect_isbusy(char *);
int facedetect_set_detect_direction(struct facedetect_params_t *);
int facedetect_set_minimum_face_size(struct facedetect_params_t *);
int facedetect_set_start_x(struct facedetect_params_t *);
int facedetect_set_start_y(struct facedetect_params_t *);
int facedetect_set_detection_width(struct facedetect_params_t *);
int facedetect_set_detection_height(struct facedetect_params_t *);
int facedetect_set_threshold(struct facedetect_params_t *);
int facedetect_init_interrupt(void);
int facedetect_int_clear(char);
int facedetect_int_wait(char);

#endif /* DM365_FACEDETECT_HW */
