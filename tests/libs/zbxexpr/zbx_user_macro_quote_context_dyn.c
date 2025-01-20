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
	const char	*context = zbx_mock_get_parameter_string("in.context");
	int		force_quote = zbx_mock_get_parameter_int("in.force_quote");
	const char	*exp_result = zbx_mock_get_parameter_string("out.exp_result");
	char		*error = NULL;

	ZBX_UNUSED(state);

	char	*result = zbx_user_macro_quote_context_dyn(context, force_quote, &error);

	if (NULL != result)
		zbx_mock_assert_str_eq("return value str", exp_result, result);

	zbx_free(result);

	if (SUCCEED != force_quote)
		zbx_free(error);
}
