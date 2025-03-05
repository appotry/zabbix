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

#include "zbxmocktest.h"
#include "zbxmockutil.h"
#include "zbxmockassert.h"

#include "zbxexpr.h"

void	zbx_mock_test_entry(void **state)
{
	const char		*str;
	char			*error = NULL;
	int			value, expected_ret, ret;
	zbx_custom_interval_t	*custom_intervals;

	ZBX_UNUSED(state);

	str = zbx_mock_get_parameter_string("in.str");
	expected_ret = zbx_mock_str_to_return_code(zbx_mock_get_parameter_string("out.return"));

	if (FAIL == (ret = zbx_validate_interval(str, &error)))
		zbx_free(error);

	zbx_mock_assert_int_eq("return value", expected_ret, ret);

	expected_ret = NULL == strstr(str, "{$") ? expected_ret : FAIL;

	if (SUCCEED == (ret = zbx_interval_preproc(str, &value, &custom_intervals, NULL)))
		zbx_custom_interval_free(custom_intervals);

	zbx_mock_assert_int_eq("return value", expected_ret, ret);


}
