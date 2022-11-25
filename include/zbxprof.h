/*
** Zabbix
** Copyright (C) 2001-2022 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#ifndef ZABBIX_PROF_H
#define ZABBIX_PROF_H

typedef enum
{
	ZBX_PROF_PROCESSING,
	ZBX_PROF_RWLOCK,
	ZBX_PROF_MUTEX
}
zbx_prof_scope_t;

void	zbx_prof_enable(void);
void	zbx_prof_disable(void);
void	zbx_prof_start(const char *func_name, zbx_prof_scope_t scope);
void	zbx_prof_end_wait(void);
void	zbx_prof_end(void);
void	zbx_prof_update(double time_now);

#endif
