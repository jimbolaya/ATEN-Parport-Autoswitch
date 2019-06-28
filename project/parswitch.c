/* parswitch.c
 * My first attempt to write a parallel port driver (or something)
 */

/* Necessary header files */

#include <linux/module.h>
#include <linux/init.h> 

#include <linux/config.h> 

#include <linux/errno.h> 

#include <linux/kernel.h>  /* *** in sched.h *** */
#include <linux/major.h> 
#include <linux/sched.h>
#include <linux/malloc.h> 
#include <linux/fcntl.h> 
#include <linux/delay.h>

#include <linux/parport.h>

#include <asm/irq.h> 
#include <asm/uaccess.h>
/* #include <asm/system.h> *** in sched.h *** */

#include <linux/fs.h>
#include <linux/wrapper.h>

/* Standard in kernel modules */

#include <linux/param.h>  /* don't know */
/* Deal with CONFIG_MODVERSIONS */
/* #if CONFIG_MODVERSIONS==1
#define MODVERSIONS
#include <linux/modversions.h>
#endif *** It causes too much trouble **** */

#ifndef MAX_DEVS
#define MAX_DEVS 3
#endif

#ifndef NUM_PORTS
#define NUM_PORTS 5
#endif

#ifndef BUFSIZE
#define BUFSIZE 1
#endif

/* initialize the module */

/* #define PARSWITCH_MAJOR 130 */

const char *mydevname = "parswitch";
static int parswitch_major = 0;
/* static int parswitch_minor = 0; */
static int parswitch_counter [ NUM_PORTS ];
struct parport *myport;
struct pardevice *mydevice[MAX_DEVS];
struct wait_queue *mywaitqueue;

void switchport ( int subport )
{
    char portswitch = 0;
    
    mydevice[0]->port->ops->frob_control 
      (mydevice[0]->port, PARPORT_CONTROL_SELECT, 0);

    mydevice[0]->port->ops->write_data(mydevice[0]->port, 0x04);
    mydevice[0]->port->ops->write_data(mydevice[0]->port, 0x84);
    mydevice[0]->port->ops->write_data(mydevice[0]->port, 0x05);
    mydevice[0]->port->ops->write_data(mydevice[0]->port, 0x85);

    switch ( subport ) 
    {  /* make this an enumerated type */
      case 0: /* Broadcast */
         portswitch = 0x01;
         break;
      case 1: /* port A */
         portswitch = 0x06;
         break;
      case 2: /* port B */
         portswitch = 0x07;
         break;
      case 3: /* port C */
         portswitch = 0x02;
         break;
      case 4: /* port D */
         portswitch = 0x03;
         break;
    }
    

    mydevice[0]->port->ops->write_data(mydevice[0]->port, portswitch);
    mydevice[0]->port->ops->write_data(mydevice[0]->port, (portswitch | 0x80) );
    
    mydevice[0]->port->ops->frob_control 
      (mydevice[0]->port, PARPORT_CONTROL_SELECT,  PARPORT_CONTROL_SELECT);

    return; /* cannot fail */
    
}

static void parswitch_attach (struct parport *port, int devnum)
{
    if(devnum > MAX_DEVS)
      {
        printk("Maximum of MAX_DEVS devices allowed at this time.\n");
        return;
      }
    mydevice[devnum] = parport_register_device(port, mydevname, NULL, NULL, NULL, 0, NULL);
    /* printk("Attached to port.\n"); */
}

static int parswitch_open(struct inode *inode, struct file *file)
{

    int major, minor;
    int i;

    MOD_INC_USE_COUNT; /* This increments the use counter for this module  This module
			  will not be unloaded until this is zero */
    
    
#ifdef DEBUG
    printk("parswitch_open (%p, %p)\n", inode,file);
#endif
    
    major = inode->i_rdev >> 8;
    minor = inode->i_rdev & 0xFF;
    
#ifdef DEBUG    
    printk("Parswitch: %d.%d\n", major, minor);
#endif
    /*  The following awful hack is to make sure that if we are in broadcast mode
	then we want to mark all the ports as busy.  Other wise we don't want to 
	go into broadcast mode, because we are already using another port */

    if ( minor == 0 )
      {
	for ( i=0; i<NUM_PORTS ;i ++)
	  {
	    if ( parswitch_counter[i] )
	      {
		MOD_DEC_USE_COUNT;
		return -EBUSY;
	      }
	  }
      }
    else 
      {
	if ( parswitch_counter[minor] || parswitch_counter[0] )
	      {
		MOD_DEC_USE_COUNT;
		return -EBUSY;
	      }
      }

     parswitch_counter[minor] ++;

    return 0;
    
} 


/* non zero return means error */
/* This function is called when somebody tries to write into our
device file. */

static ssize_t parswitch_write(struct file * file, const char * buf, size_t count, loff_t *ppos)
{
    unsigned int minor;
    int curcount = 0;
    int success = 0, i, j, k, mydelay;
    unsigned char chunk, my_status;
    unsigned int command_set;
    
    command_set = PARPORT_CONTROL_INTEN | PARPORT_CONTROL_SELECT | PARPORT_CONTROL_INIT ;
    minor = MINOR(file->f_dentry->d_inode->i_rdev);
    
#ifdef DEBUG
    printk ("\n\n\ndevice_write(%p,%s,%d)\n\n\n",    file, buf, count);
#endif
    
    /* Now lets claim the first port since I am not actually putting this switch on more than one port now */
    
    
    
    while (curcount <  count )
    {
        do
        {
            success = parport_claim_or_block(mydevice[0]); 
            
        } while( success == -EAGAIN ); 
        
        
        /* if(success == 0)
            printk("Successfully claimed the port for minor=%d\n", minor);
        else if (success > 0)
            printk("Successfully claimed the port.  It was blocked for a time.\n");
        else 
            printk("Could not claim the port even after blocking.\n");
	*/

        switchport( minor );

	mydelay = 1;
        for ( i = 0; ((i < BUFSIZE) && (curcount < count)) ; i++)
	{
	    if( get_user(chunk,  buf + curcount))
                return -EFAULT;
	    /* printk("get_user(%c, %i)\n", chunk ,buf + curcount); */
	    /* udelay(mydelay * 4000);
	       mydevice[0]->port->ops->write_control(mydevice[0]->port, command_set);
	       /* Send the byte of info to the pins */
	    /* mydevice[0]->port->ops->write_data(mydevice[0]->port, chunk); */
  	    k = 0;
	    /* Check for Busy low before strobing */
	    while ( parport_wait_peripheral( mydevice[0]->port , PARPORT_STATUS_BUSY|PARPORT_STATUS_ERROR,  \
					      PARPORT_STATUS_BUSY|PARPORT_STATUS_ERROR) )
	      {
		while ( parport_wait_peripheral( mydevice[0]->port , PARPORT_STATUS_BUSY|PARPORT_STATUS_ERROR,  \
						 PARPORT_STATUS_BUSY|PARPORT_STATUS_ERROR) && k < mydelay)
		  {
		    k++;
		  }
		k = 0;
		if (signal_pending(current)) 
		  return curcount;
		schedule_timeout(jiffies+HZ);
		
	       } 
	     
	     if (chunk == '\n')
	       {
		 mydelay = 500;
	       }
	     else
	       {
		 mydelay = 100;
	       }
	     
	     switchport( minor );
	     
	     mydevice[0]->port->ops->write_control(mydevice[0]->port, command_set);
	     
	     /* Send the byte of info to the pins */
	     mydevice[0]->port->ops->write_data(mydevice[0]->port, chunk);
	     
	     /* if(parport_wait_peripheral( mydevice[0]->port , PARPORT_STATUS_BUSY|PARPORT_STATUS_ERROR,  \
		PARPORT_STATUS_BUSY|PARPORT_STATUS_ERROR) )
		schedule_timeout(jiffies+HZ); */
	     
	     
	     switchport( minor );
	     
	     mydevice[0]->port->ops->write_control(mydevice[0]->port, command_set);
	     
	     /* Send the byte of info to the pins */
	     mydevice[0]->port->ops->write_data(mydevice[0]->port, chunk);
	     


	     /* Strobe */
	     mydevice[0]->port->ops->write_control(mydevice[0]->port, command_set | PARPORT_CONTROL_STROBE);
	     udelay(1);
	     mydevice[0]->port->ops->write_control(mydevice[0]->port, command_set);
	     udelay(1);
	     
	     /* udelay( 300 );   */
	     
	     /* do { 
		my_status = mydevice[0]->port->ops->read_status(mydevice[0]->port);
		printk("waiting for busy low");
		}while  (  (my_status & PARPORT_STATUS_BUSY) ); 
		
		do {
		my_status = mydevice[0]->port->ops->read_status(mydevice[0]->port);
		printk("waiting for ack");
		}
		while( !( my_status & PARPORT_STATUS_ACK )); */

	   
	     if (signal_pending(current)) 
	       return curcount;
	     
	     /* current->state = TASK_UNINTERRUPTIBLE; */
	     

	     schedule_timeout(jiffies+HZ);
	    

	     curcount ++;
	     
	} 
	/* printk ("mydelay = %i\n", mydelay); */
        parport_release(mydevice[0]);
        /* success = parport_yield (mydevice[0]);  */
    }
  
  
  
  /* Again, return the number of input characters used */
  return curcount;
}


static void parswitch_release( struct inode *inode, struct file *file)
{
    int minor;
    
    minor = inode->i_rdev & 0xFF;
    
    parswitch_counter [minor] --;

    MOD_DEC_USE_COUNT;
    
    return;
    
}


struct file_operations parsw_fops =
{
    NULL,  /* seek */
    NULL,  /* read */
    parswitch_write,  /* write */
    NULL,  /* readdir */
    NULL,  /* select */
    NULL,  /* ioctl */
    NULL,  /* mmap */
    parswitch_open,
    NULL,  /* flush */
    parswitch_release
};

int init_module()
{
                                            /* PARSWITCH_MAJOR */
    parswitch_major = module_register_chrdev(0, mydevname, &parsw_fops);
      
    if (parswitch_major < 0)
    {
        printk( "Device failed device number registration.\n");
        return parswitch_major;
    }
    
    /* printk( " Managed to get device number %d.\n", parswitch_major);
    
       printk("Attempting to get a port and register a device on it.\n"); */
    myport = parport_enumerate();

    /* Lets attach our driver only to the first port */

    if ( myport != NULL )
    {
        parswitch_attach(myport,0);
    }

    return 0;
}


/* clean up the module */
void cleanup_module()
{
    int i;
    
    /* printk("Short is the life of a kernel module\n");
       printk("major device # %d \n", parswitch_major); */
    
    
    i = module_unregister_chrdev(parswitch_major, mydevname);
    if (i < 0 )
        printk ("Error in unregister_chrdev: %d\n", i);

    /* Stop notifications */
    /* Not yet, this is for 2.3/2.4 parport_unregister_driver(&mydriver); */
    
    /* Unregister devices */

    parport_unregister_device (mydevice[0]);
	
    
}

