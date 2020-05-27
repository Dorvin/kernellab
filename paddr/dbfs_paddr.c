#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <asm/pgtable.h>
#include <asm/io.h>

MODULE_LICENSE("GPL");

static struct dentry *dir, *output;
static struct task_struct *task;

// Should use same format with app.c to communicate
struct packet {
	pid_t pid;
	unsigned long vaddr;
	unsigned long paddr;
};

static ssize_t read_output(struct file *fp,
                        char __user *user_buffer,
                        size_t length,
                        loff_t *position)
{
        // Implement read file operation
        // Get packet struct to communicate
        struct packet *packet_buf = (struct packet*)user_buffer;
        // Get task struct
        task = pid_task(find_vpid(pid), PIDTYPE_PID);
        // Walk process
        // pgd->p4d->pud->pmd->pte
        pgd_t *pgd = pgd_offset(task->mm, packet_buf->vaddr);
        p4d_t *p4d = p4d_offset(pgd, packet_buf->vaddr);
        pud_t *pud = pud_offset(p4d, packet_buf->vaddr);
        pmd_t *pmd = pmd_offset(pud, packet_buf->vaddr);
        pte_t *pte = pte_offset_kernel(pmd, packet_buf->vaddr);

        // Finaly write paddr
        // PA = PFN | PO
        packet_buf = (pte_pfn(*pte) << 12) | (packet_buf->vaddr & ((1 << 12) - 1));

        return 0;
}

static const struct file_operations dbfs_fops = {
        // Mapping file operations with your functions
        .read = read_output,
};

static int __init dbfs_module_init(void)
{
        // Implement init module
        dir = debugfs_create_dir("paddr", NULL);

        if (!dir) {
                printk("Cannot create paddr dir\n");
                return -1;
        }

        // Fill in the arguments below
        output = debugfs_create_file("output", S_IRWXU, dir, NULL, &dbfs_fops);

	printk("dbfs_paddr module initialize done\n");

        return 0;
}

static void __exit dbfs_module_exit(void)
{
        // Implement exit module
        debugfs_remove_recursive(dir);

	printk("dbfs_paddr module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);
