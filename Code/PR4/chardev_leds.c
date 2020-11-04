//Beattriz Herguedas Pinedo

// Bibliotecas del kernel

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <asm/uaccess.h>	/* for copy_to_user */
#include <linux/cdev.h>
#include <asm-generic/errno.h>
#include <linux/init.h>
#include <linux/tty.h> 
#include <linux/kd.h> 
#include <linux/vt_kern.h>
#include <linux/version.h> 


MODULE_LICENSE("GPL");

//Declaracion de las funciones
int init_module(void);
void cleanup_module(void);
static int device_open(struct inode *, struct file *);
static int device_release(struct inode *, struct file *);
static ssize_t device_read(struct file *, char *, size_t, loff_t *);
static ssize_t device_write(struct file *, const char *, size_t, loff_t *);

//Constantes
#define SUCCESS 0
//Nombre del dispositivo que creamos
#define DEVICE_NAME "chardev_leds"	/* Dev name as it appears in /proc/devices   */
#define BUF_LEN 80

//Par major minor (dev_t)
dev_t start;


struct cdev* chardev=NULL;

//Para saber si el fichero esta abierto(0 no 1 si)
static int Device_Open = 0;

//static char leidos[BUF_LEN];

//Puntero a por donde va leido el mensaje
static char *leidos_Ptr = NULL;

//Operaciones (para redireccionarlas)
static struct file_operations fops = {
    .read = device_read,
    .write = device_write,
    .open = device_open,
    .release = device_release
};

//Puntero al manejador (lo que controla los leds)
struct tty_driver* kbd_driver= NULL;

//Funcion que devuelve el manejador
struct tty_driver* get_kbd_driver_handler(void)
{
    printk(KERN_INFO "modleds: loading\n");
    printk(KERN_INFO "modleds: fgconsole is %x\n", fg_console);
#if ( LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32) )
    return vc_cons[fg_console].d->port.tty->driver;
#else
    return vc_cons[fg_console].d->vc_tty->driver;
#endif
}

//Funcion que pinta en los leds lo que pone en la mascara
static inline int set_leds(struct tty_driver* handler, unsigned int mask)
{
#if ( LINUX_VERSION_CODE > KERNEL_VERSION(2,6,32) )
    return (handler->ops->ioctl) (vc_cons[fg_console].d->port.tty, KDSETLED,mask);
#else
    return (handler->ops->ioctl) (vc_cons[fg_console].d->vc_tty, NULL, KDSETLED, mask);
#endif
}

//Funcion a la que se llama cuando se inserta el modulo (al hacer insmod)
int init_module(void)
{
    int major;		/* Major number assigned to our device driver */
    int minor;		/* Minor number assigned to the associated character device */
    int ret;

    /* Get available (major,minor) range */
    // 0 es el minor que queremos que tenga (con el major que pueda)
    // 1 es la cantidad de minors que reservamos
    if ((ret=alloc_chrdev_region (&start, 0, 1,DEVICE_NAME))) {
        printk(KERN_INFO "Can't allocate chrdev_region()");
        return ret;
    }

    /* Create associated cdev */
    if ((chardev=cdev_alloc())==NULL) {
        printk(KERN_INFO "cdev_alloc() failed ");
        unregister_chrdev_region(start, 1);
        return -ENOMEM;
    }

    //Le asocia las operaciones al dispositivo
    cdev_init(chardev,&fops);

    //Al dispositivo le asociamos el major y el minor
    if ((ret=cdev_add(chardev,start,1))) {
        printk(KERN_INFO "cdev_add() failed ");
        kobject_put(&chardev->kobj);
        unregister_chrdev_region(start, 1);
        return ret;
    }

    //Llamada para obtener el manejador
    kbd_driver= get_kbd_driver_handler();

    major=MAJOR(start);
    minor=MINOR(start);

    //Escribe en un fichero log toda la info (major, minor, etc)
    printk(KERN_INFO "I was assigned major number %d. To talk to\n", major);
    printk(KERN_INFO "the driver, create a dev file with\n");
    //Este es el mensaje importante
    //sudo para poder acceder en root
    //mknod para crear un fichero
    //-m para poner la mascara
    //666 es la mascara normal (lectura y escritura) (4 lectura, 2 escritura y 1 ejecutar, 3 veces: propietario, grupo, usuario)
    //c porque es un dispositivo de caracteres y no de bloques
    printk(KERN_INFO "'sudo mknod -m 666 /dev/%s c %d %d'.\n", DEVICE_NAME, major,minor);
    printk(KERN_INFO "Try to cat and echo to the device file.\n");
    printk(KERN_INFO "Remove the device file and module when done.\n");

    return SUCCESS;
}

//Funcion que borra el modulo
void cleanup_module(void)
{
    /* Destroy chardev */
    if (chardev)
        cdev_del(chardev);

    //Deja los leds apagados
    set_leds(kbd_driver,0);
    /*
     * Unregister the device
     */
    unregister_chrdev_region(start, 1);
}

//Funcion que abre el dispositivo
static int device_open(struct inode *inode, struct file *file)
{
    if (Device_Open){
    	printk(KERN_ALERT "El dispositivo ya esaba abierto");
        return -EBUSY;
    }

    //Variable abierto a 1
    Device_Open++;
    printk(KERN_INFO "Se acaba de abrir el dispositivo\n");

    /* Increase the module's reference counter */
    //Contador interno de las personas que han abierto el dispositivo
    try_module_get(THIS_MODULE);

    return SUCCESS;
}

//Funcion que cierra el dispositivo
static int device_release(struct inode *inode, struct file *file)
{
    Device_Open--;		/* We're now ready for our next caller */

    /*
     * Decrement the usage count, or else once you opened the file, you'll
     * never get get rid of the module.
     */
    module_put(THIS_MODULE);
    printk(KERN_INFO "El dispositivo se acaba de cerrar\n");

    return 0;
}

//Operacion no soportada
static ssize_t device_read(struct file *filp,	/* see include/linux/fs.h   */
                           char *buffer,	/* buffer to fill with data */
                           size_t length,	/* length of the buffer     */
                           loff_t * offset)
{
    printk(KERN_ALERT "Sorry, this operation isn't supported.\n");
    return -EPERM;
}

//Para escribir en el dispositivo
//filp es el descriptor del fichero
//buff es el buffer con lo que tiene que escribir (cadena de caracteres)
//len es la longitud de lo que tiene que escribir
//off es el desplazamiento (desde donde tiene que empezar a escribir)
static ssize_t device_write(struct file *filp, const char *buff, size_t len, loff_t * off){
    int bytes_to_write = len;

    /*if(bytes_to_write > strlen(buff))
    	bytes_to_write = strlen(buff);*/

   	printk(KERN_INFO "Longitud: %d\n", len);

    printk(KERN_INFO "Estamos en el write\n");

    //Array del kernel de la longitud del mensaje que queremos escribir
    //es necesario porque el array que llega es del usuario y queremos escribir en el kernel
    //(uno no tiene por que tener acceso a cosas del otro)
    leidos_Ptr = (char *) vmalloc(len);

    //Copiamos de buffer en leidos_ptr
    if(copy_from_user(leidos_Ptr, buff, bytes_to_write))
    	return -EFAULT;

    //Inicializamos la mascara a 0
    unsigned int mask = 0;

    //Ponemos los 3 bits a 0
    int encender_numLock = 0;
    int encender_capsLock = 0;
    int encender_scrollLock = 0;

    //Recorremos el mensaje entero para ver que leds hay que encender
    //orden de los leds en el lab: CNS
    int i = 0;
    while (i < len){
    	if (*leidos_Ptr == '1'){
    		encender_numLock = 2;
    	}
    	else if (*leidos_Ptr == '2'){
    		encender_capsLock = 4;
    	}
    	else if (*leidos_Ptr == '3'){
    		encender_scrollLock = 1;
    	}
    	else{
    		//Informar de error de numero no permitido
    		//(o no haria faltra, no enciende nada y ya)
    	}
    	printk(KERN_INFO "Leido: %c \n", *leidos_Ptr);
    	leidos_Ptr++;
    	i++;
    }

    //La mascara es la suma de los 3
    mask = encender_numLock+encender_capsLock+encender_scrollLock;
    printk(KERN_INFO "Mascara: %d \n", mask);

    //Enciende los leds
    set_leds(kbd_driver,mask);

    //Devolvemos los bytes que hemos podido escribir
    return bytes_to_write;
}