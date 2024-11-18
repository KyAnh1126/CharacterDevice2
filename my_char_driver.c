#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <asm/ioctl.h>
#include <linux/sched.h>
#include <linux/mutex.h>

#define MY_MAJOR 42
#define MY_MAX_MINORS 5
#define MY_DEV_NAME "my_dev_driver"
#define MY_CLASS_NAME "my_dev_class"
#define MY_BUF_SZ 256

#define MY_IOCTL_IN _IOC(_IOC_WRITE, 'k', 1, sizeof(my_ioctl_data));

MODULE_LICENSE("GPL");           
MODULE_AUTHOR("Ky Anh");    
MODULE_DESCRIPTION("A simple Linux char driver"); 
MODULE_VERSION("0.1");  

static struct class* my_dev_class;

struct my_device_data {
    struct cdev cdev; // struct chứa thông tin file device (đường dẫn, major number, minor number,...)
    size_t size; // size của buffer
    char buffer[MY_BUF_SZ];
    wait_queue_head_t wq; // queue các thread vào trạng thái chờ 
    int flag; // điều kiện để thread được thực thi
    struct mutex my_mutex; // dùng thêm mutex để đồng bộ hóa luồng
};

// tạo 5 device files tương ứng với các thiết bị con của driver này
struct my_device_data devs[MY_MAX_MINORS];

static int my_open(struct inode*, struct file*);
static ssize_t my_read(struct file*, char*, size_t, loff_t*);
static ssize_t my_write(struct file*, const char*, size_t, loff_t*);
static int my_release(struct inode*, struct file*);
static long my_ioctl(struct file*, unsigned int, unsigned long);

static const struct file_operations my_fops = {
    .owner = THIS_MODULE,
    .open = my_open,
    .read = my_read,
    .write = my_write,
    .release = my_release,
    .unlocked_ioctl = my_ioctl,
};

static int my_open(struct inode* inode, struct file* file) {
    printk(KERN_INFO "CHECK open!\n");
    struct my_device_data* my_data;
    my_data = container_of(inode->i_cdev, struct my_device_data, cdev);
    mutex_lock(&(my_data->my_mutex)); // đồng bộ hóa tiến trình, ngăn 2 thread được đánh thức và thực thi cùng một lúc khi chứa gán flag = 0
    wait_event_interruptible(my_data->wq, my_data->flag != 0); // nếu flag != 0 -> được thực thi và ngược lại
    printk(KERN_INFO "GO open!\n");
    my_data->flag = 0; // gán flag = 0, ngăn tiến trình sau đó được thực thi nếu tiến trình này chưa release
    mutex_unlock(&(my_data->my_mutex)); // unlock mutex
    my_data->size = MY_BUF_SZ;
    file->private_data = my_data;
    return 0;
}

// hàm my_read: copy buffer của driver đến buffer của user, offset trỏ tới vị trí hiện tại của file KHI ĐỌC
// hàm my_write: copy buffer của user đến buffer của driver, offset trỏ tới vị trí hiện tại KHI GHI
// offset của my_read và my_write là 2 offset khác nhau, vì vậy cần open file 2 lần (với 2 chế đô đọc và ghi) khác nhau để không đưa ra kết qủa không chính xác

static ssize_t my_read(struct file* file, char* user_buffer, size_t size, loff_t *offset) {
    printk(KERN_INFO "GO read!\n");
    struct my_device_data *my_data;
    my_data = (struct my_device_data*) file->private_data;

    ssize_t len = min(my_data->size - *offset, size);
    if(len <= 0) {
        return 0;
    }

    // printk(KERN_INFO "my_data->size: %zu, offset: %lld, size: %zu, len: %zu\n", my_data->size, *offset, size, len);
    // printk(KERN_INFO "my_data->buffer: %s\n", my_data->buffer);

    if(copy_to_user(user_buffer, my_data->buffer + *offset, len)) {
        return -EFAULT;
    }
    *offset += len;
    return len;
}

static ssize_t my_write(struct file* file, const char* user_buffer, size_t size, loff_t* offset) {
    printk(KERN_INFO "GO write!\n");
    struct my_device_data *my_data;
    my_data = (struct my_device_data*) file->private_data;

    ssize_t len = min(my_data->size - *offset, size);
    if(len <= 0) {
        return 0;
    }

    // printk(KERN_INFO "my_data->size: %zu, offset: %lld, size: %zu, len: %zu\n", my_data->size, *offset, size, len);

    if(copy_from_user(my_data->buffer + *offset, user_buffer, len)) {
        return -EFAULT;
    }   

    // printk(KERN_INFO "my_data->buffer: %s\n", my_data->buffer);

    *offset += len;
    return len;
}

static int my_release(struct inode* inode, struct file* file) {
    printk(KERN_INFO "GO release!\n");
    struct my_device_data *my_data;
    my_data = (struct my_device_data*) file->private_data;

    my_data->flag = 1; // gán flag = 1, cho phép các luồng khác được thực thi
    wake_up_interruptible(&(my_data->wq)); // đánh thức các luồng đang trong hàng đợi wq
    return 0;
}

// hàm my_ioctl dùng để thực thi các thao tác đặc biệt tới thiết bị ngoài read và write
static long my_ioctl(struct file* file, unsigned int cmd, unsigned long arg) {
    // struct my_device_data* my_data;
    // my_data = (struct my_device_data*) file->private_data;
    // my_ioctl_data mid;

    // switch(cmd) {
    //     case MY_IOCTL_IN: {
    //         // copy buffer from user to mid -> allow access to buffer of user
    //         if(copy_from_user(&mid, (my_ioctl_data*)arg, sizeof(my_ioctl_data))) {
    //             return -EFAULT;
    //         }
    //         // process data, execute cmd
    //         break;
    //     }
    //     default: {
    //         return -ENOTTY;
    //     }
    //     return 0;
    // }
    return 0;
}

static int my_init_module(void) {
    printk(KERN_INFO "GO init!\n");
    int err;
    dev_t dev = MKDEV(MY_MAJOR, 0);
    err = register_chrdev_region(dev, MY_MAX_MINORS, MY_DEV_NAME);
    // chỉ đăng ký vùng các major và minor number (0 -> MY_MAX_MINORS - 1) cho MY_DEV_NAME, chưa tạo device file
    if(err != 0) {
        return err;
    }

    printk(KERN_INFO "err: %d\n", err);
    
    my_dev_class = class_create(MY_CLASS_NAME);
    if(IS_ERR(my_dev_class)) {
        unregister_chrdev_region(dev, MY_MAX_MINORS);
        return PTR_ERR(my_dev_class);
    }

    for(int i = 0; i < MY_MAX_MINORS; ++i) {
        cdev_init(&devs[i].cdev, &my_fops); // khởi tạo cdev, gán my_fops
        cdev_add(&devs[i].cdev, MKDEV(MY_MAJOR, i), 1); // gán major number và minor number cho cdev của device
        device_create(my_dev_class, NULL, MKDEV(MY_MAJOR, i), NULL, "my_dev_%d", i); // ví dụ: tạo device file với đường dẫn /dev/my_dev_0 
        memset(&devs[i].buffer, 0, MY_BUF_SZ);
        devs[i].flag = 1;
        init_waitqueue_head(&(devs[i].wq));
        mutex_init(&(devs[i].my_mutex));
    }

    return 0;
}

static void my_cleanup_module(void) {
    printk(KERN_INFO "GO clean!\n");
    for(int i = 0; i < MY_MAX_MINORS; ++i) {
        dev_t dev = MKDEV(MY_MAJOR, i);
        device_destroy(my_dev_class, dev);
        cdev_del(&devs[i].cdev);
    }
    class_destroy(my_dev_class);
    unregister_chrdev_region(MKDEV(MY_MAJOR, 0), MY_MAX_MINORS);
}

module_init(my_init_module); 
module_exit(my_cleanup_module); 

// inode structure: thông tin file được lưu trữ ở ổ cứng, được copy tới v-node tại RAM
// inode: static (class), unchanged
// file: dynamic (object), changed 

// major: id of driver, minor: id of physical device of driver
