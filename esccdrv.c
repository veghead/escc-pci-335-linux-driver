/******************************************************
 *
 * Copyright (C) 2000 Commtech, Inc. Wichita KS
 *
 * esccdrv.c -- init stuff for escc-pci module
 *
 * Tested with Linux version 2.2 12
 ******************************************************/

/* $Id: esccdrv.c,v 1.7 2005/01/21 20:57:23 carl Exp $ */

/*
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

If you encounter problems or have suggestions and/or bug fixes please email them to:

custserv@commtech-fastcom.com

or mailed to:

Commtech, Inc.
9011 E. 37th N.
Wichita, KS 67226
ATTN: Linux BugFix Dept

*/

#ifndef __KERNEL__
#define __KERNEL__
#endif
#ifndef MODULE
#define MODULE
#endif
#ifdef MODVERSIONS
#include <linux/modversions.h>
#endif
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/seq_file.h>
MODULE_LICENSE("GPL");

#include <linux/kernel.h> /* printk() */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(2, 4, 0)
#include <linux/slab.h>
#else
#include <linux/malloc.h> /* kmalloc() */
#endif
#include <linux/vmalloc.h>
#include <linux/fs.h>    /* everything... */
#include <linux/errno.h> /* error codes */
#include <linux/types.h> /* size_t */
#include <linux/proc_fs.h>
#include <linux/fcntl.h> /* O_ACCMODE */
#include <linux/ioport.h>
#include <linux/string.h>
#include <linux/pci.h> //for PCI stuff

#include "esccdrv.h" /* local definitions */

int escc_major = 0;
int escc_nr_devs = MAX_ESCC_BOARDS * 2; /* number of escc devices */
clkset clock;                           // not needed other than for param to ioctl building
Escc_Dev *escc_devices;                 /* allocated in init_module */
// these are a holdover from the ESCC-ISA card, but the locking is and can be usefull so
// I will leave most of it intact, even if it isn't necessary for all but a couple of registers (writes to PVR mostly)
struct semaphore boardlock[MAX_ESCC_BOARDS]; // keeps accesses to both channels of a single board in line
int used_board_numbers[MAX_ESCC_BOARDS];     // used to determine which devices are part of the same board
static struct proc_dir_entry *proc_escc_entry;
static struct pci_dev *escc_pci_devs[MAX_ESCC_BOARDS];

module_param(escc_major, int, 0444);
MODULE_PARM_DESC(escc_major, "Character device major number (0 for dynamic allocation)");

// if you need different ops on different minors, here is a good place to put them...
// we only have one set currently
// MODULE_AUTHOR("Carl");
// MODULE_DESCRIPTION("Character Driver for the ESCC devive");
/*
 * The different file operations
 */
struct file_operations escc_fops = {
    .owner = THIS_MODULE,
    .read = escc_read,
    .write = escc_write,
    .poll = escc_poll,
    .unlocked_ioctl = escc_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl = escc_ioctl,
#endif
    .open = escc_open,
    .release = escc_release,
    .fsync = escc_fsync,
    .llseek = escc_lseek,
};

#ifdef ESCC_USE_PROC /* don't waste space if unused */
/*
 * The proc filesystem: function to read an entry
 */

static int escc_proc_show(struct seq_file *m, void *v)
{
    int i;
    Escc_Dev *d;
    PDEBUG("escc_read_procmem entered");
    for (i = 0; i < escc_nr_devs; i++)
    {
        // fill in usefull info here, displayed in /proc/esccdrv via 'cat /proc/esccdev'
        d = &escc_devices[i];
        seq_printf(m, "Dev %i: Base:%x irq:%i ch:%i rb:%u rz:%u tb:%u tz:%u -- oc:%lu #t:%u\n",
                   i, d->base, d->irq, d->channel, d->settings.n_rbufs, d->settings.n_rfsize_max, d->settings.n_tbufs, d->settings.n_tfsize_max, d->escc_u_count, atomic_read(&d->transmit_frames_available));
    }
    return 0;
}

static int escc_proc_open(struct inode *inode, struct file *file)
{
    return single_open(file, escc_proc_show, NULL);
}

static const struct proc_ops escc_proc_ops = {
    .proc_open = escc_proc_open,
    .proc_read = seq_read,
    .proc_lseek = seq_lseek,
    .proc_release = single_release,
};

#endif /* SCULL_USE_PROC */

/*
 * Finally, the module stuff
 */
#ifdef USE_2_6
static int __init escc_init(void)
#else
int init_module(void)
#endif
{
    int result, i;
    int index;
    unsigned long amccbase[MAX_ESCC_BOARDS]; // holds all S5933 base addresses
    unsigned long esccbase[MAX_ESCC_BOARDS]; // holds all 82532 base addresses
    unsigned irq[MAX_ESCC_BOARDS];           // holds all needed IRQ's
    struct pci_dev *pdev = NULL;             // to get base addresses from pci subsystem
    board_settings board_switches;           // needed to pass to function that sets it all up
    Escc_Dev *dev;                           // needed everywhere
    int err = 0;

    for (i = 0; i < MAX_ESCC_BOARDS; i++)
    {
        // start blank
        amccbase[i] = 0;
        esccbase[i] = 0;
        irq[i] = 0;
    }
    index = 0;
#ifdef CONFIG_PCI
    for_each_pci_dev(pdev)
    {
        if ((pdev->vendor != ESCCP_VENDOR_ID) || (pdev->device != ESCCP_DEVICE_ID))
            continue;
        if (index >= MAX_ESCC_BOARDS)
            break;
        // ok here we have a board at bus/function, so get its stats
        err = pci_enable_device(pdev);
        if (err)
            continue;
        escc_pci_devs[index] = pci_dev_get(pdev);
        irq[index] = pdev->irq;
        amccbase[index] = pci_resource_start(pdev, 0) & PCI_BASE_ADDRESS_IO_MASK;
        esccbase[index] = pci_resource_start(pdev, 1) & PCI_BASE_ADDRESS_IO_MASK;

        printk(KERN_DEBUG "PCI BASE: amcc: %lx escc: %lx irq: %d\n", amccbase[index], esccbase[index], irq[index]);

        PDEBUG("amcc:%lx, escc:%lx, irq:%u, maj:%d\n", amccbase[index], esccbase[index], irq[index], escc_major);
        index++;
    }
    if (index == 0)
        return -ENODEV;
#else
    return -ENODEV;
#endif
    // if we get here then we have index # of esccp boards, and their particulars are
    // stored in amccbase[] esccbase[] and irq[]
    /*
     * Register your major, and accept a dynamic number
     */
    escc_nr_devs = index * 2;
    result = register_chrdev(escc_major, "esccp", &escc_fops);
    if (result < 0)
    {
        printk(KERN_WARNING "esccp: can't get major %d\n", escc_major);
        return result;
    }
    if (escc_major == 0)
        escc_major = result; /* dynamic */

    /*
     * allocate the devices
     */
    escc_devices = kmalloc(escc_nr_devs * sizeof(Escc_Dev), GFP_KERNEL);
    if (!escc_devices)
    {
        result = -ENOMEM;
        goto fail_malloc;
    }
    memset(escc_devices, 0, escc_nr_devs * sizeof(Escc_Dev));
    for (i = 0; i < escc_nr_devs; i++)
    {
        escc_devices[i].is_transmitting = 0;
        escc_devices[i].tx_type = XTF;
#if LINUX_VERSION_CODE >= VERSION_CODE(2, 4, 0)
        init_waitqueue_head(&escc_devices[i].rq);
        init_waitqueue_head(&escc_devices[i].wq);
        init_waitqueue_head(&escc_devices[i].sq);
#endif
#ifdef USE_2_6
        spin_lock_init(&escc_devices[i].irqlock);
#endif

        // set per instance data here
    }
    for (i = 0; i < MAX_ESCC_BOARDS; i++)
    {

        sema_init(&boardlock[i], 1);
        used_board_numbers[i] = 0;
    }
    /* At this point call the init function for any friend device */
    /* ... */
#ifdef USE_2_6
#else
    EXPORT_NO_SYMBOLS;
#endif

    // here we setup each board/channel
    for (i = 0; i < index; i++) // do for all boards that we found in PCI space
    {
        // add channel 0
        board_switches.amcc = amccbase[i];
        board_switches.base = esccbase[i];
        board_switches.irq = irq[i];
        board_switches.dmar = 0;
        board_switches.dmat = 0;
        board_switches.channel = 0;
        dev = &escc_devices[i * 2];
        if (esccbase[i] != 0) // don't bother if base is 0 it means no device present
        {
            if ((err = add_port(&board_switches, dev)) < 0)
                return err;
            dev->board = i;
            dev->complement_dev = &escc_devices[(i * 2) + 1];
        }
        // add channel 1
        board_switches.amcc = amccbase[i];
        board_switches.base = esccbase[i] + 0x40;
        board_switches.irq = irq[i];
        board_switches.dmar = 0;
        board_switches.dmat = 0;
        board_switches.channel = 1;
        dev = &escc_devices[(i * 2) + 1];
        if (esccbase[i] != 0) // don't bother if base is 0 it means no device present
        {
            if ((err = add_port(&board_switches, dev)) < 0)
                return err;
            dev->board = i;
            dev->complement_dev = &escc_devices[(i * 2)];
        }

        // here we look to see if the device needs an IRQ attached,
        // if the base address is nonzero it is assumed that the device is "real" and
        // we need to attach to the IRQ as long as our irq_hooked flag is 0.
        dev = &escc_devices[i * 2];
        if ((dev->base != 0) && (dev->irq != 0) && (dev->irq_hooked == 0))
        {
            PDEBUG("requesting IRQ in init\n");
            if ((err = request_irq(dev->irq, escc_irq, IRQF_SHARED, "esccp", escc_devices)) < 0)
            {
                PDEBUG("request_irq failed (in open):%i", err);
                return err;
            }
            dev->irq_hooked = 1;
        }
    }
#ifdef ESCC_USE_PROC /* only when available */
    /* this is the last line in init_module */
    proc_escc_entry = proc_create("esccpdrv", 0444, NULL, &escc_proc_ops);
    if (!proc_escc_entry)
        PDEBUG("Cannot create proc entry!\n");
#endif

    PDEBUG("MODULE-INIT\n");
    return 0; /* succeed */

fail_malloc:
    unregister_chrdev(escc_major, "esccp");
    return result;
}

// called on module unload
// clean up everything allocated here
#ifdef USE_2_6
static void __exit escc_cleanup(void)
#else
void cleanup_module(void)
#endif
{
    Escc_Dev *dev;
    int i;

    PDEBUG("MODULE-CLEANUP\n");
    unregister_chrdev(escc_major, "esccp");

#ifdef ESCC_USE_PROC
    remove_proc_entry("esccpdrv", 0);
#endif
    for (i = 0; i < escc_nr_devs; i++)
    {
        dev = &escc_devices[i];

        if (dev->irq_hooked == 1)
        {
            free_irq(dev->irq, escc_devices);
            dev->irq_hooked = 0;
        }

// give up io regions
#ifdef USE_2_6
#else
        // give up io regions
        if (escc_devices[i].base != 0)
            release_region(escc_devices[i].base, 0x40);
#endif

        // free allocated memory (if any)
        if ((dev->escc_rbuf != NULL) && (dev->escc_rbuf[0].frame != NULL))
            vfree(dev->escc_rbuf[0].frame);
        if ((dev->escc_tbuf != NULL) && (dev->escc_tbuf[0].frame != NULL))
            vfree(dev->escc_tbuf[0].frame);
        if (dev->escc_rbuf != NULL)
            vfree(dev->escc_rbuf);
        if (dev->escc_tbuf != NULL)
            vfree(dev->escc_tbuf);
    }
    // any cleanup then
    kfree(escc_devices);
    for (i = 0; i < MAX_ESCC_BOARDS; i++)
    {
        if (!escc_pci_devs[i])
            continue;
        pci_disable_device(escc_pci_devs[i]);
        pci_dev_put(escc_pci_devs[i]);
        escc_pci_devs[i] = NULL;
    }

    /* and call the cleanup functions for friend devices */
}

#ifdef USE_2_6
module_exit(escc_cleanup);
module_init(escc_init);
#endif
