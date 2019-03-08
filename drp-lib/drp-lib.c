#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include "r_dk2_if.h"
#include "../driver/drp/drp_linux.h"


static int fd_drp;
static int fd_mem;


int32_t R_DK2_Initialize(void)
{
	int32_t ret;
	int32_t drp_ret;

	fd_drp = open("/dev/drp", O_RDWR);
	fd_mem = open("/dev/mem", O_RDWR);

	if (fd_drp < 0 || fd_mem < 0)
	{
		printf("Failed to open /dev/drp (driver not loaded?)\n");
		ret = R_DK2_ERR_OS;
		goto error;
	}

	ret = ioctl(fd_drp, IOCTL_CMD_INIT, &drp_ret);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n",__func__);
		ret = R_DK2_ERR_OS;
		goto error;
	}

	return (int32_t)drp_ret;

error:
	if ( fd_mem >= 0)
		close(fd_mem);
	if ( fd_drp >= 0)
		close(fd_drp);
	fd_drp = fd_mem = 0;

	return ret;
}


int32_t R_DK2_Uninitialize(void)
{
	int32_t ret;
	int32_t drp_ret;

	if (!fd_drp)
		return -1;

	ret = ioctl(fd_drp, IOCTL_CMD_UNINIT, &drp_ret);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n",__func__);
		return R_DK2_ERR_OS;
	}

	return (int32_t)drp_ret;
}

int32_t R_DK2_Load(const void *const pconfig, const uint8_t top_tiles, const uint32_t tile_pattern, const load_cb_t pload, const int_cb_t pint, uint8_t *const paid)
{
	int32_t ret;
	uint8_t buffer[sizeof(struct R_DK2_Load_args)];
	struct R_DK2_Load_args *load_args = (struct R_DK2_Load_args *)buffer;

	if (!fd_drp)
		return -1;

	if (pload)
	{
		printf("%s: WARNING: pload argument not supported\n", __func__);
	}
	if (pint)
	{
		printf("%s: WARNING: pint argument not supported\n", __func__);
	}

	load_args->pconfig = (int32_t) pconfig;
	load_args->top_tiles = (int32_t) top_tiles;
	load_args->tile_pattern = (int32_t) tile_pattern;
	load_args->pload = (int32_t) pload;
	load_args->pint = (int32_t) pint;

	ret = ioctl(fd_drp, IOCTL_CMD_LOAD, buffer);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n", __func__);
		return R_DK2_ERR_OS;
	}

	/* First 4 bytes are the return code */
	ret = *(int32_t *)buffer;

	/* paid is passed back after return code */
	memcpy(paid, buffer + sizeof(uint32_t), R_DK2_TILE_NUM);

	return ret;
}


int32_t R_DK2_Unload(const uint8_t id, uint8_t *const paid)
{
	int32_t ret;
	uint8_t buffer[sizeof(uint32_t) + 6]; /* need enough for paid */
	struct R_DK2_Unload_args *unload_args = (struct R_DK2_Unload_args *)buffer;

	if (!fd_drp)
		return -1;

	unload_args->id = id;

	ret = ioctl(fd_drp, IOCTL_CMD_UNLOAD, buffer);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n", __func__);
		return R_DK2_ERR_OS;
	}

	/* First 4 bytes are the return code */
	ret = *(int32_t *)buffer;

	/* paid is passed back after return code */
	memcpy(paid, buffer + sizeof(uint32_t), R_DK2_TILE_NUM);

	return ret;
}


int32_t R_DK2_Activate(const uint8_t id, const uint32_t freq)
{
	int32_t ret;
	uint8_t buffer[sizeof(struct R_DK2_Activate_args)];
	struct R_DK2_Activate_args *activate_args = (struct R_DK2_Activate_args *)buffer;

	activate_args->id = id;
	activate_args->freq = freq;

	ret = ioctl(fd_drp, IOCTL_CMD_ACTIVATE, buffer);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n", __func__);
		return R_DK2_ERR_OS;
	}

	return *(int32_t *)buffer;
}


int32_t R_DK2_Inactivate(const uint8_t id)
{
	int32_t ret;
	uint8_t buffer[sizeof(struct R_DK2_Inactivate_args)];
	struct R_DK2_Inactivate_args *inactivate_args = (struct R_DK2_Inactivate_args *)buffer;

	inactivate_args->id = id;

	ret = ioctl(fd_drp, IOCTL_CMD_INACTIVATE, buffer);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n", __func__);
		return R_DK2_ERR_OS;
	}

	return *(int32_t *)buffer;
}

int32_t R_DK2_Start(const uint8_t id, const void *const pparam, const uint32_t size)
{
	int32_t ret;
	uint8_t buffer[MAX_ARG_SIZE];
	struct R_DK2_Start_args *start_args = (struct R_DK2_Start_args *)buffer;

	if (!fd_drp)
		return -1;

	if ((sizeof(struct R_DK2_Start_args) + size) > MAX_ARG_SIZE)
	{
		printf("%s: Not enough buffer space to pass arguments.\n", __func__);
		return R_DK2_ERR_OS;
	}

	start_args->id = id;
	start_args->size = size;
	memcpy(&start_args->pparam, pparam, size);

	ret = ioctl(fd_drp, IOCTL_CMD_START, buffer);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n", __func__);
		return R_DK2_ERR_OS;
	}

	return *(int32_t *)buffer;
}


int32_t R_DK2_GetStatus(const uint8_t id)
{
	int32_t ret;
	uint8_t buffer[sizeof(struct R_DK2_GetStatus_args)];
	struct R_DK2_GetStatus_args *getstatus_args = (struct R_DK2_GetStatus_args *)buffer;

	getstatus_args->id = id;

	ret = ioctl(fd_drp, IOCTL_CMD_GETSTATUS, buffer);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n", __func__);
		return R_DK2_ERR_OS;
	}

	return *(int32_t *)buffer;
}


int32_t R_DK2_GetInfo(const void *const pconfig, config_info_t *const pinfo, const bool crc_check)
{
	int32_t ret;
	uint8_t buffer[MAX_ARG_SIZE];
	struct R_DK2_GetInfo_args *getinfo_args = (struct R_DK2_GetInfo_args *)buffer;
	static struct R_DK2_GetInfo_output getinfo_output;

	getinfo_args->pconfig = (uint32_t)pconfig;
	getinfo_args->crc_check = crc_check;

	ret = ioctl(fd_drp, IOCTL_CMD_GETINFO, buffer);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n", __func__);
		return R_DK2_ERR_OS;
	}

	if (*(uint32_t *)buffer == R_DK2_SUCCESS)
	{
		memcpy(&getinfo_output, buffer, sizeof(struct R_DK2_GetInfo_output));

		pinfo->type = getinfo_output.type;
		pinfo->ver = getinfo_output.ver;
		pinfo->cid = getinfo_output.cid;
		pinfo->pname = getinfo_output.pname;
	}

	return *(int32_t *)buffer;
}

uint32_t R_DK2_GetVersion(void)
{
	int32_t ret;
	int32_t drp_ret;

	if (!fd_drp)
		return -1;

	ret = ioctl(fd_drp, IOCTL_CMD_GETVERSION, &drp_ret);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n",__func__);
		return R_DK2_ERR_OS;
	}

	return (uint32_t)drp_ret;
}


int32_t R_DK2_Wait(const uint8_t id, uint32_t timeout, uint8_t *const paid)
{
	int32_t ret;
	uint8_t buffer[MAX_ARG_SIZE];
	struct R_DK2_Wait_args *wait_args = (struct R_DK2_Wait_args *)buffer;

	if (!fd_drp)
		return -1;

	if (paid)
	{
		printf("%s: WARNING: paid argument not supported\n", __func__);
	}

	wait_args->id = id;
	wait_args->timeout = timeout;

	ret = ioctl(fd_drp, IOCTL_CMD_WAIT, buffer);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n",__func__);
		return R_DK2_ERR_OS;
	}

	return *(int32_t *)buffer;
}

int32_t R_DK2_MapConfig(uint32_t phys_addr, uint32_t size, uint32_t *copy_to_ram)
{
	int32_t ret;
	uint8_t buffer[sizeof(struct R_DK2_MapConfig_args)];
	struct R_DK2_MapConfig_args *map_args = (struct R_DK2_MapConfig_args *)buffer;

	if (!fd_drp)
		return -1;

	map_args->phys_addr = phys_addr;
	map_args->size = size;
	map_args->copy_to_ram = copy_to_ram ? 1 : 0;

	ret = ioctl(fd_drp, IOCTL_CMD_MAP, buffer);
	if (ret != 0)
	{
		printf("%s: ioctl failed\n",__func__);
		return R_DK2_ERR_OS;
	}

	ret = *(int32_t *)buffer;

	/* Return the physical address of the RAM area allocated by
	 * the driver (if was requested) */
	if (copy_to_ram)
	{
		*copy_to_ram = *(uint32_t *)(buffer + sizeof(int32_t));
	}

	return ret;
}
