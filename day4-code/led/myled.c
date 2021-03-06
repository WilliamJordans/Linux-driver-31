#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/io.h>

#define CNAME "myled" 
#define RED_PHY_BASE 0xc001a000   //a28
#define BLUE_PHY_BASE 0xc001b000  //b12
#define GREEN_PHY_BASE 0xc001e000 //e13

#define OUT    0
#define OUTENB 1
#define ALTFN0 8
#define ALTFN1 9

int major = 0;
char kbuf[2] = {0};

//映射之后产生的虚拟地址
unsigned int *red_virt_base = NULL;
unsigned int *green_virt_base = NULL;
unsigned int *blue_virt_base = NULL;

enum RGB_LED{
	RED,
	GREEN,
	BLUE,
};

int myled_open(struct inode *inode, struct file *file)
{
	int which;
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	which = MINOR(inode->i_rdev);
	file->private_data = (void *)which;
	return 0;
}

ssize_t myled_read(struct file *file, 
	char __user *ubuf, size_t size, loff_t *offs)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

ssize_t myled_write (struct file *file, 
	const char __user *ubuf, size_t size, loff_t *offs)
{
	int ret;
	int which;
	
	which = (int)file->private_data;
	
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);

	//1.拷贝用户空间的数据
	if(size > sizeof(kbuf)) size = sizeof(kbuf);
	ret = copy_from_user(kbuf,ubuf,size);
	if(ret){
		printk("copy data from user error\n");
		return -EIO;
	}

	//2.根据用户空间的数据进行点灯
	switch(which){
		case   RED: 
			kbuf[0]=='1'?(*(red_virt_base + OUT) |= (1<<28)):\
				(*(red_virt_base + OUT ) &= ~(1<<28));
			break;
		case GREEN: 	
			kbuf[0]=='1'?(*(green_virt_base + OUT) |= (1<<13)):\
				(*(green_virt_base + OUT) &= ~(1<<13));
			break;
		case  BLUE: 	
			kbuf[0]=='1'?(*(blue_virt_base + OUT) |= (1<<12)):\
				(*(blue_virt_base + OUT) &= ~(1<<12));
			break;
	}

	return size;
}

int myled_close(struct inode *inode, struct file *file)
{
	printk("%s:%s:%d\n",__FILE__,__func__,__LINE__);
	return 0;
}

const struct file_operations fops = {
	.open = myled_open,
	.read = myled_read,
	.write = myled_write,
	.release = myled_close,
};

static int __init myled_init(void)
{
	//1.注册字符设备驱动
	major = register_chrdev(0,CNAME,&fops);
	if(major < 0){
		printk("register chrdev error\n");
		return major;
	}

	//2.映射RGB_LED灯的地址
	red_virt_base = ioremap(RED_PHY_BASE,40);
	if(red_virt_base == NULL){
		printk("ioremap red led addr error\n");
		return -ENOMEM;
	}
	green_virt_base = ioremap(GREEN_PHY_BASE,40);
	if(green_virt_base == NULL){
		printk("ioremap green led addr error\n");
		return -ENOMEM;
	}
	blue_virt_base = ioremap(BLUE_PHY_BASE,40);
	if(blue_virt_base == NULL){
		printk("ioremap blue led addr error\n");
		return -ENOMEM;
	}

	//3.RGB_LED  INIT ALL OFF
	*(red_virt_base + ALTFN1) &= ~(3<<24);  //altfn1 24:25  gpio
	*(red_virt_base + OUTENB) |= (1<<28);  //outenb 28 输出
	*(red_virt_base + OUT   ) &= ~(1<<28); //out 28 low

	*(blue_virt_base + ALTFN0) &= ~(3<<24); 
	*(blue_virt_base + ALTFN0) |= (2<<24); 
	*(blue_virt_base + OUTENB) |= (1<<12);  
	*(blue_virt_base + OUT   ) &= ~(1<<12); 

	*(green_virt_base + ALTFN0) &= ~(3<<26); 
	*(green_virt_base + OUTENB) |= (1<<13);  
	*(green_virt_base + OUT   ) &= ~(1<<13); 

	
	return 0;
}

static void __exit myled_exit(void)
{
	//1.注销字符设备驱动
	unregister_chrdev(major,CNAME);

	//2.取消映射
	iounmap(red_virt_base);
	iounmap(blue_virt_base);
	iounmap(green_virt_base);
}
module_init(myled_init);
module_exit(myled_exit);
MODULE_LICENSE("GPL");









