/**
 * @file   ebbcharmutex.c
 * @author Derek Molloy
 * @date   7 April 2015
 * @version 0.1
 * @brief  An introductory character driver to support the second article of my series on
 * Linux loadable kernel module (LKM) development. This module maps to /dev/ebbchar and
 * comes with a helper C program that can be run in Linux user space to communicate with
 * this the LKM. This version has mutex locks to deal with synchronization problems.
 * @see http://www.derekmolloy.ie/ for a full description and follow-up descriptions.
*/

#include <linux/init.h>           // Macros used to mark up functions e.g. __init __exit
#include <linux/module.h>         // Core header for loading LKMs into the kernel
#include <linux/device.h>         // Header to support the kernel Driver Model
#include <linux/kernel.h>         // Contains types, macros, functions for the kernel
#include <linux/fs.h>             // Header for the Linux file system support
#include <linux/uaccess.h>          // Required for the copy to user function
#include <linux/mutex.h>	  // Required for the mutex functionality
#include <linux/moduleparam.h>
#include <linux/stat.h>
#include <linux/string.h>	//biblioteca para a lidar com as strings, mais especificamente copiar a key recebida di insmod para 					//uma variavel dentro da função de cripto

/* Skcipher kernel crypto API */
#include <crypto/skcipher.h>
/* Scatterlist manipulation */
#include <linux/scatterlist.h>
/* Error macros */
#include <linux/err.h>
#include <linux/random.h>

#include <crypto/internal/hash.h>
#include <linux/crypto.h>


#define  DEVICE_NAME "ebbchar"    ///< The device will appear at /dev/ebbchar using this value
#define  CLASS_NAME  "ebb"        ///< The device class -- this is a character device driver

MODULE_LICENSE("GPL");            ///< The license type -- this affects available functionality
MODULE_AUTHOR("Derek Molloy");    ///< The author -- visible when you use modinfo
MODULE_DESCRIPTION("A simple Linux char driver for the BBB");  ///< The description -- see modinfo
MODULE_VERSION("0.1");            ///< A version number to inform users

static int    majorNumber;                  ///< Store the device number -- determined automatically
static char   message[256] = {0};           ///< Memory for the string that is passed from userspace
static short  size_of_message;              ///< Used to remember the size of the string stored
static int    numberOpens = 0;              ///< Counts the number of times the device is opened
static struct class*  ebbcharClass  = NULL; ///< The device-driver class struct pointer
static struct device* ebbcharDevice = NULL; ///< The device-driver device struct pointer
char buffer_out[25];			    ///< Store of the Hash
char encript[32];

static DEFINE_MUTEX(ebbchar_mutex);	    ///< Macro to declare a new mutex


/// The prototype functions for the character driver -- must come before the struct definition
static int     dev_open(struct inode *, struct file *);
static int     dev_release(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static ssize_t dev_write(struct file *, const char *, size_t, loff_t *);

//VAriaveis para o recebimento dos parametros via linha de comando
static char *key = ""; 
module_param(key, charp, 0000); 
MODULE_PARM_DESC(key, "A character string"); 
static char *iv = ""; 
module_param(iv, charp, 0000); 
MODULE_PARM_DESC(iv, "A character string");
//////////////////////////////////////////////////////////////////

/**
 * Devices are represented as file structure in the kernel. The file_operations structure from
 * /linux/fs.h lists the callback functions that you wish to associated with your file operations
 * using a C99 syntax structure. char devices usually implement open, read, write and release calls
 */
static struct file_operations fops =
{
   .open = dev_open,
   .read = dev_read,
   .write = dev_write,
   .release = dev_release,
};

/** @brief The LKM initialization function
 *  The static keyword restricts the visibility of the function to within this C file. The __init
 *  macro means that for a built-in driver (not a LKM) the function is only used at initialization
 *  time and that it can be discarded and its memory freed up after that point.
 *  @return returns 0 if successful
 */
static int __init ebbchar_init(void){
   printk(KERN_INFO "EBBChar: Initializing the EBBChar LKM\n");

   // Try to dynamically allocate a major number for the device -- more difficult but worth it
   majorNumber = register_chrdev(0, DEVICE_NAME, &fops);
   if (majorNumber<0){
      printk(KERN_ALERT "EBBChar failed to register a major number\n");
      return majorNumber;
   }
   printk(KERN_INFO "EBBChar: registered correctly with major number %d\n", majorNumber);

   // Register the device class
   ebbcharClass = class_create(THIS_MODULE, CLASS_NAME);
   if (IS_ERR(ebbcharClass)){           // Check for error and clean up if there is
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to register device class\n");
      return PTR_ERR(ebbcharClass);     // Correct way to return an error on a pointer
   }
   printk(KERN_INFO "EBBChar: device class registered correctly\n");

   // Register the device driver
   ebbcharDevice = device_create(ebbcharClass, NULL, MKDEV(majorNumber, 0), NULL, DEVICE_NAME);
   if (IS_ERR(ebbcharDevice)){          // Clean up if there is an error
      class_destroy(ebbcharClass);      // Repeated code but the alternative is goto statements
      unregister_chrdev(majorNumber, DEVICE_NAME);
      printk(KERN_ALERT "Failed to create the device\n");
      return PTR_ERR(ebbcharDevice);
   }
   printk(KERN_INFO "EBBChar: device class created correctly\n"); // Made it! device was initialized
   mutex_init(&ebbchar_mutex);          // Initialize the mutex dynamically
   return 0;
}

/** @brief The LKM cleanup function
 *  Similar to the initialization function, it is static. The __exit macro notifies that if this
 *  code is used for a built-in driver (not a LKM) that this function is not required.
 */
static void __exit ebbchar_exit(void){
   mutex_destroy(&ebbchar_mutex);                       // destroy the dynamically-allocated mutex
   device_destroy(ebbcharClass, MKDEV(majorNumber, 0)); // remove the device
   class_unregister(ebbcharClass);                      // unregister the device class
   class_destroy(ebbcharClass);                         // remove the device class
   unregister_chrdev(majorNumber, DEVICE_NAME);         // unregister the major number
   printk(KERN_INFO "EBBChar: Goodbye from the LKM!\n");
}

/** @brief The device open function that is called each time the device is opened
 *  This will only increment the numberOpens counter in this case.
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_open(struct inode *inodep, struct file *filep){

   if(!mutex_trylock(&ebbchar_mutex)){                  // Try to acquire the mutex (returns 0 on fail)
	printk(KERN_ALERT "EBBChar: Device in use by another process");
	return -EBUSY;
   }
   numberOpens++;
   printk(KERN_INFO "EBBChar: Device has been opened %d time(s)\n", numberOpens);
   return 0;
}



/** @brief This function is called whenever the device is being written to from user space i.e.
 *  data is sent to the device from the user. The data is copied to the message[] array in this
 *  LKM using message[x] = buffer[x]
 *  @param filep A pointer to a file object
 *  @param buffer The buffer to that contains the string to write to the device
 *  @param len The length of the array of data that is being passed in the const char buffer
 *  @param offset The offset if required
 */

///////INICIO DA ENCRIPTATION///////////////////////////////////////////////////////////////

struct tcrypt_result {
    struct completion completion;
    int err;
};

/* tie all data structures together */
struct skcipher_def {
    struct scatterlist sg;
    struct crypto_skcipher *tfm;
    struct skcipher_request *req;
    struct tcrypt_result result;
};

/* Callback function */
static void test_skcipher_cb(struct crypto_async_request *req, int error)
{
    struct tcrypt_result *result = req->data;

    if (error == -EINPROGRESS)
        return;
    result->err = error;
    complete(&result->completion);
    pr_info("Encryption finished successfully\n");
}

/* Perform cipher operation */
static unsigned int test_skcipher_encdec(struct skcipher_def *sk,
                     int enc)
{
    int rc = 0;

    if (enc)
        rc = crypto_skcipher_encrypt(sk->req);
    else
        rc = crypto_skcipher_decrypt(sk->req);

    switch (rc) {
    case 0:
        break;
    case -EINPROGRESS:
    case -EBUSY:
        rc = wait_for_completion_interruptible(
            &sk->result.completion);
        if (!rc && !sk->result.err) {
            reinit_completion(&sk->result.completion);
            break;
        }
    default:
        pr_info("skcipher encrypt returned with %d result %d\n",
            rc, sk->result.err);
        break;
    }
    init_completion(&sk->result.completion);

    return rc;
}


/* Initialize and trigger cipher operation */
static int test_skcipher(int size, char *varEncript, char option, char *number)
{
    struct skcipher_def sk;
    struct crypto_skcipher *skcipher = NULL;
    struct skcipher_request *req = NULL;
    char *scratchpad = NULL;
    char *ivdata = NULL;
    unsigned char keyC[17];
    int ret = -EFAULT;
  
    skcipher = crypto_alloc_skcipher("cbc(aes)", 0, 0);
    if (IS_ERR(skcipher)) {
        pr_info("could not allocate skcipher handle\n");
        return PTR_ERR(skcipher);
    }

    req = skcipher_request_alloc(skcipher, GFP_KERNEL);
    if (!req) {
        pr_info("could not allocate skcipher request\n");
        ret = -ENOMEM;
        goto out;
    }

    skcipher_request_set_callback(req, CRYPTO_TFM_REQ_MAY_BACKLOG,
                      test_skcipher_cb,
                      &sk.result);

    /* passando a key para a variavel local da função */
	strcpy(keyC, key);
    if (crypto_skcipher_setkey(skcipher, keyC, 16)) {
        pr_info("key could not be set\n");
        ret = -EAGAIN;
        goto out;
    }
    ivdata = vmalloc(16);
    if (!ivdata) {
        pr_info("could not allocate ivdata\n");
        goto out;
    }
    strcpy(ivdata, iv);
    

    /* Input data will be random */
    scratchpad = vmalloc(16);
    if (!scratchpad) {
        pr_info("could not allocate scratchpad\n");
        goto out;
    }
    strcpy(scratchpad, varEncript);

    sk.tfm = skcipher;
    sk.req = req;

    /* We encrypt one block */
    sg_init_one(&sk.sg, scratchpad, 16);
    skcipher_request_set_crypt(req, &sk.sg, &sk.sg, 16, ivdata);
    init_completion(&sk.result.completion);


	if(option == 'e'){
    ret = test_skcipher_encdec(&sk, 1);//1 encripta 0 desencripta
}
	if(option == 'd'){
    ret = test_skcipher_encdec(&sk, 0);//1 encripta 0 desencripta
}
    
    if (ret)
        goto out;
    char *resultdata = sg_virt(&sk.sg);

print_hex_dump(KERN_DEBUG, "texto: ", DUMP_PREFIX_NONE, 16,1, resultdata, 16, true);
strcpy(encript, resultdata);
int w=0;
while(w<strlen(resultdata)){printk(KERN_INFO "Encriptedddd: %x", resultdata[w]); w++;}
    pr_info("Encryption triggered successfully\n");

out:
    if (skcipher)
        crypto_free_skcipher(skcipher);
    if (req)
        skcipher_request_free(req);
    if (ivdata)
        vfree(ivdata);
    if (scratchpad)
        vfree(scratchpad);
    return ret;
}
//////FIM DA ENCRIPTATION////////////////////////////////////////////////////////////////////////////////////


/** @brief This function is called whenever device is being read from user space i.e. data is
 *  being sent from the device to the user. In this case is uses the copy_to_user() function to
 *  send the buffer string to the user and captures any errors.
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 *  @param buffer The pointer to the buffer to which this function writes the data
 *  @param len The length of the b
 *  @param offset The offset if required
 */
//HASHHHHHHHHHHH///

static struct shash_desc *config_sdesc(struct crypto_shash *alg, unsigned int *hash_len)
{
    
    struct shash_desc *sdesc;
    int size;
    *hash_len=crypto_shash_descsize(alg);
    size = sizeof(struct shash_desc) + crypto_shash_descsize(alg);
    sdesc = kmalloc(size, GFP_KERNEL);
    if (!sdesc)
        return ERR_PTR(-ENOMEM);
    sdesc->tfm = alg;
    sdesc->flags = 0x0;
    return sdesc;
}


static int calc_hash(struct crypto_shash *alg,
   const unsigned char *data, unsigned int datalen,
   unsigned char *digest, unsigned int *hash_len){
   struct shash_desc *sdesc2;
   int ret;
   sdesc2 = config_sdesc(alg,hash_len);
   
   if (IS_ERR(sdesc2)) {
       pr_info("can't alloc sdesc\n");
       return PTR_ERR(sdesc2);
   }
   //ret = crypto_shash_digest(&sdesc->shash, data, datalen, digest);
   ret = crypto_shash_digest(sdesc2, data, datalen, digest);
   printk(KERN_INFO "hashj:%s", digest );
   kfree(sdesc2);
   return ret;
}

static int test_hash(const unsigned char *data, unsigned int datalen,
             unsigned char *digest, unsigned int *hash_len)
{
    struct crypto_shash *alg;
    char *hash_tipo = "sha1";
    int ret;

    alg = crypto_alloc_shash(hash_tipo, CRYPTO_ALG_TYPE_SHASH, 0);
    if (IS_ERR(alg)) {
         pr_info("can't alloc alg %s\n", hash_tipo);
         return PTR_ERR(alg);
    }
    //printk(KERN_INFO "passou");
    ret = calc_hash(alg, data, datalen, digest,hash_len);
    crypto_free_shash(alg);
    return ret;
}
//FIM da HASH////

static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset){

int error_count = 0;
   // copy_to_user has the format ( * to, *from, size) and returns 0 on success
   error_count = copy_to_user(buffer, message, size_of_message);

   if (error_count==0){           // success!
      printk(KERN_INFO "EBBChar: Sent %d characters to the user\n", size_of_message);
      return (size_of_message=0); // clear the position to the start and return 0
   }
   else {
      printk(KERN_INFO "EBBChar: Failed to send %d characters to the user\n", error_count);
      return -EFAULT;      // Failed -- return a bad address message (i.e. -14)
   }
}

/************************************/
/*converter para string*/

void hex_to_string( char vet[], char result[] )
{
    int pos=0,i=0,valor=0;
    while(pos<32)
    {

        if(vet[pos]>=97 && vet[pos]<=122)
        {
            valor+=(vet[pos]-87)*16;
        }

        else if(vet[pos]>=65 && vet[pos]<=90)
        {
            valor+=(vet[pos]-55)*16;
        }
        else
        {
            valor+=(vet[pos]-48)*16;
        }
        //-----------------------------------------------------------------------------------------------------------------------------
        if(vet[pos+1]>=97 && vet[pos+1]<=122)
        {
            valor+=(vet[pos+1]-87);
        }
        else if(vet[pos+1]>=65 && vet[pos+1]<=90)
        {
            valor+=(vet[pos+1]-55);
        }
        else
        {
            valor+=(vet[pos+1]-48);
        }
        result[i]=valor;
        valor=0;
        i=i+1;
        pos=pos+2;

    }
    result[i]='\0';

}

void convert_hexa(unsigned char* input, char* output){//string to hex
   int loop=0;
   int i=0;
   while(input[loop] < strlen(input)){
      sprintf((char*)(output+i),"%02X", input[loop]);
      loop+=1;
      i+=2;
   }
   //marking the end of the string
   output[i++] = '\0';
}
/*****************************************/

void converter(char* vet){
int i=0, j=0;
unsigned char num;
while(i<16){
num=(unsigned char)encript[i]/16;
if(num>9) {num= num + 87;}
else{num=num +'0';}
vet[j] = num;
j++;
num=(unsigned char)encript[i]%16;
if(num>9){num =num+ 87;}
else{num= num + '0';}
vet[j]=num;
i++;
j++;
}

}

/************************************************/
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset){
 
size_t lenk;
	int rett;
	int j,i;
 	unsigned int hash_len;
char number[33];
char valor[32];
char string[32];
char option = buffer[0];
char vet[32];
printk(KERN_INFO "O testeee");
int w=2;
int pos=0;
while(w<strlen(buffer)){printk(KERN_INFO "Valor: %c", buffer[w]); valor[pos]=buffer[w]; w++; pos++;}

hex_to_string(buffer+2, string);

int a;
switch(option)
   {
   case 'e':
      a = test_skcipher(16, string, option, number);
	converter(vet);
	sprintf(message, "Encript: %s", vet);

      break;
  case 'd':

      a = test_skcipher(16, string, option, number);
	sprintf(message, "Decript :%s", encript);

      break;
   case 'h':
lenk = strlen(buffer+2);
      printk(KERN_INFO "buffer:%s (tamanho: %d)", buffer+2,lenk);
      rett=test_hash(buffer+2,lenk, buffer_out,&hash_len);
      printk(KERN_INFO "Resposta %d e tamanho:  %d", rett, hash_len);

      j = 0;
         for (i = 0; i < 20; i++){
            sprintf(buffer + j, "%02x", buffer_out[i] & 0xff);
            j += 2;
         }
w=0;
while(w<strlen(buffer)-2){printk(KERN_INFO "Valor da hash: %x", buffer_out[w]); w++;}
sprintf(message, "%s", buffer);   // appending received string with its length
      break;
   default:
      break;
      
   }


  
  // sprintf(message, "%s(%zu letters) %s", buffer, len, valor);   // appending received string with its length
   size_of_message = strlen(message);                 // store the length of the stored message
   printk(KERN_INFO "EBBChar: Received %zu characters from the user\n", len);
   return len;
}

/** @brief The device release function that is called whenever the device is closed/released by
 *  the userspace program
 *  @param inodep A pointer to an inode object (defined in linux/fs.h)
 *  @param filep A pointer to a file object (defined in linux/fs.h)
 */
static int dev_release(struct inode *inodep, struct file *filep){
   mutex_unlock(&ebbchar_mutex);                      // release the mutex (i.e., lock goes up)
   printk(KERN_INFO "EBBChar: Device successfully closed\n");
   return 0;
}

/** @brief A module must use the module_init() module_exit() macros from linux/init.h, which
 *  identify the initialization function at insertion time and the cleanup function (as
 *  listed above)
 */
module_init(ebbchar_init);
module_exit(ebbchar_exit);
