#ifndef IR_H
#define IR_H

// IR driver - copied from LG Titania3 board

#define REG_IR_BASE             0x1F007B00
///////////////////////////////////////////////////////////////////////////////
#define REG_IR_CTRL             (0x0000)
    #define IR_SEPR_EN              0x0200
    #define IR_TIMEOUT_CHK_EN       0x0100
    #define IR_INV                  0x80
    #define IR_INT_MASK             0x40
    #define IR_RPCODE_EN            0x20
    #define IR_LG01H_CHK_EN         0x10
    #define IR_DCODE_PCHK_EN        0x08
    #define IR_CCODE_CHK_EN         0x04
    #define IR_LDCCHK_EN            0x02
    #define IR_EN                   0x01
#define REG_IR_HDC_UPB          (0x0001)
#define REG_IR_HDC_LOB          (0x0002)
#define REG_IR_OFC_UPB          (0x0003)
#define REG_IR_OFC_LOB          (0x0004)
#define REG_IR_OFC_RP_UPB       (0x0005)
#define REG_IR_OFC_RP_LOB       (0x0006)
#define REG_IR_LG01H_UPB        (0x0007)
#define REG_IR_LG01H_LOB        (0x0008)
#define REG_IR_LG0_UPB          (0x0009)
#define REG_IR_LG0_LOB          (0x000A)
#define REG_IR_LG1_UPB          (0x000B)
#define REG_IR_LG1_LOB          (0x000C)
#define REG_IR_SEPR_UPB         (0x000D)
#define REG_IR_SEPR_LOB         (0x000E)
#define REG_IR_TIMEOUT_CYC_L    (0x000F)
#define REG_IR_TIMEOUT_CYC_H_CODE_BYTE  (0x0010)
#define REG_IR_SEPR_BIT_FIFO_CTRL       (0x0011)
#define REG_IR_CCODE            (0x0012)
#define REG_IR_GLHRM_NUM        (0x0013)
#define REG_IR_CKDIV_NUM_KEY_DATA       (0x0014)
#define REG_IR_SHOT_CNT_L       (0x0015)
#define REG_IR_SHOT_CNT_H_FIFO_STATUS   (0x0016)
    #define IR_RPT_FLAG             0x0100
    #define IR_FIFO_EMPTY           0x0200
#define REG_IR_FIFO_RD_PULSE    (0x0018)


#endif
