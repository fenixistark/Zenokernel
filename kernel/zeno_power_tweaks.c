// SPDX-License-Identifier: GPL-2.0
/* ZenoKernel: Native kernel thread to enforce VM efficiency without root */
#include <linux/kthread.h>
#include <linux/delay.h>
#include <linux/sched.h>
#include <linux/kmod.h>

extern int vm_swappiness;
extern int vm_dirty_ratio;
extern int dirty_background_ratio;
extern unsigned long vm_dirty_bytes;
extern unsigned long dirty_background_bytes;

static void zeno_zram_resize(void)
{
    char *envp[] = { "HOME=/", "PATH=/sbin:/system/sbin:/system/bin:/system/xbin", NULL };
    
    char *argv_swapoff[] = { "/system/bin/swapoff", "/dev/block/zram0", NULL };
    char *argv_size[] = { "/system/bin/sh", "-c", "echo 6442450944 > /sys/block/zram0/disksize", NULL };
    char *argv_mkswap[] = { "/system/bin/mkswap", "/dev/block/zram0", NULL };
    char *argv_swapon[] = { "/system/bin/swapon", "/dev/block/zram0", NULL };
    
    call_usermodehelper(argv_swapoff[0], argv_swapoff, envp, UMH_WAIT_EXEC);
    call_usermodehelper(argv_size[0], argv_size, envp, UMH_WAIT_EXEC);
    call_usermodehelper(argv_mkswap[0], argv_mkswap, envp, UMH_WAIT_EXEC);
    call_usermodehelper(argv_swapon[0], argv_swapon, envp, UMH_WAIT_EXEC);
    
    pr_info("ZenoKernel: ZRAM resized to 6GB and swapon\n");
}

static int zeno_power_tweaks_thread(void *data)
{
    bool zram_resized = false;
    ssleep(60); /* Aguarda o Android terminar o boot */

    while (!kthread_should_stop()) {
        if (!zram_resized) {
            zeno_zram_resize();
            zram_resized = true;
        }
        
        WRITE_ONCE(vm_swappiness, 60);
        WRITE_ONCE(dirty_background_ratio, 5);
        WRITE_ONCE(vm_dirty_ratio, 10);
        WRITE_ONCE(vm_dirty_bytes, 0UL);
        WRITE_ONCE(dirty_background_bytes, 0UL);
        ssleep(60);
    }
    return 0;
}

static int __init zeno_power_tweaks_init(void)
{
    kthread_run(zeno_power_tweaks_thread, NULL, "zeno_power_tweaks");
    return 0;
}
late_initcall(zeno_power_tweaks_init);
