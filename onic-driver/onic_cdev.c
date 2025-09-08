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
#include "onic_cdev.h"
#include <linux/pci.h>
#include <linux/syscalls.h>
/**
 * sysfs class structure
 **/
static struct class *onic_cdev_class = NULL;

// synchronisation
// for read synchronisation
static struct semaphore read_sem;
static struct semaphore read_mutex;
static struct semaphore read_mutex2;
static int read_read_idx;
static int read_write_idx;
static int *read_queue_pool;
// for write synchronisation
static struct semaphore write_sem;
static struct semaphore write_mutex;
static struct semaphore write_mutex2;
static int write_read_idx;
static int write_write_idx;
static int *write_queue_pool;

/**
 * Minor of this char device
 **/
static int cdev_minor = 0;

/**
 * Open a character device and initialize private data
 **/
static int onic_cdev_open(struct inode *inode, struct file *file) {
  struct onic_cdev *onic_cdev_ptr = container_of(inode->i_cdev, struct onic_cdev, mm_cdev);
  file->private_data = onic_cdev_ptr;
  dev_dbg(&onic_cdev_ptr->qdev->pdev->dev, "%s: Open onic_cdev.\n", onic_cdev_ptr->name);
  return 0;
}

/**
 * Close a character device
 **/
static int onic_cdev_close(struct inode *inode, struct file *file) {
  // Do nothing
  struct onic_cdev *onic_cdev_ptr = (struct onic_cdev*) file->private_data;
  dev_info(&onic_cdev_ptr->qdev->pdev->dev, "%s: Close onic_cdev.\n", onic_cdev_ptr->name);
  return 0;
}

static long onic_cdev_ioctl(
  struct file *file,	/* ditto */
  unsigned int ioctl_num,	/* number and param for ioctl */
  unsigned long ioctl_param){
	return 0;
}

static void unmap_user_buf(struct cdev_io_cb *iocb, bool write)
{
  int i;

  if (!iocb->pages || !iocb->page_nb)
    return;

  for (i = 0; i < iocb->page_nb; i++) {
    if (iocb->pages[i]) {
      if (!write)
        set_page_dirty(iocb->pages[i]);
      put_page(iocb->pages[i]);
    } else{
      break;
    }
  }

  if (i != iocb->page_nb)
    pr_err("sgl pages %d/%u.\n", i, iocb->page_nb);

  iocb->page_nb = 0;
}

/*
 * cdev r/w
 */
static inline void iocb_release(struct cdev_io_cb *iocb)
{
  if (iocb->pages)
    iocb->pages = NULL;
  kfree(iocb->sgl);
  iocb->sgl = NULL;
  iocb->buf = NULL;
}

static int map_user_buf_to_sgl(struct cdev_io_cb *iocb, bool write)
{
  unsigned long len = iocb->len;
  char *buf = iocb->buf;
  struct qdma_sw_sg *sg;
  unsigned int pg_off = offset_in_page(buf);
  unsigned int pages_nr = (len + pg_off + PAGE_SIZE - 1) >> PAGE_SHIFT;
  int i;
  int rv;

  if (len == 0)
    pages_nr = 1;
  if (pages_nr == 0)
    return -EINVAL;

  iocb->page_nb = 0;
  sg = kmalloc(pages_nr * (sizeof(struct qdma_sw_sg) +
          sizeof(struct page *)), GFP_KERNEL);
  if (!sg) {
    pr_err("sgl allocation failed for %u pages", pages_nr);
    return -ENOMEM;
  }

  memset(sg, 0, pages_nr * (sizeof(struct qdma_sw_sg) +
          sizeof(struct page *)));
  iocb->sgl = sg;

  iocb->pages = (struct page **)(sg + pages_nr);
  rv = get_user_pages_fast((unsigned long)buf, pages_nr, 1/* write */,
              iocb->pages);
  /* No pages were pinned */
  if (rv < 0) {
    pr_err("unable to pin down %u user pages, %d.\n",
        pages_nr, rv);
    goto err_out;
  }
  /* Less pages pinned than wanted */
  if (rv != pages_nr) {
    pr_err("unable to pin down all %u user pages, %d.\n",
        pages_nr, rv);
    iocb->page_nb = rv;
    rv = -EFAULT;
    goto err_out;
  }

  for (i = 1; i < pages_nr; i++) {
    if (iocb->pages[i - 1] == iocb->pages[i]) {
      pr_err("duplicate pages, %d, %d.\n",
          i - 1, i);
      iocb->page_nb = pages_nr;
      rv = -EFAULT;
      goto err_out;
    }
  }

  sg = iocb->sgl;
  for (i = 0; i < pages_nr; i++, sg++) {
    unsigned int offset = offset_in_page(buf);
    unsigned int nbytes = min_t(unsigned int, PAGE_SIZE - offset,
                    len);
    struct page *pg = iocb->pages[i];

    flush_dcache_page(pg);

    sg->next = sg + 1;
    sg->pg = pg;
    sg->offset = offset;
    sg->len = nbytes;
    sg->dma_addr = 0UL;

    buf += nbytes;
    len -= nbytes;
  }

  iocb->sgl[pages_nr - 1].next = NULL;
  iocb->page_nb = pages_nr;

  return 0;

err_out:
  unmap_user_buf(iocb, write);
  iocb_release(iocb);

  return rv;
}

static ssize_t onic_gen_read_write(struct file *file, char __user *buf,
        size_t count, loff_t *pos, bool write, int target_queue)
{
  struct onic_cdev *xcdev = (struct onic_cdev *)file->private_data;
  struct cdev_io_cb iocb;
  struct qdma_request *req = &iocb.qd_req;
  ssize_t res = 0;
  int rv;
  unsigned long qhndl;

  if (!xcdev) {
    pr_err("file 0x%p, xcdev NULL, 0x%p,%llu, pos %llu, W %d.\n",
        file, buf, (u64)count, (u64)*pos, write);
    return -EINVAL;
  }

  if (!xcdev->fp_rw) {
    pr_err("file 0x%p, %s, NO rw, 0x%p,%llu, pos %llu, W %d.\n",
        file, xcdev->name, buf, (u64)count, (u64)*pos, write);
    return -EINVAL;
  }

  qhndl = write ? xcdev->mm_h2c_q_hndl + target_queue : xcdev->mm_c2h_q_hndl + target_queue;

  pr_debug("%s, priv 0x%lx: buf 0x%p,%llu, pos %llu, W %d.\n",
      xcdev->name, qhndl, buf, (u64)count, (u64)*pos,
      write);

  memset(&iocb, 0, sizeof(struct cdev_io_cb));
  iocb.buf = buf;
  iocb.len = count;
  rv = map_user_buf_to_sgl(&iocb, write);
  if (rv < 0)
    return rv;

  req->sgcnt = iocb.page_nb;
  req->sgl = iocb.sgl;
  req->write = write ? 1 : 0;
  req->dma_mapped = 0;
  req->udd_len = 0;
  req->ep_addr = (u64)*pos;
  req->count = count;
  req->timeout_ms = 10 * 1000;    /* 10 seconds */
  req->fp_done = NULL;        /* blocking */
  req->h2c_eot = 1;        /* set to 1 for STM tests */

  res = xcdev->fp_rw(xcdev->dev_handle, qhndl, req);

  unmap_user_buf(&iocb, write);
  iocb_release(&iocb);

  if(!write){
    down(&read_mutex2);
    read_queue_pool[read_write_idx] = target_queue;
    read_write_idx = (read_write_idx + 1) % xcdev->no_mm_queues;
    up(&read_mutex2);
    up(&read_sem);
  }
  else{
    down(&write_mutex2);
    write_queue_pool[write_write_idx] = target_queue;
    write_write_idx = (write_write_idx + 1) % xcdev->no_mm_queues;
    up(&write_mutex2);
    up(&write_sem);
  }

  return res;
}

/**
 * Write operation for a character device
 **/
static ssize_t onic_cdev_write(struct file *file, const char __user *usr_buf, size_t count, loff_t *offset) {
  struct onic_cdev *onic_cdev_ptr = (struct onic_cdev*) file->private_data;
  int target_queue;
  down(&write_sem);
  down(&write_mutex);
  target_queue = write_queue_pool[write_read_idx];
  write_read_idx = (write_read_idx + 1) % onic_cdev_ptr->no_mm_queues;
  up(&write_mutex);
  dev_dbg(&onic_cdev_ptr->qdev->pdev->dev, "Write obtained queue %d\n", target_queue);
  return onic_gen_read_write(file, (char *) usr_buf, count, offset, 1, target_queue);
}

/**
 * Read operation for a character device
 **/
static ssize_t onic_cdev_read(struct file *file, char __user *usr_buf, size_t count, loff_t *offset) {
  struct onic_cdev *onic_cdev_ptr = (struct onic_cdev*) file->private_data;
  int target_queue;
  down(&read_sem);
  down(&read_mutex);
  target_queue = read_queue_pool[read_read_idx];
  read_read_idx = (read_read_idx + 1) % onic_cdev_ptr->no_mm_queues;
  up(&read_mutex);
  dev_dbg(&onic_cdev_ptr->qdev->pdev->dev, "Read obtained queue %d\n", target_queue);
  return onic_gen_read_write(file, (char *) usr_buf, count, offset, 0, target_queue);
}

/**
 * Set offset in the character device
 */
static loff_t onic_cdev_llseek(struct file *file, loff_t off, int whence) {
  struct onic_cdev *onic_cdev_ptr = (struct onic_cdev*) file->private_data;

  loff_t newpos = 0;

  switch (whence) {
  case 0: /* SEEK_SET */
    newpos = off;
    break;
  case 1: /* SEEK_CUR */
    newpos = file->f_pos + off;
    break;
  case 2: /* SEEK_END, @TODO should work from end of address space */
    newpos = UINT_MAX + off;
    break;
  default: /* can't happen */
    return -EINVAL;
  }
  if (newpos < 0)
    return -EINVAL;
  file->f_pos = newpos;

  dev_dbg(&onic_cdev_ptr->qdev->pdev->dev, "%s: pos=%lld\n", onic_cdev_ptr->name, (signed long long)newpos);

  return newpos;
}

/**
 * File operation registration
 **/
static const struct file_operations onic_cdev_fops = {
  .read         = onic_cdev_read,
  .write        = onic_cdev_write,
  .unlocked_ioctl = onic_cdev_ioctl,
  .open         = onic_cdev_open,
  .release      = onic_cdev_close,
  .llseek       = onic_cdev_llseek,
};

int onic_init_cdev(struct onic_cdev *onic_cdev_ptr, int no_mm_queues) {
  int i;

  sema_init(&read_sem, no_mm_queues);
  sema_init(&read_mutex, 1);
  sema_init(&read_mutex2, 1);
  read_read_idx = 0;
  read_write_idx = 0;

  for(i = 0 ; i < no_mm_queues ; i++){
    read_queue_pool[i] = i;
  }

  sema_init(&write_sem, no_mm_queues);
  sema_init(&write_mutex, 1);
  sema_init(&write_mutex2, 1);
  write_read_idx = 0;
  write_write_idx = 0;
  for(i = 0 ; i < no_mm_queues ; i++){
    write_queue_pool[i] = i;
  }
  return 0;
}

int onic_create_cdev(struct onic_cdev *onic_cdev_ptr, struct onic_priv * xpriv, unsigned int qid) {
  int err;
  struct device* sysfs_dev;
  struct xlnx_dma_dev *xdev;
  dev_t dev;
  int no_mm_queues;

  onic_cdev_ptr->cdev_minor_cnt = MAX_MINOR_DEV;

  xdev = (struct xlnx_dma_dev *)xpriv->dev_handle;
  no_mm_queues = xpriv->pinfo->mm_queues;

  onic_cdev_ptr->dev_handle = xpriv->dev_handle;
  xpriv->onic_cdev_ptr = onic_cdev_ptr;
  xpriv->onic_cdev_ptr->qdev = xdev_2_qdev(xdev);
  xpriv->onic_cdev_ptr->xpriv = xpriv;
  xpriv->onic_cdev_ptr->qdev->pdev = xpriv->pcidev;

  onic_cdev_ptr->fp_rw = qdma_request_submit;
  onic_cdev_ptr->no_mm_queues = no_mm_queues;

  read_queue_pool = (int*) kzalloc(sizeof(int) * no_mm_queues, GFP_KERNEL);
  write_queue_pool = (int*) kzalloc(sizeof(int) * no_mm_queues, GFP_KERNEL);

  // Create a cdev class
  onic_cdev_class = class_create(THIS_MODULE, ONIC_CDEV_CLASS_NAME);

  if(IS_ERR(onic_cdev_class)) {
    dev_err(&onic_cdev_ptr->qdev->pdev->dev, "%s: failed to create open-nic cdev class.",
            ONIC_CDEV_CLASS_NAME);
    onic_cdev_class = NULL;
    // return error with no such device
    return -ENODEV;
  }

  // allocate a range of character device number. The major number will be chosen dynamically
  err = alloc_chrdev_region(&dev, 0, onic_cdev_ptr->cdev_minor_cnt, ONIC_CDEV_CLASS_NAME);
  if(err) {
    dev_err(&onic_cdev_ptr->qdev->pdev->dev, "unable to allocate character device region %d.\n", err);
    return err;
  }

  onic_cdev_ptr->cdev_major = MAJOR(dev);
  sprintf(onic_cdev_ptr->name, "%s", ONIC_CDEV_CLASS_NAME);
  //dev_info(&onic_cdev_ptr->qdev->pdev->dev, "onic_cdev_ptr->name = %s\n", onic_cdev_ptr->name);

  onic_cdev_ptr->mm_cdev.owner = THIS_MODULE;
  if(qid >= onic_cdev_ptr->cdev_minor_cnt) {
    dev_err(&onic_cdev_ptr->qdev->pdev->dev, "%s: No character device available!\n", onic_cdev_ptr->name);
    onic_cdev_ptr->cdev_minor = cdev_minor;
    return -1;
  }else{
    cdev_minor = qid;
    onic_cdev_ptr->cdev_minor    = cdev_minor;
  }
  onic_cdev_ptr->cdev_no = MKDEV(onic_cdev_ptr->cdev_major, onic_cdev_ptr->cdev_minor);

  // Initialize the char device with its file operations
  cdev_init(&(onic_cdev_ptr->mm_cdev), &onic_cdev_fops);

  // Add the device to the system
  err = cdev_add(&onic_cdev_ptr->mm_cdev, onic_cdev_ptr->cdev_no, 1);
  if(err < 0){
    dev_err(&onic_cdev_ptr->qdev->pdev->dev, "cdev_add failed %d, %s\n", err, onic_cdev_ptr->name);
    return err;
  } else {
    dev_info(&onic_cdev_ptr->qdev->pdev->dev, "successfully cdev_add a character device, %s, to the system", onic_cdev_ptr->name);
  }

  // Create a device file node and register it with sysfs
  if(onic_cdev_class){
    sysfs_dev = device_create(onic_cdev_class, &(onic_cdev_ptr->qdev->pdev->dev), onic_cdev_ptr->cdev_no, NULL, "%s", onic_cdev_ptr->name);
    if(IS_ERR(sysfs_dev)) {
      err = PTR_ERR(sysfs_dev);
      dev_err(&onic_cdev_ptr->qdev->pdev->dev, "%s: device_create failed %d\n", onic_cdev_ptr->name, err);
      cdev_del(&onic_cdev_ptr->mm_cdev);
      return err;
    } else {
      dev_info(&onic_cdev_ptr->qdev->pdev->dev, "successffully device_create a character device, %s, and register it", onic_cdev_ptr->name);
    }
  }

  return 0;
}

void onic_destroy_cdev(struct onic_cdev *onic_cdev_ptr) {
  dev_info(&onic_cdev_ptr->qdev->pdev->dev, "%s cdev_major=%d before destroyed\n", onic_cdev_ptr->name, onic_cdev_ptr->cdev_major);
  cdev_del(&onic_cdev_ptr->mm_cdev);

  if(cdev_minor>=0) {
    device_destroy(onic_cdev_class, onic_cdev_ptr->cdev_no);
    dev_info(&onic_cdev_ptr->qdev->pdev->dev, "%s device_destroy done!\n", onic_cdev_ptr->name);
  } else {
    dev_err(&onic_cdev_ptr->qdev->pdev->dev, "%s device_destroy failed!\n", onic_cdev_ptr->name);
  }

  // Remove sysfs class for this char device
  if(onic_cdev_class) {
    //class_unregister(onic_cdev_class);
    class_destroy(onic_cdev_class);
    unregister_chrdev_region(MKDEV(onic_cdev_ptr->cdev_major, 0), MAX_MINOR_DEV);
    dev_info(&onic_cdev_ptr->qdev->pdev->dev, "%s class_unregister, class_destroy and unregister_chrdev_region done!\n", onic_cdev_ptr->name);
  } else {
    dev_err(&onic_cdev_ptr->qdev->pdev->dev, "%s class_unregister, class_destroy and unregister_chrdev_region failed!\n", onic_cdev_ptr->name);
  }

  // Reset major number assigned to this char device
  if(onic_cdev_ptr->cdev_major) {
    onic_cdev_ptr->cdev_major = 0;
      dev_info(&onic_cdev_ptr->qdev->pdev->dev, "%s cdev_major is reset to %d, onic_destroy_cdev done\n", onic_cdev_ptr->name, onic_cdev_ptr->cdev_major);
  }

  kfree(read_queue_pool);
  kfree(write_queue_pool);
  kfree(onic_cdev_ptr);
}
