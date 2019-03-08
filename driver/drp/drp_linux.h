#include <linux/ioctl.h>


// IOCTL
enum {
	IOCTL_CMD_INIT		= _IOR(0xC1, 0x21, uint32_t),
	IOCTL_CMD_UNINIT	= _IOR(0xC1, 0x22, uint32_t),
	IOCTL_CMD_LOAD 		= _IOWR(0xC1, 0x23, uint32_t),
	IOCTL_CMD_UNLOAD 	= _IOWR(0xC1, 0x24, uint32_t),
	IOCTL_CMD_ACTIVATE	= _IOWR(0xC1, 0x25, uint32_t),
	IOCTL_CMD_INACTIVATE	= _IOWR(0xC1, 0x26, uint32_t),
	IOCTL_CMD_START 	= _IOWR(0xC1, 0x27, uint32_t),
	IOCTL_CMD_GETSTATUS 	= _IOWR(0xC1, 0x28, uint32_t),
	IOCTL_CMD_GETINFO 	= _IOWR(0xC1, 0x29, uint32_t),
	IOCTL_CMD_GETVERSION	= _IOR(0xC1, 0x2A, uint32_t),

	IOCTL_CMD_WAIT		= _IOR(0xC1, 0x2B, uint32_t),
	IOCTL_CMD_MAP		= _IOR(0xC1, 0x2C, uint32_t),
};

/* int32_t R_DK2_Load(const void *const pconfig, const uint8_t top_tiles, const uint32_t tile_pattern, const load_cb_t pload, const int_cb_t pint, uint8_t *const paid); */
struct R_DK2_Load_args
{
	uint32_t	pconfig;	// const void *const pconfig
	uint32_t	top_tiles;	// const uint8_t top_tiles
	uint32_t	tile_pattern;	// const uint32_t tile_pattern
	uint32_t	pload;		// const load_cb_t pload
	uint32_t	pint;		// const int_cb_t pint
	//uint32_t	paid;		// uint8_t *const paid
};

/* int32_t R_DK2_Unload(const uint8_t id, uint8_t *const paid); */
struct R_DK2_Unload_args
{
	uint32_t	id;		// const uint8_t id
//	uint32_t	paid;		// uint8_t *const paid
};

/* int32_t R_DK2_Activate(const uint8_t id, const uint32_t freq); */
struct R_DK2_Activate_args
{
	uint32_t	id;		// const uint8_t id
	uint32_t	freq;		// const uint32_t freq
};


/* int32_t R_DK2_Inactivate(const uint8_t id); */
struct R_DK2_Inactivate_args
{
	uint32_t	id;		// const uint8_t id
};


/* int32_t R_DK2_Start(const uint8_t id, const void *const pparam, const uint32_t size); */
struct R_DK2_Start_args
{
	uint32_t	id;		// const uint8_t id
	uint32_t	size;		// const uint32_t size
	uint32_t	pparam;		// const void *const pparam
	/* [pparam data here (as input)] */
};


/* int32_t R_DK2_GetStatus(const uint8_t id);; */
struct R_DK2_GetStatus_args
{
	uint32_t	id;		// const uint8_t id
};

/* int32_t R_DK2_GetInfo(const void *const pconfig, config_info_t *const pinfo, const bool crc_check); */
struct R_DK2_GetInfo_args
{
	uint32_t	pconfig;	// const void *const pconfig
	uint32_t	crc_check;	// const bool crc_check
//	uint32_t	pinfo;		// config_info_t *const pinfo
};
struct R_DK2_GetInfo_output
{
	uint32_t	return_value;
	uint32_t	type;
	uint32_t	ver;
	uint32_t	cid;
	char		pname[32];
};

/* int32_t R_DK2_Wait(const uint8_t id, uint32_t timeout, uint8_t *const paid); */
struct R_DK2_Wait_args
{
	uint32_t	id;		// const uint8_t id
	uint32_t	timeout;	// uint32_t timeout
	uint32_t	paid;		// uint8_t *const paid
};

/* int32_t R_DK2_MapConfig(uint32_t phys_addr, uint32_t size, uint32_t *copy_to_ram); */
struct R_DK2_MapConfig_args
{
	uint32_t	phys_addr;
	uint32_t	size;
	uint32_t	copy_to_ram;
};


#define MAX_ARG_SIZE 256

typedef bool bool_t;

/* From generate/os_abstraction/inc/r_os_abstraction_typedef.h */
typedef void* p_mutex_t;		/* mutex handle object */
typedef p_mutex_t* pp_mutex_t;		/* pointer to mutex handle object*/

/* From generate/os_abstraction/inc/r_os_abstraction_api.h */
void* R_OS_MutexCreate (void);
void R_OS_MutexRelease(p_mutex_t p_mutex);
bool_t R_OS_MutexWait (pp_mutex_t pp_mutex, uint32_t time_out);


// fake out enum types
typedef int e_r_drv_intc_intid_t;
typedef int e_r_drv_intc_priority_t;
typedef int e_r_drv_intc_err_t;
typedef int e_mmu_err_t;

/* From generate/drivers/r_intc/inc/r_intc_lld_rza2m.h */
#define INTC_ID_DRP_NMLINT              ((e_r_drv_intc_intid_t)447)
e_r_drv_intc_err_t R_INTC_SetPriority( e_r_drv_intc_intid_t int_id, e_r_drv_intc_priority_t priority );
e_r_drv_intc_err_t R_INTC_RegistIntFunc(e_r_drv_intc_intid_t int_id, void (* func)(uint32_t int_sense));
e_r_drv_intc_err_t R_INTC_Enable( e_r_drv_intc_intid_t int_id );

/* From generate/drivers/r_mmu/inc/r_mmu_lld_rza2m.h */
e_mmu_err_t R_MMU_VAtoPA( uint32_t vaddress, uint32_t * paddress );


//uint32_t calc_crc(const uint8_t *const buf, const uint32_t len);

