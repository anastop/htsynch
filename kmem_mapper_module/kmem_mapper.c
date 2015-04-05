/*#include <linux/config.h>*/
#include <linux/version.h>
#include <linux/init.h>
#include <linux/mman.h>
#include <linux/errno.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mm.h>
#ifdef MODVERSIONS
#  include <linux/modversions.h>
#endif
#include <asm/io.h>
/*#include <asm/highmem.h>*/

#include <linux/cpu_controls.h>

#define KMEM_MAPPER_MAJOR 201
int kmem_mapper_major = KMEM_MAPPER_MAJOR;
int kmem_mapper_minor = 0;

/* character device structures */
static struct cdev kmem_mapper_cdev;

/* methods of the character device */
static int kmem_mapper_open(struct inode *inode, struct file *filp);
static int kmem_mapper_release(struct inode *inode, struct file *filp);
static int kmem_mapper_mmap(struct file *filp, struct vm_area_struct *vma);

/* the file operations, i.e. all character device methods */
static struct file_operations kmem_mapper_fops = {
    .open = kmem_mapper_open,
    .release = kmem_mapper_release,
    .mmap = kmem_mapper_mmap,
    .owner = THIS_MODULE,
};

/* 
 * Original pointer for kmalloc'd area as returned by kmalloc
 * The mwmon_mmap_area pointer will be rounded up to a page
 * boundary, in case kmalloc_ptr does not do so, and 
 * mwmon_mmap_area2 will point MONITORED_REGIONS_DISTANCE
 * bytes afterwards.
 */ 
static char *kmalloc_ptr;

/* 
 * The number of bytes between the first byte of the first
 * monitored memory region (as pointed by mwmon_mmap_area)
 * and the first byte of the second monitored memory region 
 * (as pointed by mwmon_mmap_area2).
 */
#define MONITORED_REGIONS_DISTANCE 512
#define LEN (4096)
unsigned long virt_addr;

/* character device open method */
static int kmem_mapper_open(struct inode *inode, struct file *filp)
{
    int i;

    printk(KERN_ALERT "[kmem_mapper]: Device opened. \n");

    /*Every time the device is opened, initialize the first */
    /*10 bytes of the 2 monitored memory areas...*/
    for (i = 0; i < 10; i++) {
        mwmon_mmap_area[i] = MWMON_ORIGINAL_VAL;
        mwmon_mmap_area2[i] = MWMON_ORIGINAL_VAL;
    }

    return 0;
}

/* character device last close method */
static int kmem_mapper_release(struct inode *inode, struct file *filp)
{
    printk(KERN_ALERT "[kmem_mapper]: Device closed\n");
    return 0;
}


#ifndef VMALLOC_VMADDR
#define VMALLOC_VMADDR(x) ((unsigned long)(x))
#endif

/*From: http://www.scs.ch/~frey/linux/memorymap.html*/
volatile void *virt_to_kseg(volatile void *address) {
    pgd_t *pgd; pmd_t *pmd; pte_t *ptep, pte;
    unsigned long va, ret = 0UL;
    va=VMALLOC_VMADDR((unsigned long)address);
    /* get the page directory. Use the kernel memory map. */
    pgd = pgd_offset_k(va);
    /* check whether we found an entry */
    if (!pgd_none(*pgd)) {
         /*I'm not sure if we need this, or the line for 2.4*/
            /*above will work reliably too*/
         /*If you know, please email me :-)*/
        pud_t *pud = pud_offset(pgd, va);       
        pmd = pmd_offset(pud, va);
        /* check whether we found an entry */
        if (!pmd_none(*pmd)) {
            /* get a pointer to the page table entry */
            ptep = pte_offset_map(pmd, va);
            pte = *ptep;
            /* check for a valid page */
            if (pte_present(pte)) {
                /* get the address the page is refering to */
                ret = (unsigned long)page_address(pte_page(pte));
                /* add the offset within the page to the page address */
                ret |= (va & (PAGE_SIZE -1));
            }
        }
    }
    return((volatile void *)ret);
}

static int kmem_mapper_mmap(struct file * filp, struct vm_area_struct * vma) {
    int ret;
    
    printk(KERN_ALERT "[kmem_mapper]: Mapping device memory to userspace...\n");
    printk(KERN_ALERT "[kmem_mapper]: mapped memory starts at user virt. address: 0x%lx\n", 
                                    vma->vm_start);
    printk(KERN_ALERT "[kmem_mapper]: mapped memory ends at user virt. address: 0x%lx\n", 
                                    vma->vm_end);

    /*vma->vm_start, vma->vm_pgoff << PAGE_SHIFT, kmalloc_ptr;*/

    /*arg1: the virtual memory area into which the page range is being mapped*/
    /*arg2: the userspace virtual address where remapping should begin*/
    /*arg3: the page frame number corresponding to the physical address to which*/
        /*the virtual address should be mapped. The p.f.n. us simply the physical*/
        /*address right shifted by PAGE_SHIFT bits. For most uses, the vm_pgoff field*/
        /*of the vma structure contains exactly the value you need.*/
    /*arg4: the size, in bytes, of the area being remapped*/
    /*arg5: the protection requested for the new VMA.*/
    ret = remap_pfn_range(vma,
               vma->vm_start,
               virt_to_phys((void*)((unsigned long)mwmon_mmap_area)) >> PAGE_SHIFT,
               vma->vm_end-vma->vm_start,
               PAGE_SHARED);
               /*vma->vm_page_prot); */
    if(ret != 0) {
        return -EAGAIN;
    }
    return 0;
}


/* module initialization - called at module load time */
static int __init kmem_mapper_init(void)
{
    int ret = 0;
    int i;
    dev_t devno = 0;

    devno = MKDEV(kmem_mapper_major, kmem_mapper_minor);
    ret = register_chrdev_region(devno, 1, "kmem_mapper");
    if (ret < 0) {
        printk(KERN_WARNING "kmem_mapper: can't get major %d\n", kmem_mapper_major);
        unregister_chrdev_region(devno, 1);
        return ret;
    }
    
    /* initialize the device structure and register the device with the kernel */
    cdev_init(&kmem_mapper_cdev, &kmem_mapper_fops);
    if ((ret = cdev_add(&kmem_mapper_cdev, devno, 1)) < 0) {
        printk(KERN_ERR "[kmem_mapper]: could not allocate chrdev for kmem_mapper\n");
        return ret;
    }
    
    printk(KERN_ALERT "[kmem_mapper]: Loading module...\n");
    
    /*reserve memory with kmalloc - Allocating Memory in the Kernel*/
    kmalloc_ptr = kmalloc(LEN + 2 * PAGE_SIZE, GFP_KERNEL);
    if (!kmalloc_ptr) {
        printk(KERN_ALERT "[kmem_mapper]: kmalloc failed\n");
        return -1;
    }
    
    /*monitor/mwait operation requirements are met because:*/
    /*1. mwmon_mmap_area is aligned at page boundary (this happens also when mapped*/
    /*at userspace), so it aligned at MWMON_ALIGN_BOUNDARY (64), too. */
    /*2. we take care (in kernelspace, i.e. system call implementations, as*/
    /*well as in userspace), not to update any memory location within the next */
    /*MWMON_MONITORED_MEM_BYTES (64) bytes from mwmon_mmap_area. That would*/
    /*trigger false wake-ups.*/
    
    mwmon_mmap_area = (char *)(((unsigned long)kmalloc_ptr + PAGE_SIZE -1) & PAGE_MASK);
    mwmon_mmap_area2 = mwmon_mmap_area + MONITORED_REGIONS_DISTANCE;
    
    printk(KERN_ALERT "[kmem_mapper]: allocated area starts at kernel virt. "
            "address: 0x%lx\n", (unsigned long)kmalloc_ptr);
    printk(KERN_ALERT "[kmem_mapper]: first page-aligned monitored mem. region "
            "starts at kernel virt. address: 0x%lx (physical address: 0x%lx," 
            "page frame: 0x%lx)\n", 
            (unsigned long)mwmon_mmap_area, 
            virt_to_phys((void*)((unsigned long)mwmon_mmap_area)),
            virt_to_phys((void*)((unsigned long)mwmon_mmap_area)) >> PAGE_SHIFT  );
    printk(KERN_ALERT "[kmem_mapper]: second page-aligned monitored mem. region "
            "starts at kernel virt. address: 0x%lx (physical address: 0x%lx,"
            "page frame: 0x%lx)\n", 
            (unsigned long)mwmon_mmap_area2, 
            virt_to_phys((void*)((unsigned long)mwmon_mmap_area2)),
            virt_to_phys((void*)((unsigned long)mwmon_mmap_area2)) >> PAGE_SHIFT  );


    for (virt_addr=(unsigned long)mwmon_mmap_area; 
        virt_addr < (unsigned long)mwmon_mmap_area + LEN;
        virt_addr+=PAGE_SIZE) {
            /*reserve all pages to make them remapable*/
            SetPageReserved(virt_to_page(virt_addr));
    }
    /*printk(KERN_ALERT "[kmem_mapper]: mwmon_mmap_area at 0x%p (phys 0x%lx)\n", mwmon_mmap_area,*/
        /*virt_to_phys((void *)virt_to_kseg(mwmon_mmap_area)));*/


    /*Initialize the first 10 bytes of the 2 monitored memory regions...*/
    for (i = 0; i < 10; i++) {
        mwmon_mmap_area[i] = MWMON_ORIGINAL_VAL;
        mwmon_mmap_area2[i] = MWMON_ORIGINAL_VAL;
    }

#ifdef KERNEL_2_6_13_1
    /*Initialize the lint variable address at a safe distance (non-triggerable) from the*/
    /*beginning of the monitored memory area.*/
    /*This has to be done also in userspace.*/
    mwmon_lint_var_addr = (long int*)(mwmon_mmap_area + MWMON_LINT_VAR_OFFSET); 
    
    printk(KERN_ALERT "[kmem_mapper]: long int variable is located at kernel virt. address: 0x%lx\n",
                                     (unsigned long)mwmon_lint_var_addr);
#endif

    return 0;

}

/* module unload */
static void __exit kmem_mapper_exit(void)
{
    dev_t devno = MKDEV(kmem_mapper_major, kmem_mapper_minor);
    
    printk(KERN_ALERT "[kmem_mapper]: Unloading module\n");
    
    for (virt_addr=(unsigned long)mwmon_mmap_area; virt_addr < (unsigned long)mwmon_mmap_area + LEN;
        virt_addr+=PAGE_SIZE) {
            /*clear all pages*/
            ClearPageReserved(virt_to_page(virt_addr));
    }
    kfree(kmalloc_ptr);
    
    /* remove the character device */
    cdev_del(&kmem_mapper_cdev);
    unregister_chrdev_region(devno, 1);
}

module_init(kmem_mapper_init);
module_exit(kmem_mapper_exit);
MODULE_DESCRIPTION("kmem_mapper driver");
MODULE_AUTHOR("anastop");
MODULE_LICENSE("Dual BSD/GPL");
