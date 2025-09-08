#ifndef ONIC_H
#define ONIC_H

#include <linux/netdevice.h>
#include <linux/cpumask.h>
#include "libqdma_export.h"
#include "onic_register.h"
#include "qdma_access/qdma_access_common.h"
#include "qdma_descq.h"

#define ONIC_ERROR_STR_BUF_LEN              (512)

#define ONIC_RX_COPY_THRES                  (256)
#define ONIC_RX_PULL_LEN                    (128)
#define ONIC_NAPI_WEIGHT                    (64)

#define DRV_CDEV_NAME "reconic-mm"
#include "onic_cdev.h"

#define QDMA_BAR 0
#define QDMA_USER_BAR 2
#define QDMA_QUEUE_BASE 0
#define QDMA_QUEUE_MAX 1024
#define CMAC_PORT_ID 0
#define QDMA_MM_QUEUE 4
#define QDMA_NET_QUEUE 64
#define QMDA_TOTAL_QUEUE_ACTIVE (QDMA_MM_QUEUE + QDMA_NET_QUEUE)
#define RING_SIZE 1024
#define C2H_TMR_CNT 5
#define C2H_CNT_THR 64
#define C2H_BUF_SIZE 1024
#define PCI_MSIX_USER_CNT 1

#define CMAC_RX_LANE_ALIGNMENT_RESET_CNT 8
#define CMAC_RX_LANE_ALIGNMENT_TIMEOUT_CNT 32

struct qdma_fmap_ctxt {
    u32 qbase:11;
    u32 rsvd0:21;
    u32 qmax:12;
    u32 rsvd1:20;
};

struct onic_dma_request {
  struct sk_buff *skb;
  struct net_device *netdev;
  struct qdma_request qdma;
  struct qdma_sw_sg sgl[MAX_SKB_FRAGS];
};

struct onic_platform_info {
  u8 qdma_bar;
  u8 user_bar;
  u16 queue_base;
  u16 queue_max;
  u16 used_queues;
  u16 active_tx_queues;
  u16 active_rx_queues;
  u16 mm_queues;
  u8 pci_msix_user_cnt;
  bool pci_master_pf;
  bool poll_mode;
  bool intr_mod_en;
  int ring_sz;
  int c2h_tmr_cnt;
  int c2h_cnt_thr;
  int c2h_buf_sz;
  bool rsfec_en;
  u8 port_id;
  u8 mac_addr[6];
};

/* ONIC Net device private structure */
struct onic_priv {
  u8 rx_desc_rng_sz_idx;
  u8 tx_desc_rng_sz_idx;
  u8 rx_buf_sz_idx;
  u8 rx_timer_idx;
  u8 rx_cnt_th_idx;
  u8 cmpl_rng_sz_idx;

  struct net_device *netdev;
  struct pci_dev *pcidev;
  struct onic_platform_info *pinfo;

  u16 num_msix;
  u16 nb_queues;

  struct kmem_cache *dma_req;
  struct qdma_dev_conf qdma_dev_conf;
  struct qdma_dev* qdev;
  unsigned long dev_handle;
  void __iomem *bar_base;

  unsigned long base_tx_q_handle, base_rx_q_handle;
  struct napi_struct *napi;
  struct rtnl_link_stats64 *tx_qstats, *rx_qstats;
  struct onic_cdev *onic_cdev_ptr;
};

#endif /* ONIC_H */
