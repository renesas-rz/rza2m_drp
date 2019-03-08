#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/fs.h>
#include <linux/interrupt.h>
#include <linux/dma-mapping.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/wait.h>

#include "drp_iodefine.h"
#include "drp_linux.h"
#include "r_dk2_if.h"

//#define DEBUG

struct drp_priv {
	struct platform_device *pdev;
	void __iomem *base;
	const char *dev_name;
	void (*irq_handler) (uint32_t);
	struct mutex lock;

	uint8_t init_called;	/* We can only call R_DK2_Initialize once */

	wait_queue_head_t wait;
};
struct drp_priv *my_priv;	/* for functions called that are not Linux aware */

void __iomem *drpk_base_addr;

struct config_data {
	void		*virt;
	uint32_t	phys;
	uint32_t	size;
	uint32_t	copied_from;
};
#define MAX_CONFIG 32
static struct config_data configs[MAX_CONFIG];

/* Cache Coherent Buffers for DRP HW */
typedef uint8_t work_load__t[R_DK2_TILE_NUM][16];
typedef uint8_t work_start_t[R_DK2_TILE_NUM][32];
work_load__t *work_load_ptr;
work_start_t *work_start_ptr;
#define WORK_AREA_SIZE ((R_DK2_TILE_NUM * 16) + (R_DK2_TILE_NUM * 32)) /* 288 bytes */
void *work_buffer;		/* A cache coherent buffer for DRP HW work area */
dma_addr_t work_phys;		/* The physical address of the allocated memory */
#define PARAMS_SIZE (R_DK2_TILE_NUM * MAX_ARG_SIZE)
void *params_buffer;		/* A cache coherent buffer to hold passed parameters */
dma_addr_t params_phys;		/* the physical address of the allocated memory */

#define DRP_NOT_FINISH  (0)
#define DRP_FINISH      (1)
static uint8_t drp_lib_id[R_DK2_TILE_NUM];
static volatile uint8_t drp_lib_status[R_DK2_TILE_NUM];

uint8_t finish_status;	/* each bit represents a TILE status */

static void cb_drp_finish(uint8_t id)
{
	uint32_t tile_no;

	/* Change the operation state of the DRP library notified by the argument to finish */
	for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
	{
		if (drp_lib_id[tile_no] == id)
		{
			drp_lib_status[tile_no] = DRP_FINISH;
			finish_status |= 1 << tile_no;
			break;
		}
	}

	return;
}

static irqreturn_t drp_interrupt(int irq, void *dev_id)
{
	struct drp_priv *priv = dev_id;

	priv->irq_handler(0);

	//wake_up_interruptible(&priv->wait);
	wake_up(&priv->wait);

	return IRQ_HANDLED;
}

void DRP_CLOCK_ON(void)
{
#if 0 /* Clock should already be on */
	
	/* DRP Clock on */
	CPG.STBCR9.BIT.MSTP90 = 0;
	dummy_buf = CPG.STBCR9.BYTE; /* Dummy read */

	/* DRP standby out */
	wait_count = 0;
	CPG.STBREQ2.BIT.STBRQ24 = 0;
	while(0 != CPG.STBACK2.BIT.STBAK24)
	{
		if (STANBYOUT_WAIT_COUNT < wait_count)
		{
			result = R_DK2_ERR_STATUS;
			goto func_end;
		}
		wait_count++;
	}
#endif
}


static DEFINE_SPINLOCK(ttbr_lock);
void ttbr_lookup(uint32_t vaddress, uint32_t * paddress )
{
	uint32_t pa;
	uint32_t va;
	unsigned long flags;

	/* Mask 4k PAGE boundary */
	va = vaddress & 0xFFFFF000;

	spin_lock_irqsave(&ttbr_lock, flags);
	asm volatile("\t mcr p15, 0, %0, c7, c8, 1\n"
		: 			/* no output */
		: "r"(va)		/* input */
		);
	asm volatile("\t isb\n");
	asm volatile("\t mrc p15, 0, %0, c7, c4, 0\n"
		: "=r" (pa)		/* output */
		:			/* no input */
		);
	spin_unlock_irqrestore(&ttbr_lock, flags);

	/* Remove low bits that are not address */
	pa &= 0xFFFFF000;
	
	/* Add page offset back in */
	pa |= vaddress & 0xFFF;

	*paddress= pa;
}

/******************************************************************************
* Function Name: R_MMU_VAtoPA
* Description  : Convert virtual address to physical address
* Arguments    : uint32_t   vaddress  ; I : virtual address to be convert
*              : uint32_t * paddress  ; O : physical address
* Return Value : MMU_SUCCESS         : successful
*                MMU_ERR_TRANSLATION : overflow of virtual or physical area
******************************************************************************/
e_mmu_err_t R_MMU_VAtoPA( uint32_t vaddress, uint32_t * paddress )
{
	e_mmu_err_t err = 0; /* 0=MMU_SUCCESS, -2=MMU_ERR_TRANSLATION */
	struct page * my_page;
	uint32_t offset;
	phys_addr_t paddr;
	int i;

	if (vaddress < 0xBF000000) {
		/* Not a virtual address
		 * (they passed a physical RAM address) */
		*paddress = vaddress;
		goto out;
	}

	/* 0xBF000000 - 0xBFFFFFFF: Static driver variable */
	if (vaddress < 0xC0000000) {
		ttbr_lookup(vaddress, paddress);
		goto out;
	}

	/* ioremapped DRP config data area */
	for (i=0; i < MAX_CONFIG; i++) {
		if (configs[i].phys == 0)
			break;

		if (	(vaddress >= (uint32_t)configs[i].virt) && 
			(vaddress < ((uint32_t)configs[i].virt + configs[i].size))) {

			*paddress = (vaddress - (uint32_t)configs[i].virt) + configs[i].phys;
			goto out;
		}
	}

	/* DMA buffer DRP Work Area */
	if ((vaddress >= (uint32_t)work_load_ptr) && 
	    (vaddress < ((uint32_t)work_load_ptr + WORK_AREA_SIZE))) {
		*paddress = (vaddress - (uint32_t)work_load_ptr) + work_phys;
		goto out;
	}

	/* DMA buffer parameters */
	if ((vaddress >= (uint32_t)params_buffer) && 
	    (vaddress < ((uint32_t)params_buffer + PARAMS_SIZE))) {
		*paddress = (vaddress - (uint32_t)params_buffer) + params_phys;
		goto out;
	}


	/* (VMALLOC_START (0xC1000000) - VMALLOC_END (0xFF800000) */
	if ((vaddress >= VMALLOC_START) && (vaddress <VMALLOC_END)) {

		printk("%s: can't translate address\n", __func__);
		err = -2;
		goto out;

	} else {
		/* 0xC0000000 - 0xCxxxxxxx: Allocated on heap or stack */
		my_page = virt_to_page((void *)vaddress);
		offset = offset_in_page(vaddress);
		paddr = page_to_phys(my_page) + offset;
		*paddress= paddr;
	}

out:
#ifdef DEBUG
	printk("%s: vaddress=%08X, paddress=%08X\n", __func__, vaddress, *paddress);
#endif
	return err;
}

/**
 * @brief      Sets the interrupt priority level
 *
 * @param[in]  int_id : Interrupt ID (0 - 511)
 * @param[in]  priority : Interrupt priority level (0 - 31)
 * @retval     INTC_SUCCESS : SUCCESS
 * @retval     INTC_ERR_INVALID_ID : Invalid interrupt ID specified by argument
 * @retval     INTC_ERR_INVALID_PRIORITY : Invalid interrupt priority level specified by argument
 */
e_r_drv_intc_err_t R_INTC_SetPriority( e_r_drv_intc_intid_t int_id, e_r_drv_intc_priority_t priority )
{
	return 0;
}

/**
 * @brief      Registers the function specified by the func\n
 *             to the element specified by the int_id \n
 *             in the INTC interrupt handler function
 *
 * @param[in]  int_id : Interrupt ID (0 - 512)
 * @param[in]  void (*func)(uint32_t) : Function to be registered to INTC interrupt hander table
 * @retval     INTC_SUCCESS : SUCCESS
 * @retval     INTC_ERR_INVALID : Error End (Illegal parameters)
 */
e_r_drv_intc_err_t R_INTC_RegistIntFunc(e_r_drv_intc_intid_t int_id, void (* func)(uint32_t int_sense))
{
	my_priv->irq_handler = func;

	return 0;
}

/**
 * @brief      Enables interrupt
 *
 * @param[in]  int_id : Interrupt ID (0 - 511)
 *
 * @retval     INTC_SUCCESS : SUCCESS
 * @retval     INTC_ERR_INVALID_ID : Invalid interrupt ID specified by argument
 */
e_r_drv_intc_err_t R_INTC_Enable( e_r_drv_intc_intid_t int_id )
{
	return 0;
}

/**
 * @brief Creates a mutex and returns a pointer to it. 
 * 
 * @retval p_mutex: Pointer to mutex created.
 * @retval NULL If mutex creation fails.
 */
void* R_OS_MutexCreate(void)
{
	mutex_init(&my_priv->lock);
	return &my_priv->lock;
}

/**
 * @brief Attempts to claim mutex for 'timeout' length, will fail if not possible.
 * If mutex passed is NULL, this function will create new mutex.
 * 
 * @param[in] pp_mutex: Address of mutex object to acquire. 
 * @param[in] time_out: Length of Time to wait for mutex.
 *  
 * @retval TRUE Mutex is acquired
 * @retval FALSE Wait Timed out, mutex not acquired.
 */
bool_t R_OS_MutexWait(pp_mutex_t pp_mutex, uint32_t time_out)
{
	int ret;

	ret = mutex_lock_interruptible(*pp_mutex);	/* Acquire the mutex, interruptible */
	if (!ret)
		return true;
	else
		return false;
}

/**
 * @brief Releases possesion of a mutex
 * 
 * @param[in] p_mutex: Mutex object to release.
 *
 * @return None.  
 */
void R_OS_MutexRelease(p_mutex_t p_mutex)
{
	mutex_unlock(p_mutex);
}

int32_t drp_map(uint32_t phys_addr, uint32_t size, uint32_t *copy_to_ram)
{
	int i;
	struct device *dev = &my_priv->pdev->dev;
	void *config_in_flash;

	/* Determine if already mapped */
	for (i=0; i < MAX_CONFIG; i++) {
		if (size != configs[i].size)
			continue;
		if (phys_addr == configs[i].phys)
			return R_DK2_SUCCESS;
		if (phys_addr == configs[i].copied_from) {
			*copy_to_ram = configs[i].phys;
			return R_DK2_SUCCESS;
		}
	}

	/* find an open spot */
	for (i=0; i < MAX_CONFIG; i++)
		if (configs[i].phys == 0)
			break;

	if (i == MAX_CONFIG) {
		printk("%s: Maximum config mappings (%d) reached.\n", __func__, MAX_CONFIG);
		return R_DK2_ERR_INTERNAL;
	}

	configs[i].size = size;

	/* Do we need to allocate RAM to copy the config into? */
	if (copy_to_ram) {
		configs[i].virt = dma_alloc_coherent(dev,
					size,
					&configs[i].phys,
					GFP_KERNEL);	

		if (!configs[i].virt) {
			printk("%s: Failed to allocate DMA coherent buffer for config data of size %d\n", __func__, size);

			return -EACCES;
		}

		/* Temporarily ioremap the config area so we can copy it into RAM */
		config_in_flash = ioremap(phys_addr, size);
		if (!config_in_flash) {
			dma_free_coherent(dev, size, configs[i].virt, configs[i].phys);
			configs[i].phys = configs[i].size = 0;
			configs[i].virt = NULL;
			printk("%s: ioremap of DRP config area failed\n", __func__);
			return -EACCES;
		}

		memcpy(configs[i].virt, config_in_flash, size);
		iounmap(config_in_flash);
		configs[i].copied_from = phys_addr;
		*copy_to_ram = configs[i].phys;

		return R_DK2_SUCCESS;
	}

	/* Map physical area into kernel virtual area */
	configs[i].virt = ioremap(phys_addr, size);
	if (!configs[i].virt) {
		printk("%s: ioremap of DRP config area failed\n", __func__);
		return -EACCES;
	}
	configs[i].phys = phys_addr;

	return R_DK2_SUCCESS;
}

int32_t drp_map_lookup(uint32_t phys_addr, void **virt_addr)
{
	int i;

	for (i=0; i < MAX_CONFIG; i++) {
		if (configs[i].phys == 0)
			break;

		if (	(phys_addr >= configs[i].phys) && 
			(phys_addr < (configs[i].phys + configs[i].size))) {

			*virt_addr = (phys_addr - configs[i].phys) + configs[i].virt;
			return R_DK2_SUCCESS;
		}
	}

	printk("%s: Config area mapping for 0x%08X not found.\n", __func__, phys_addr);

	return R_DK2_ERR_ARG;
}

int32_t drp_wait_for_interrupt(uint32_t id, uint32_t timeout)
{
	int ret;
	uint8_t wait_mask = id;
	ktime_t tim;

	/* Already finished? */
	if ((finish_status & wait_mask) == wait_mask)
		return R_DK2_SUCCESS;	/* return immediately */

	/* Wait for interrupt or multiple interrupts */
	tim = ktime_set(0, timeout*1000*1000);		/* seconds, nano seconds */
	ret = wait_event_hrtimeout(my_priv->wait,
				(finish_status & wait_mask) == wait_mask,
				tim); /* 250ms */

	if ((finish_status & wait_mask) != wait_mask)
		return R_DK2_ERR_BUSY;	/* timeout */

	return R_DK2_SUCCESS;
}

static long drp_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	int32_t drp_ret;
	int tile_no;
	struct drp_priv *priv = my_priv;
	uint8_t buffer[MAX_ARG_SIZE];
	struct R_DK2_Load_args *load_args;
	struct R_DK2_Unload_args *unload_args;
	struct R_DK2_Activate_args *activate_args;
	struct R_DK2_Inactivate_args *inactivate_args;
	struct R_DK2_Start_args *start_args;
	struct R_DK2_GetStatus_args *getstatus_args;
	struct R_DK2_GetInfo_args *getinfo_args;
	struct R_DK2_GetInfo_output *getinfo_output;
	struct R_DK2_Wait_args *wait_args;
	struct R_DK2_MapConfig_args *map_args;
	void *start_param;
	void *virt_addr;
	uint32_t phys_addr;

#define DRP_DCLKINACT *(volatile unsigned long *)(drpk_base_addr + 0xFFD820)

	switch (cmd) {

	case IOCTL_CMD_INIT:
		#ifdef DEBUG
		printk("%s: R_DK2_Initialize()\n", __func__);
		#endif

		if (priv->init_called) {
			/* Can only call R_DK2_Initialize once */
			drp_ret = R_DK2_SUCCESS;
		}
		else {
			drp_ret = R_DK2_Initialize();
			priv->init_called = 1;
		}
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;
		break;

	case IOCTL_CMD_UNINIT:
		#ifdef DEBUG
		printk("%s: R_DK2_Uninitialize()\n", __func__);
		#endif
		drp_ret = R_DK2_Uninitialize();
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;
		break;

	case IOCTL_CMD_LOAD:
		/* Get arguments */
		if (copy_from_user(buffer, (void *)arg, sizeof(struct R_DK2_Load_args)))
			return -EACCES;
		load_args = (struct R_DK2_Load_args *)buffer;
		#ifdef DEBUG
		printk("%s: R_DK2_Load(0x%08X, 0x%X, 0x%X, NULL, NULL)\n", __func__,
		load_args->pconfig, load_args->top_tiles, load_args->tile_pattern);
		#endif

		/* Look ioreammped address from physical address */
		drp_ret = drp_map_lookup(load_args->pconfig, &virt_addr);

		if (drp_ret == R_DK2_SUCCESS)
			drp_ret = R_DK2_Load(	virt_addr,
						load_args->top_tiles,
						load_args->tile_pattern,
						NULL,
						&cb_drp_finish,
						&drp_lib_id[0]);

		/* Return results */
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;
		if (copy_to_user((void *)arg + sizeof(drp_ret), (void *)drp_lib_id, sizeof(drp_lib_id)))
			return -EACCES;

		#ifdef DEBUG
		printk("%s: R_DK2_Load output (paid = %X %X %X %X %X %X)\n", __func__,
		drp_lib_id[0],drp_lib_id[1],drp_lib_id[2],drp_lib_id[3],drp_lib_id[4],drp_lib_id[5]);
		#endif

		break;

	case IOCTL_CMD_UNLOAD:
		/* Get arguments */
		if (copy_from_user(buffer, (void *)arg, sizeof(struct R_DK2_Unload_args)))
			return -EACCES;
		unload_args = (struct R_DK2_Unload_args *)buffer;
		#ifdef DEBUG
		printk("%s: R_DK2_Unload(%d)\n", __func__, unload_args->id);
		#endif
		drp_ret = R_DK2_Unload(unload_args->id, &drp_lib_id[0]);

		/* Return results */
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;
		if (copy_to_user((void *)arg + sizeof(drp_ret), (void *)drp_lib_id, sizeof(drp_lib_id)))
			return -EACCES;

		#ifdef DEBUG
		printk("%s: R_DK2_Unoad output (paid = %X %X %X %X %X %X)\n", __func__,
		drp_lib_id[0],drp_lib_id[1],drp_lib_id[2],drp_lib_id[3],drp_lib_id[4],drp_lib_id[5]);
		#endif

		break;

	case IOCTL_CMD_ACTIVATE:
		/* Get arguments */
		if (copy_from_user(buffer, (void *)arg, sizeof(struct R_DK2_Activate_args)))
			return -EACCES;
		activate_args = (struct R_DK2_Activate_args *)buffer;
		#ifdef DEBUG
		printk("%s: R_DK2_Activate(0x%08X, %d)\n", __func__, activate_args->id, activate_args->freq);
		#endif
		drp_ret = R_DK2_Activate(activate_args->id, activate_args->freq);

		/* Return results */
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;

		break;

	case IOCTL_CMD_INACTIVATE:
		/* Get arguments */
		if (copy_from_user(buffer, (void *)arg, sizeof(struct R_DK2_Inactivate_args)))
			return -EACCES;
		inactivate_args = (struct R_DK2_Inactivate_args *)buffer;
		#ifdef DEBUG
		printk("%s: R_DK2_Inactivate(0x%08X)\n", __func__, inactivate_args->id);
		#endif
		drp_ret = R_DK2_Inactivate(inactivate_args->id);

		/* Return results */
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;

		break;

	case IOCTL_CMD_START:
		/* Get arguments */
		if (copy_from_user(buffer, (void *)arg, MAX_ARG_SIZE))
			return -EACCES;

		start_args = (struct R_DK2_Start_args *)buffer;
		#ifdef DEBUG
		printk("%s: R_DK2_Start(0x%08X, %d)\n", __func__, start_args->id, start_args->size);
		#endif

		/* Copy from cached buffer into non-cached buffer */
		start_param = params_buffer;
		tile_no = 1;
		/* find top tile */
		while ((start_args->id & tile_no) == 0) {
			start_param += MAX_ARG_SIZE;
			tile_no = tile_no << 1;
		}
		memset(start_param, 0, MAX_ARG_SIZE);
		memcpy(start_param, &start_args->pparam, start_args->size);

		/* Initialize variable to be used to show finish status */
		for (tile_no = 0; tile_no < R_DK2_TILE_NUM; tile_no++)
		{
			if (drp_lib_id[tile_no] == start_args->id)
			{
				drp_lib_status[tile_no] = DRP_NOT_FINISH;
				finish_status &= ~(1 << tile_no);
				break;
			}
		}

		/* Start operation */
		drp_ret = R_DK2_Start(start_args->id, start_param, start_args->size);

		/* Return results */
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;

		break;

	case IOCTL_CMD_GETSTATUS:
		/* Get arguments */
		if (copy_from_user(buffer, (void *)arg, sizeof(struct R_DK2_GetStatus_args)))
			return -EACCES;
		getstatus_args = (struct R_DK2_GetStatus_args *)buffer;
		#ifdef DEBUG
		printk("%s: R_DK2_GetStatus(0x%08X)\n", __func__, getstatus_args->id);
		#endif
		drp_ret = R_DK2_GetStatus(getstatus_args->id);

		/* Return results */
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;

		break;

	case IOCTL_CMD_GETINFO:
		printk("%s: **** Future planned implementation ****\n", __func__);
		/* Get arguments */
		if (copy_from_user(buffer, (void *)arg, sizeof(struct R_DK2_GetInfo_args)))
			return -EACCES;
		getinfo_args = (struct R_DK2_GetInfo_args *)buffer;
		getinfo_output = (struct R_DK2_GetInfo_output *)buffer;
		#ifdef DEBUG
		printk("%s: R_DK2_GetInfo(0x%08X %d)\n", __func__, getinfo_args->pconfig, getinfo_args->crc_check);
		#endif

		{
			config_info_t *pinfo = (void *)buffer + 100;
			memset(pinfo, 0, sizeof(config_info_t));

			drp_ret = R_DK2_GetInfo((void *)getinfo_args->pconfig, pinfo, getinfo_args->crc_check);

			getinfo_output->return_value = drp_ret;
			if (drp_ret == R_DK2_SUCCESS) {
				getinfo_output->type = pinfo->type;
				getinfo_output->ver = pinfo->ver;
				getinfo_output->cid = pinfo->cid;
				memcpy(getinfo_output->pname, pinfo->pname, 32);
			}
		}

		/* Return results */
		if (copy_to_user((void *)arg, (void *)getinfo_output,	sizeof(struct R_DK2_GetInfo_output)))
			return -EACCES;

		break;

	case IOCTL_CMD_GETVERSION:
		#ifdef DEBUG
		printk("%s: R_DK2_GetVersion()\n", __func__);
		#endif
		drp_ret = R_DK2_GetVersion();
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;
		break;

	case IOCTL_CMD_WAIT:
		/* Get arguments */
		if (copy_from_user(buffer, (void *)arg, sizeof(struct R_DK2_Wait_args)))
			return -EACCES;
		wait_args = (struct R_DK2_Wait_args *)buffer;
		#ifdef DEBUG
		printk("%s: R_DK2_Wait(0x%08X %d)\n", __func__, wait_args->id, wait_args->timeout);
		#endif

		drp_ret = drp_wait_for_interrupt(wait_args->id, wait_args->timeout);

		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;
		break;

	case IOCTL_CMD_MAP:
		/* Get arguments */
		if (copy_from_user(buffer, (void *)arg, sizeof(struct R_DK2_MapConfig_args)))
			return -EACCES;
		map_args = (struct R_DK2_MapConfig_args *)buffer;
		#ifdef DEBUG
		printk("%s: R_DK2_Map(0x%08X %d %d)\n", __func__, map_args->phys_addr, map_args->size, map_args->copy_to_ram);
		#endif

		drp_ret = drp_map(map_args->phys_addr,
				  map_args->size,
				  map_args->copy_to_ram ? &phys_addr : 0);

		if ( !drp_ret && map_args->copy_to_ram ) {

			/* Return physical address of allocated RAM */
			if (copy_to_user((void *)(arg + sizeof(int32_t)),	// skip over return value
					 (void *)&phys_addr, sizeof(uint32_t))) {
				return -EACCES;
			}
		}

		// Copy in return value
		if (copy_to_user((void *)arg, (void *)&drp_ret,	sizeof(drp_ret)))
			return -EACCES;

		break;

	default:
		dev_err(&priv->pdev->dev, "Unknown ioctl (%X)\n", cmd);
	}

	return 0;
}

static const struct file_operations fops = {
	.owner = THIS_MODULE,
	.unlocked_ioctl = drp_ioctl,
};

static struct miscdevice misc = {
	.minor		= MISC_DYNAMIC_MINOR,
	.name		= "drp",
	.fops		= &fops,
};

static int drp_probe(struct platform_device *pdev)
{
	struct drp_priv *priv;
	struct device_node *np = pdev->dev.of_node;
	struct resource *res;
#if 0 /* ignore clock for now, should already be on */
	struct clk *clk;
#endif
	int ret;
	unsigned int irq;
	uint32_t config_area_phys;	/* Physical address of DRP config data */
	uint32_t config_area_size;	/* Size of DRP config data */

	priv = devm_kzalloc(&pdev->dev, sizeof(*priv), GFP_KERNEL);
	if (!priv) {
		dev_err(&pdev->dev, "cannot allocate private data\n");
		ret = -ENOMEM;
		goto error;
	}
	platform_set_drvdata(pdev, priv);
	priv->pdev = pdev;
	priv->dev_name = dev_name(&pdev->dev);
	my_priv = priv;

	res = platform_get_resource(pdev, IORESOURCE_MEM, 0);
	if (!res) {
		dev_err(&pdev->dev, "cannot get resources (reg)\n");
		goto error;
	}
	priv->base = devm_ioremap_nocache(&pdev->dev, res->start, resource_size(res));
	if (!priv->base) {
		dev_err(&pdev->dev, "cannot ioremap\n");
		goto error;
	}
	drpk_base_addr = priv->base;

#if 0 /* ignore clock for now, should already be on */
	clk = devm_clk_get(&pdev->dev, NULL);
	if (IS_ERR(clk)) {
		dev_err(&pdev->dev, "failed to get clock\n");
		ret = -1;
		goto error;
	}

	ret = clk_prepare_enable(clk);
	if (ret < 0) {
		dev_info(&pdev->dev, "failed to enable clk\n");
		goto error;
	}
#endif

	irq = platform_get_irq(pdev, 0);
	ret = devm_request_irq(&pdev->dev, irq, drp_interrupt, 0, "drp", priv);
	if (ret) {
		dev_err(&pdev->dev, "Failed to claim IRQ!\n");
		goto error;
	}

	/* Map physical location of config data to into kernel space */
	/* Read values from Device Tree */
	if( !of_property_read_u32(np, "config-area-phys", &config_area_phys) ) {
		of_property_read_u32(np, "config-area-size", &config_area_size);
		if (drp_map(config_area_phys, config_area_size, 0)) {
			printk("%s: ioremap of DRP config area failed\n", __func__);
			goto error;
			ret = -EACCES;
		}
	}

	/* Allocate a Coherent Memory buffer which means cache will always be
	 * flushed back to the device memory immediately */
	work_buffer = dma_alloc_coherent(&pdev->dev, WORK_AREA_SIZE, &work_phys, GFP_KERNEL);
	if (!work_buffer) {
		printk("%s: Failed to allocate DMA coherent buffer for DRP work area\n", __func__);
		goto error;
		ret = -EACCES;
	}
	work_load_ptr = work_buffer;
	work_start_ptr = (void *)work_load_ptr + R_DK2_TILE_NUM * 16;

	params_buffer = dma_alloc_coherent(&pdev->dev, PARAMS_SIZE, &params_phys, GFP_KERNEL);
	if (!work_buffer) {
		printk("%s: Failed to allocate DMA coherent buffer for parameter area\n", __func__);
		goto error;
		ret = -EACCES;
	}

	init_waitqueue_head(&priv->wait);

	/* Finally, register our driver interface */
	misc_register(&misc);

	printk("DRP driver loaded\n");

	return 0;

error:
	dev_err(&pdev->dev, "DRP driver initialization Failed.\n");
	return ret;
}

static int drp_remove(struct platform_device *pdev)
{
	int i;
	
	for (i=0; i < MAX_CONFIG; i++)
		if (configs[i].virt) {
			if (configs[i].copied_from)
				dma_free_coherent(&pdev->dev, configs[i].size, configs[i].virt, configs[i].phys);
			else
				iounmap(configs[i].virt);
		}

	dma_free_coherent(&pdev->dev, WORK_AREA_SIZE, work_buffer, work_phys);
	dma_free_coherent(&pdev->dev, PARAMS_SIZE, params_buffer, params_phys);
	printk("DRP driver unloaded\n");

	return 0;
}

static const struct of_device_id of_drp_match[] = {
	{ .compatible = "renesas,r7s9210-drp",},
	{ /* sentinel */ }
};

MODULE_DEVICE_TABLE(of, of_drp_match);

static struct platform_driver drp_driver = {
	.driver = {
		.name	= "drp",
		.owner	= THIS_MODULE,
		.of_match_table	= of_drp_match,
		},
	.probe	= drp_probe,
	.remove	= drp_remove,
};

static int __init drp_init(void)
{
	return platform_driver_register(&drp_driver);
}

static void __exit drp_exit(void)
{
	misc_deregister(&misc);
	return platform_driver_unregister(&drp_driver);
}

module_init(drp_init);
module_exit(drp_exit);

MODULE_DESCRIPTION("Renesas DRP driver");
MODULE_AUTHOR("Renesas");
MODULE_LICENSE("GPL v2");
