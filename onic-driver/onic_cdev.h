/*
 * Copyright (c) 2021 Xilinx, Inc.
 * All rights reserved.
 *
 * This source code is free software; you can redistribute it and/or modify it
 * under the terms and conditions of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 */
#ifndef __ONIC_CDEV_H__
#define __ONIC_CDEV_H__

#include <linux/cdev.h>
#include "onic.h"
//#include "qdma_access/qdma_export.h"
#include "libqdma/libqdma_export.h"
#include "libqdma/qdma_device.h"
#include <asm/cacheflush.h>
#include <linux/syscalls.h>
#include <linux/semaphore.h>

#define ONIC_CDEV_CLASS_NAME DRV_CDEV_NAME
#define MAX_MINOR_DEV 64

/**
 * Data structure for a character device
 **/
struct onic_cdev {
  /* pointer to PCIe device handler */
  struct qdma_dev* qdev;
  /* Generic character device interface */
  struct cdev mm_cdev;
  /* Minor number */
  int cdev_minor;
  /* Major number */
  int cdev_major;
  /* Character device number */
  dev_t cdev_no;
  /* Minor number count */
  int cdev_minor_cnt;
  /* c2h queue handle */
  unsigned long mm_c2h_q_hndl;
  /* h2c queue handle */
  unsigned long mm_h2c_q_hndl;
  int no_mm_queues;
  unsigned long dev_handle;
  /* callback function to handle read/write request */
  ssize_t (*fp_rw)(unsigned long xpdev_hndl, unsigned long q_hndl, struct qdma_request *qd_req);
  /* name of the character device */
  char name[0];
  struct onic_priv *xpriv;
  //struct net_device *netdev;

  int read_idx;
  int write_idx;

};

/**
 * Data structure for io callback of a character device
 **/
struct cdev_io_cb {
  void *private;
  /* pointer to the user buffer */
  void __user *buf;
  /* length of the user buffer */
  size_t len;
  /* page number */
  unsigned int page_nb;
  /* scatter gather list */
  struct qdma_sw_sg *sgl;
  /* pages allocated to accommodate the scatter gather list */
  struct page **pages;
  /* qdma request */
  struct qdma_request qd_req;
};

/**
 * qdma scatter gather request
 * @ingroup libqdma_struct
 *
 */
//struct qdma_sw_sg {
    /** pointer to next page */
//    struct qdma_sw_sg *next;
    /** pointer to current page */
//    struct page *pg;
    /** offset in current page */
//    unsigned int offset;
    /** length of the page */
//    unsigned int len;
    /** dma address of the allocated page */
//    dma_addr_t dma_addr;
//};

/**
 * onic_init_cdev - initilize a character device
 * @onic_cdev_ptr: pointer to an onic_cdev data
 * Return 0 on success, negative on failure
 **/
int onic_init_cdev(struct onic_cdev *onic_cdev_ptr, int no_mm_queues);

/**
 * onic_destroy_cdev - destroy a character device
 * @onic_cdev_ptr: pointer to an onic_cdev data
 **/
void onic_destroy_cdev(struct onic_cdev *onic_cdev_ptr);

/**
 * onic_create_cdev - create a character device
 * @onic_cdev_ptr: pointer to an onic_cdev data
 * @xpriv: ONIC private data
 * @qid: QDMA qid as a char device minor number
 * Return 0 on success, negative on failure
 **/
int onic_create_cdev(struct onic_cdev *onic_cdev_ptr, struct onic_priv * xpriv, unsigned int qid);

#endif /* ifndef __ONIC_CDEV_H__ */
