/****************************************************************************
 *   FileName    : tcc_tsif_hwset.h
 *   Description : 
 ****************************************************************************
 *
 *   TCC Version 1.0
 *   Copyright (c) Telechips, Inc.
 *   ALL RIGHTS RESERVED
 *
 ****************************************************************************/
#ifndef __TCC_TSIF_MODULE_HWSET_H__
#define __TCC_TSIF_MODULE_HWSET_H__

#include <mach/memory.h>
#include <mach/irqs.h>



#pragma pack(push, 4)
struct tcc_tsif_regs {
    volatile unsigned long TSDI, TSRXCR, TSPID[16], TSIC, TSIS, TSISP, TSTXC;
};
#pragma pack(pop)

struct tea_dma_buf {
    void *v_addr;
    unsigned int dma_addr;
    int buf_size; // total size of DMA
    void *v_sec_addr;
    unsigned int dma_sec_addr;
    int buf_sec_size; // total size of DMA
};

typedef struct tcc_tsif_handle tcc_tsif_handle_t;
typedef int (*dma_alloc_f)(struct tea_dma_buf *tdma, unsigned int size);
typedef void (*dma_free_f)(struct tea_dma_buf *tdma);

typedef int (*FN_UPDATECALLBACK) (unsigned int dmxid, unsigned int ftype, unsigned fid, unsigned int value1, unsigned int value2, unsigned int bErrCRC);

struct tcc_tsif_handle {
    struct tea_dma_buf *dma_buffer;
    unsigned int dmx_id;
    unsigned int dma_intr_packet_cnt;
    unsigned int dma_mode;
    unsigned int serial_mode;
    unsigned char *fw_data;
    unsigned int fw_size;
    unsigned int tsif_port_num;
    unsigned int working_mode; //0:tsif for tdmb, 1:tsif mode for dvbt & isdbt, 2:internal mode

#if defined(CONFIG_iTV_PLATFORM)
    unsigned int dma_recv_packet_cnt;
    int cur_q_pos;
#endif
};

struct tcc_tsif_filter {
    unsigned int f_id;
    unsigned int f_type;
    unsigned int f_pid;
    unsigned int f_size;
    unsigned char *f_comp;
    unsigned char *f_mask;
    unsigned char *f_mode;
};

extern int tca_tsif_init(struct tcc_tsif_handle *h);
extern void tca_tsif_clean(struct tcc_tsif_handle *h);
extern int tca_tsif_cm3_fw_init(struct tcc_tsif_handle *h);
extern void tca_tsif_cm3_fw_deinit(struct tcc_tsif_handle *h);
extern int tca_tsif_start(struct tcc_tsif_handle *h);
extern int tca_tsif_stop(struct tcc_tsif_handle *h);

extern int tca_tsif_register_pids(struct tcc_tsif_handle *h, unsigned int *pids, unsigned int count);
extern int tca_tsif_can_support(void);
extern int tca_tsif_buffer_updated_callback(struct tcc_tsif_handle *h, FN_UPDATECALLBACK buffer_updated);
extern int tca_tsif_set_pcrpid(struct tcc_tsif_handle *h, unsigned int pid);
extern long long tca_tsif_get_stc(struct tcc_tsif_handle *h);
extern int tca_tsif_add_filter(struct tcc_tsif_handle *h, struct tcc_tsif_filter *feed);
extern int tca_tsif_remove_filter(struct tcc_tsif_handle *h, struct tcc_tsif_filter *feed);
#endif /*__TCC_TSIF_MODULE_HWSET_H__*/
