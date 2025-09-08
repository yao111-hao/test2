#include <linux/module.h>
#define INCLUDE_VERMAGIC
#include <linux/build-salt.h>
#include <linux/elfnote-lto.h>
#include <linux/vermagic.h>
#include <linux/compiler.h>

BUILD_SALT;
BUILD_LTO_INFO;

MODULE_INFO(vermagic, VERMAGIC_STRING);
MODULE_INFO(name, KBUILD_MODNAME);

__visible struct module __this_module
__section(".gnu.linkonce.this_module") = {
	.name = KBUILD_MODNAME,
	.init = init_module,
#ifdef CONFIG_MODULE_UNLOAD
	.exit = cleanup_module,
#endif
	.arch = MODULE_ARCH_INIT,
};

#ifdef CONFIG_RETPOLINE
MODULE_INFO(retpoline, "Y");
#endif

static const struct modversion_info ____versions[]
__used __section("__versions") = {
	{ 0x25f8bfc1, "module_layout" },
	{ 0x609f1c7e, "synchronize_net" },
	{ 0x2d3385d3, "system_wq" },
	{ 0x7870c286, "kmem_cache_destroy" },
	{ 0x277c232f, "cdev_del" },
	{ 0x745fbadf, "netdev_info" },
	{ 0xc1c7e6c0, "kmalloc_caches" },
	{ 0xeb233a45, "__kmalloc" },
	{ 0x5d7a3971, "cdev_init" },
	{ 0x1fdc7df2, "_mcount" },
	{ 0xed6b80f3, "pci_enable_sriov" },
	{ 0xb5b54b34, "_raw_spin_unlock" },
	{ 0x94c27ac9, "debugfs_create_dir" },
	{ 0x91eb9b4, "round_jiffies" },
	{ 0x28ef4c7f, "napi_disable" },
	{ 0x98cf60b3, "strlen" },
	{ 0x970f20a5, "napi_schedule_prep" },
	{ 0xd3fa2c0d, "__napi_schedule_irqoff" },
	{ 0x41ed3709, "get_random_bytes" },
	{ 0x39513822, "dma_set_mask" },
	{ 0x7052fddf, "pcie_set_readrq" },
	{ 0x5821f3a1, "pci_disable_device" },
	{ 0xeb1c4eaa, "pci_disable_msix" },
	{ 0x95855422, "netif_carrier_on" },
	{ 0xc3690fc, "_raw_spin_lock_bh" },
	{ 0x2fde57c0, "pci_disable_sriov" },
	{ 0xffeedf6a, "delayed_work_timer_fn" },
	{ 0x402c5a08, "netif_carrier_off" },
	{ 0x56470118, "__warn_printk" },
	{ 0xcd9f81af, "device_destroy" },
	{ 0x9a08e796, "__dev_kfree_skb_any" },
	{ 0xeae3dfd6, "__const_udelay" },
	{ 0xee2b8603, "pci_release_regions" },
	{ 0xc6f46339, "init_timer_key" },
	{ 0xfb5acfaf, "pcie_capability_clear_and_set_word" },
	{ 0x9fa7184a, "cancel_delayed_work_sync" },
	{ 0x3213f038, "mutex_unlock" },
	{ 0x6091b333, "unregister_chrdev_region" },
	{ 0xdcdaecf0, "pcie_get_readrq" },
	{ 0xdf204797, "dma_free_attrs" },
	{ 0x3c3ff9fd, "sprintf" },
	{ 0x70d4e8e4, "set_page_dirty" },
	{ 0xe2147518, "kthread_create_on_node" },
	{ 0x18aac8af, "dma_set_coherent_mask" },
	{ 0x15ba50a6, "jiffies" },
	{ 0x3cfe412d, "kthread_bind" },
	{ 0xd9a5ea54, "__init_waitqueue_head" },
	{ 0x80c0af65, "pci_set_master" },
	{ 0x6caf5da2, "__alloc_pages" },
	{ 0xdcb764ad, "memset" },
	{ 0x9f91bcf2, "netif_tx_stop_all_queues" },
	{ 0x9b4f6cce, "pci_iounmap" },
	{ 0x8518a4a6, "_raw_spin_trylock_bh" },
	{ 0xd35cce70, "_raw_spin_unlock_irqrestore" },
	{ 0xf21788f1, "ethtool_op_get_link" },
	{ 0x112b81f7, "kthread_stop" },
	{ 0xb6568815, "free_netdev" },
	{ 0x9166fada, "strncpy" },
	{ 0x42c4eece, "register_netdev" },
	{ 0xf1560f85, "netif_receive_skb" },
	{ 0xa5e16cdd, "napi_enable" },
	{ 0x43cfdd90, "debugfs_remove" },
	{ 0x8ec7dbb2, "dma_alloc_attrs" },
	{ 0xef987bee, "kmem_cache_free" },
	{ 0x4dfa8d4b, "mutex_lock" },
	{ 0x8c03d20c, "destroy_workqueue" },
	{ 0x184c91d1, "__dev_kfree_skb_irq" },
	{ 0x6626afca, "down" },
	{ 0xa7a2e932, "netif_set_real_num_rx_queues" },
	{ 0x396ecf2f, "device_create" },
	{ 0xc38c83b8, "mod_timer" },
	{ 0xe2504c9c, "netif_set_real_num_tx_queues" },
	{ 0x18cf4aed, "netif_napi_add" },
	{ 0x92d5838e, "request_threaded_irq" },
	{ 0x6b4b2933, "__ioremap" },
	{ 0x89c9a638, "_dev_err" },
	{ 0x42160169, "flush_workqueue" },
	{ 0xfe487975, "init_wait_entry" },
	{ 0xd5a0871d, "cdev_add" },
	{ 0x167c5967, "print_hex_dump" },
	{ 0x85fd2dbe, "_dev_info" },
	{ 0xa3e4616a, "kmem_cache_alloc" },
	{ 0xbdb821cb, "__free_pages" },
	{ 0x618911fc, "numa_node" },
	{ 0xa916b694, "strnlen" },
	{ 0xe77bf8a, "pci_enable_msix_range" },
	{ 0xfe916dc6, "hex_dump_to_buffer" },
	{ 0xa748901f, "__napi_schedule" },
	{ 0xe46021ca, "_raw_spin_unlock_bh" },
	{ 0xb2fcb56d, "queue_delayed_work_on" },
	{ 0x296695f, "refcount_warn_saturate" },
	{ 0x3ea1b6e4, "__stack_chk_fail" },
	{ 0x1000e51, "schedule" },
	{ 0x8ddd8aad, "schedule_timeout" },
	{ 0x92997ed8, "_printk" },
	{ 0xf9236d43, "napi_complete_done" },
	{ 0xf360d110, "dma_map_page_attrs" },
	{ 0x9f0b2287, "eth_type_trans" },
	{ 0x69f38847, "cpu_hwcap_keys" },
	{ 0xf38a2402, "dev_driver_string" },
	{ 0x33bddfeb, "wake_up_process" },
	{ 0x57a14207, "netdev_err" },
	{ 0xcbd4898c, "fortify_panic" },
	{ 0xd1ba9b3c, "pci_unregister_driver" },
	{ 0xd58d4b67, "kmem_cache_alloc_trace" },
	{ 0xba8fbd64, "_raw_spin_lock" },
	{ 0x3928efe9, "__per_cpu_offset" },
	{ 0x34db050b, "_raw_spin_lock_irqsave" },
	{ 0xa0f1399e, "kmem_cache_create" },
	{ 0x32d1a1e5, "__netif_napi_del" },
	{ 0x3eeb2322, "__wake_up" },
	{ 0xb3f7646e, "kthread_should_stop" },
	{ 0x8c26d495, "prepare_to_wait_event" },
	{ 0xdc0e4855, "timer_delete" },
	{ 0x37a0cba, "kfree" },
	{ 0x4829a47e, "memcpy" },
	{ 0x6df1aaf1, "kernel_sigaction" },
	{ 0xa841a81e, "pci_request_regions" },
	{ 0xaf56600a, "arm64_use_ng_mappings" },
	{ 0xb3f33bb6, "pci_num_vf" },
	{ 0xedc03953, "iounmap" },
	{ 0xcf2a6966, "up" },
	{ 0x755b420c, "__pci_register_driver" },
	{ 0x8cb3f82a, "class_destroy" },
	{ 0x2a78242c, "dma_unmap_page_attrs" },
	{ 0x92540fbf, "finish_wait" },
	{ 0xb09b7637, "unregister_netdev" },
	{ 0x1ba59527, "__kmalloc_node" },
	{ 0xc5b6f236, "queue_work_on" },
	{ 0x656e4a6e, "snprintf" },
	{ 0xf1d49b35, "pci_iomap" },
	{ 0x905b4467, "pci_bus_max_busnr" },
	{ 0x7f02188f, "__msecs_to_jiffies" },
	{ 0x1e757432, "__napi_alloc_skb" },
	{ 0xc60d0620, "__num_online_cpus" },
	{ 0x2debd5a1, "pci_enable_device" },
	{ 0xd379d317, "pci_msix_vec_count" },
	{ 0x14b89635, "arm64_const_caps_ready" },
	{ 0xeab3563e, "__class_create" },
	{ 0x49cd25ed, "alloc_workqueue" },
	{ 0xd12fd7b, "flush_dcache_page" },
	{ 0x9e7d6bd0, "__udelay" },
	{ 0xe3ec2f2b, "alloc_chrdev_region" },
	{ 0x115d7734, "__put_page" },
	{ 0x7fdab77c, "get_user_pages_fast" },
	{ 0xdafd6b1f, "__skb_pad" },
	{ 0xc31db0ce, "is_vmalloc_addr" },
	{ 0xc1514a3b, "free_irq" },
	{ 0xef740323, "alloc_etherdev_mqs" },
};

MODULE_INFO(depends, "");

MODULE_ALIAS("pci:v000010EEd00009048sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd00009148sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd00009248sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd00009348sv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000903Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000913Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000923Fsv*sd*bc*sc*i*");
MODULE_ALIAS("pci:v000010EEd0000933Fsv*sd*bc*sc*i*");

MODULE_INFO(srcversion, "3476192445EB290D8ACA6A6");
