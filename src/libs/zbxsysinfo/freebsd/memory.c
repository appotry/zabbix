/*
** Copyright (C) 2001-2025 Zabbix SIA
**
** This program is free software: you can redistribute it and/or modify it under the terms of
** the GNU Affero General Public License as published by the Free Software Foundation, version 3.
**
** This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
** without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
** See the GNU Affero General Public License for more details.
**
** You should have received a copy of the GNU Affero General Public License along with this program.
** If not, see <https://www.gnu.org/licenses/>.
**/

#include "zbxsysinfo.h"
#include "../sysinfo.h"

static u_int	pagesize = 0;

#define ZBX_SYSCTLBYNAME(name, value)									\
													\
	len = sizeof(value);										\
	if (0 != sysctlbyname(name, &value, &len, NULL, 0))						\
	{												\
		SET_MSG_RESULT(result, zbx_dsprintf(NULL, "Cannot obtain \"%s\" system parameter: %s",	\
				name, zbx_strerror(errno)));						\
		return SYSINFO_RET_FAIL;								\
	}

static int	vm_memory_total(AGENT_RESULT *result)
{
	unsigned long	totalbytes;
	size_t		len;

	ZBX_SYSCTLBYNAME("hw.physmem", totalbytes);

	SET_UI64_RESULT(result, (zbx_uint64_t)totalbytes);

	return SYSINFO_RET_OK;
}

static int	vm_memory_active(AGENT_RESULT *result)
{
	u_int	activepages;
	size_t	len;

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_active_count", activepages);

	SET_UI64_RESULT(result, (zbx_uint64_t)activepages * pagesize);

	return SYSINFO_RET_OK;
}

static int	vm_memory_inactive(AGENT_RESULT *result)
{
	u_int	inactivepages;
	size_t	len;

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_inactive_count", inactivepages);

	SET_UI64_RESULT(result, (zbx_uint64_t)inactivepages * pagesize);

	return SYSINFO_RET_OK;
}

static int	vm_memory_wired(AGENT_RESULT *result)
{
	u_int	wiredpages;
	size_t	len;

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_wire_count", wiredpages);

	SET_UI64_RESULT(result, (zbx_uint64_t)wiredpages * pagesize);

	return SYSINFO_RET_OK;
}

static int	vm_memory_cached(AGENT_RESULT *result)
{
	u_int	cachedpages;
	size_t	len;

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_cache_count", cachedpages);

	SET_UI64_RESULT(result, (zbx_uint64_t)cachedpages * pagesize);

	return SYSINFO_RET_OK;
}

static int	vm_memory_free(AGENT_RESULT *result)
{
	u_int	freepages;
	size_t	len;

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_free_count", freepages);

	SET_UI64_RESULT(result, (zbx_uint64_t)freepages * pagesize);

	return SYSINFO_RET_OK;
}

static int	vm_memory_used(AGENT_RESULT *result)
{
	u_int	activepages, wiredpages, cachedpages;
	size_t	len;

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_active_count", activepages);
	ZBX_SYSCTLBYNAME("vm.stats.vm.v_wire_count", wiredpages);
	ZBX_SYSCTLBYNAME("vm.stats.vm.v_cache_count", cachedpages);

	SET_UI64_RESULT(result, (zbx_uint64_t)(activepages + wiredpages + cachedpages) * pagesize);

	return SYSINFO_RET_OK;
}

static int	vm_memory_pused(AGENT_RESULT *result)
{
	u_int	activepages, wiredpages, cachedpages, totalpages;
	size_t	len;

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_active_count", activepages);
	ZBX_SYSCTLBYNAME("vm.stats.vm.v_wire_count", wiredpages);
	ZBX_SYSCTLBYNAME("vm.stats.vm.v_cache_count", cachedpages);

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_page_count", totalpages);

	if (0 == totalpages)
	{
		SET_MSG_RESULT(result, zbx_strdup(NULL, "Cannot calculate percentage because total is zero."));
		return SYSINFO_RET_FAIL;
	}

	SET_DBL_RESULT(result, (activepages + wiredpages + cachedpages) / (double)totalpages * 100);

	return SYSINFO_RET_OK;
}

static int	vm_memory_available(AGENT_RESULT *result)
{
	u_int	inactivepages, cachedpages, freepages;
	size_t	len;

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_inactive_count", inactivepages);
	ZBX_SYSCTLBYNAME("vm.stats.vm.v_cache_count", cachedpages);
	ZBX_SYSCTLBYNAME("vm.stats.vm.v_free_count", freepages);

	SET_UI64_RESULT(result, (zbx_uint64_t)(inactivepages + cachedpages + freepages) * pagesize);

	return SYSINFO_RET_OK;
}

static int	vm_memory_pavailable(AGENT_RESULT *result)
{
	u_int	inactivepages, cachedpages, freepages, totalpages;
	size_t	len;

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_inactive_count", inactivepages);
	ZBX_SYSCTLBYNAME("vm.stats.vm.v_cache_count", cachedpages);
	ZBX_SYSCTLBYNAME("vm.stats.vm.v_free_count", freepages);

	ZBX_SYSCTLBYNAME("vm.stats.vm.v_page_count", totalpages);

	if (0 == totalpages)
	{
		SET_MSG_RESULT(result, zbx_strdup(NULL, "Cannot calculate percentage because total is zero."));
		return SYSINFO_RET_FAIL;
	}

	SET_DBL_RESULT(result, (inactivepages + cachedpages + freepages) / (double)totalpages * 100);

	return SYSINFO_RET_OK;
}

static int	vm_memory_buffers(AGENT_RESULT *result)
{
	u_int	bufspace;
	size_t	len;

	ZBX_SYSCTLBYNAME("vfs.bufspace", bufspace);

	SET_UI64_RESULT(result, bufspace);

	return SYSINFO_RET_OK;
}

static int	vm_memory_shared(AGENT_RESULT *result)
{
	struct vmtotal	vm;
	size_t		len = sizeof(vm);
	int		mib[] = {CTL_VM, VM_METER};

	if (0 != sysctl(mib, 2, &vm, &len, NULL, 0))
	{
		SET_MSG_RESULT(result, zbx_dsprintf(NULL, "Cannot obtain system information: %s", zbx_strerror(errno)));
		return SYSINFO_RET_FAIL;
	}

	SET_UI64_RESULT(result, (zbx_uint64_t)(vm.t_vmshr + vm.t_rmshr) * pagesize);

	return SYSINFO_RET_OK;
}

int     vm_memory_size(AGENT_REQUEST *request, AGENT_RESULT *result)
{
	char	*mode;
	int	ret;

	if (1 < request->nparam)
	{
		SET_MSG_RESULT(result, zbx_strdup(NULL, "Too many parameters."));
		return SYSINFO_RET_FAIL;
	}

	if (0 == pagesize)
	{
		size_t	len;

		ZBX_SYSCTLBYNAME("vm.stats.vm.v_page_size", pagesize);
	}

	mode = get_rparam(request, 0);

	if (NULL == mode || '\0' == *mode || 0 == strcmp(mode, "total"))
		ret = vm_memory_total(result);
	else if (0 == strcmp(mode, "active"))
		ret = vm_memory_active(result);
	else if (0 == strcmp(mode, "inactive"))
		ret = vm_memory_inactive(result);
	else if (0 == strcmp(mode, "wired"))
		ret = vm_memory_wired(result);
	else if (0 == strcmp(mode, "cached"))
		ret = vm_memory_cached(result);
	else if (0 == strcmp(mode, "free"))
		ret = vm_memory_free(result);
	else if (0 == strcmp(mode, "used"))
		ret = vm_memory_used(result);
	else if (0 == strcmp(mode, "pused"))
		ret = vm_memory_pused(result);
	else if (0 == strcmp(mode, "available"))
		ret = vm_memory_available(result);
	else if (0 == strcmp(mode, "pavailable"))
		ret = vm_memory_pavailable(result);
	else if (0 == strcmp(mode, "buffers"))
		ret = vm_memory_buffers(result);
	else if (0 == strcmp(mode, "shared"))
		ret = vm_memory_shared(result);
	else
	{
		SET_MSG_RESULT(result, zbx_strdup(NULL, "Invalid first parameter."));
		ret = SYSINFO_RET_FAIL;
	}

	return ret;
}
