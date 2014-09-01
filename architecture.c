#include <linux/module.h> 
#include <linux/init.h>   
#include <linux/fs.h>   
#include <linux/spi/spi.h> 
#include <asm/uaccess.h>    
#include <linux/device.h>

#define PIFACE_MAJOR 250        //need to be check 

struct piface_data {
        dev_t devt ;
        struct spi_device *spi ;
        struct bin_attribute bin ;
};

char wokao[]="ABCDEFGHIJKLMNOPQRST";
static char string_that_to_be_send[64];

static struct class *piface_class ;

static int piface_open(struct inode *inode , struct file *filp )
{
        printk("Drv info : %s()\n", __FUNCTION__);
        return 0 ;
}

static int piface_release(struct inode *inode , struct file *filp )
{
        printk("Drv info : %s()\n", __FUNCTION__);
        return 0 ;
}

static ssize_t piface_write(struct file *filp ,const char __user *buf , size_t count , loff_t *f_ops )
{
        int err ;
        printk("Drv info : %s()\n", __FUNCTION__);
        err = copy_from_user(string_that_to_be_send+*f_ops , buf ,count );
        printk("%s()-buffer : %s \n",__FUNCTION__ , string_that_to_be_send);
        if(err )
                return -EFAULT ;
        return 0 ;
}

static ssize_t piface_read( struct file *filp , char __user *buf , size_t count , loff_t *f_ops )
{
        int err ;
        printk("Drv info : %s()\n", __FUNCTION__);
        err = copy_to_user(buf,string_that_to_be_send , count );
        printk("%s()-buffer : %s \n",__FUNCTION__ , buf);
        if (err)
                return -EFAULT ;
        return err ;
}

/**********************************************************************/
static ssize_t piface_bin_write(struct file *file , struct kobject *kobj ,
                                                                struct bin_attribute *attr ,
                                                                char *buf , loff_t off , size_t cnt )
{
        struct device *dev ;
        struct piface_data *piface ;
        dev = container_of(kobj, struct device , kobj);
        piface = dev_get_drvdata(dev);
        memcpy(string_that_to_be_send , buf , cnt);
        return 0 ;
}

static ssize_t piface_bin_read(struct file *file , struct kobject *kobj ,
                                                       struct bin_attribute *attr ,
                                                           char *buf , loff_t off , size_t cnt )
{       
        struct device *dev ;
        struct piface_data *piface ;
        int str_length ;
        dev = container_of(kobj, struct device , kobj);
        piface = dev_get_drvdata(dev);
        if( off >= piface->bin.size)
                return 0 ;
        if((off+cnt)>piface->bin.size)
                cnt = piface->bin.size-off ;

        str_length = snprintf(buf , cnt , "%s/n" , string_that_to_be_send);
        return str_length ;
}

/**********************************************************************/

//read 
static ssize_t show_piface(struct device *dev, struct device_attribute *attr,char *buf)
{
        int err ;
        char *dataa = "fuck!this is test\n"; 
        err = snprintf(buf ,PAGE_SIZE , "%s", dataa );
        return err;
}

//write
static ssize_t store_piface(struct device *dev, struct device_attribute *attr,const char *buf, size_t count )
{
        printk("input buf = %s \n" , buf );
//      length = simple_strtol(buf, NULL, 0);
        return count;
}

static DEVICE_ATTR( piface_attr , S_IWUGO | S_IRUGO , show_piface , store_piface);

static const struct file_operations piface_ops = {
        .open = piface_open ,
        .release = piface_release ,
        .write = piface_write ,
        .read = piface_read ,
};

static int piface_probe (struct spi_device *spi)
{
        struct piface_data *piface = NULL;
        unsigned long minor ;
        int err = 0 ;

        printk("Drv info : %s()\n", __FUNCTION__);
        piface = kzalloc(sizeof(*piface), GFP_KERNEL);
        if(!piface)
                return -ENOMEM ;

        piface->spi = spi ;
        
        minor = 0 ;
        piface->devt = MKDEV(PIFACE_MAJOR , minor);
        device_create(piface_class , &spi->dev , piface->devt , 
                                  piface , "piface%d.%d" , spi->master->bus_num , 
                                  spi->chip_select);
        
/**********************************************************************/
        sysfs_bin_attr_init(&piface->bin);
        piface->bin.attr.name = "sys_piface";
        piface->bin.attr.mode = S_IRUGO | S_IWUGO ;
        piface->bin.read = piface_bin_read ;
        piface->bin.write = piface_bin_write ;
        piface->bin.size = 16 ; //need to be update

        err = sysfs_create_bin_file(&spi->dev.kobj , &piface->bin);
        if (err)
                        return err ;

        err = device_create_file( &spi->dev , &dev_attr_piface_attr );
        if (err)
                return err ;

/**********************************************************************/
        spi_set_drvdata(spi , piface);
        printk("Drv info : %s()\n", __FUNCTION__);
        return 0 ;
}

static int piface_remove (struct spi_device *spi)
{
        struct piface_data *piface ;
        piface = spi_get_drvdata(spi);
        device_remove_file( &spi->dev , &dev_attr_piface_attr );
        sysfs_remove_bin_file(&spi->dev.kobj , &piface->bin);
        piface->spi = NULL ;
        spi_set_drvdata(spi , NULL);
        device_destroy(piface_class , piface->devt);
        kfree(piface);
        return 0 ;
}

static struct spi_driver piface_driver = {
        .driver = {
                .name = "piface",
                .owner = THIS_MODULE ,  
        },
        .probe = piface_probe ,
        .remove = piface_remove ,
};

static int __init piface_init(void)
{       int status ;
        status = register_chrdev(PIFACE_MAJOR , "piface" , &piface_ops);
        if (status < 0 )
                return status ;

        piface_class = class_create(THIS_MODULE , "piface");
        if (IS_ERR(piface_class)){
                unregister_chrdev(PIFACE_MAJOR , piface_driver.driver.name);
        }

        printk("hello , Kernel-%s()\n", __FUNCTION__);
        return spi_register_driver(&piface_driver);
        return 0 ;
}

static void __exit piface_exit(void)
{
        printk("Goodbye , Kernel-%s()\n", __FUNCTION__);
        spi_unregister_driver(&piface_driver);
        class_destroy(piface_class);
        unregister_chrdev(PIFACE_MAJOR , piface_driver.driver.name);
}

module_init(piface_init);
module_exit(piface_exit);

MODULE_AUTHOR("hubuyu");
MODULE_LICENSE("GPL");
MODULE_ALIAS("spi:piface");
MODULE_DESCRIPTION("This is SPI device Piface");
