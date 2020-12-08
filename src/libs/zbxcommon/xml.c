/*
** Zabbix
** Copyright (C) 2001-2020 Zabbix SIA
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

#include "common.h"
#include "zbxalgo.h"
#include "zbxjson.h"
/* LIBXML2 is used */
#ifdef HAVE_LIBXML2
#	include <libxml/parser.h>
#	include <libxml/tree.h>
#	include <libxml/xpath.h>
#endif

#define XML_TEXT_NAME	"text"
#define XML_CDATA_NAME	"cdata"
#define XML_TEXT_TAG	"#text"
#define XML_JSON_TRUE	1
#define XML_JSON_FALSE	0

typedef struct
{
	char *		 	name;
	char *		 	value;
	zbx_vector_str_t 	attributes;
	zbx_vector_ptr_t	chnodes;
	int			array;
}
zbx_xml_node_t;

static char	data_static[ZBX_MAX_B64_LEN];

/******************************************************************************
 *                                                                            *
 * Purpose: get DATA from <tag>DATA</tag>                                     *
 *                                                                            *
 ******************************************************************************/
int	xml_get_data_dyn(const char *xml, const char *tag, char **data)
{
	size_t	len, sz;
	const char	*start, *end;

	sz = sizeof(data_static);

	len = zbx_snprintf(data_static, sz, "<%s>", tag);
	if (NULL == (start = strstr(xml, data_static)))
		return FAIL;

	zbx_snprintf(data_static, sz, "</%s>", tag);
	if (NULL == (end = strstr(xml, data_static)))
		return FAIL;

	if (end < start)
		return FAIL;

	start += len;
	len = end - start;

	if (len > sz - 1)
		*data = (char *)zbx_malloc(*data, len + 1);
	else
		*data = data_static;

	zbx_strlcpy(*data, start, len + 1);

	return SUCCEED;
}

void	xml_free_data_dyn(char **data)
{
	if (*data == data_static)
		*data = NULL;
	else
		zbx_free(*data);
}

/******************************************************************************
 *                                                                            *
 * Function: xml_escape_dyn                                                   *
 *                                                                            *
 * Purpose: replace <> symbols in string with &lt;&gt; so the resulting       *
 *          string can be written into xml field                              *
 *                                                                            *
 * Parameters: data - [IN] the input string                                   *
 *                                                                            *
 * Return value: an allocated string containing escaped input string          *
 *                                                                            *
 * Comments: The caller must free the returned string after it has been used. *
 *                                                                            *
 ******************************************************************************/
char	*xml_escape_dyn(const char *data)
{
	char		*out, *ptr_out;
	const char	*ptr_in;
	int		size = 0;

	if (NULL == data)
		return zbx_strdup(NULL, "");

	for (ptr_in = data; '\0' != *ptr_in; ptr_in++)
	{
		switch (*ptr_in)
		{
			case '<':
			case '>':
				size += 4;
				break;
			case '&':
				size += 5;
				break;
			case '"':
			case '\'':
				size += 6;
				break;
			default:
				size++;
		}
	}
	size++;

	out = (char *)zbx_malloc(NULL, size);

	for (ptr_out = out, ptr_in = data; '\0' != *ptr_in; ptr_in++)
	{
		switch (*ptr_in)
		{
			case '<':
				*ptr_out++ = '&';
				*ptr_out++ = 'l';
				*ptr_out++ = 't';
				*ptr_out++ = ';';
				break;
			case '>':
				*ptr_out++ = '&';
				*ptr_out++ = 'g';
				*ptr_out++ = 't';
				*ptr_out++ = ';';
				break;
			case '&':
				*ptr_out++ = '&';
				*ptr_out++ = 'a';
				*ptr_out++ = 'm';
				*ptr_out++ = 'p';
				*ptr_out++ = ';';
				break;
			case '"':
				*ptr_out++ = '&';
				*ptr_out++ = 'q';
				*ptr_out++ = 'u';
				*ptr_out++ = 'o';
				*ptr_out++ = 't';
				*ptr_out++ = ';';
				break;
			case '\'':
				*ptr_out++ = '&';
				*ptr_out++ = 'a';
				*ptr_out++ = 'p';
				*ptr_out++ = 'o';
				*ptr_out++ = 's';
				*ptr_out++ = ';';
				break;
			default:
				*ptr_out++ = *ptr_in;
		}

	}
	*ptr_out = '\0';

	return out;
}

/**********************************************************************************
 *                                                                                *
 * Function: xml_escape_xpath_stringsize                                          *
 *                                                                                *
 * Purpose: calculate a string size after symbols escaping                        *
 *                                                                                *
 * Parameters: string - [IN] the string to check                                  *
 *                                                                                *
 * Return value: new size of the string                                           *
 *                                                                                *
 **********************************************************************************/
static size_t	xml_escape_xpath_stringsize(const char *string)
{
	size_t		len = 0;
	const char	*sptr;

	if (NULL == string )
		return 0;

	for (sptr = string; '\0' != *sptr; sptr++)
		len += (('"' == *sptr) ? 2 : 1);

	return len;
}

/**********************************************************************************
 *                                                                                *
 * Function: xml_escape_xpath_insstring                                           *
 *                                                                                *
 * Purpose: replace " symbol in string with ""                                    *
 *                                                                                *
 * Parameters: string - [IN/OUT] the string to update                             *
 *                                                                                *
 **********************************************************************************/
static void xml_escape_xpath_string(char *p, const char *string)
{
	const char	*sptr = string;

	while ('\0' != *sptr)
	{
		if ('"' == *sptr)
			*p++ = '"';

		*p++ = *sptr++;
	}
}

/**********************************************************************************
 *                                                                                *
 * Function: xml_escape_xpath                                                     *
 *                                                                                *
 * Purpose: escaping of symbols for using in xpath expression                     *
 *                                                                                *
 * Parameters: data - [IN/OUT] the string to update                               *
 *                                                                                *
 **********************************************************************************/
void xml_escape_xpath(char **data)
{
	size_t	size;
	char	*buffer;

	if (0 == (size = xml_escape_xpath_stringsize(*data)))
		return;

	buffer = zbx_malloc(NULL, size + 1);
	buffer[size] = '\0';
	xml_escape_xpath_string(buffer, *data);
	zbx_free(*data);
	*data = buffer;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_query_xpath                                                  *
 *                                                                            *
 * Purpose: execute xpath query                                               *
 *                                                                            *
 * Parameters: value  - [IN/OUT] the value to process                         *
 *             params - [IN] the operation parameters                         *
 *             errmsg - [OUT] error message                                   *
 *                                                                            *
 * Return value: SUCCEED - the value was processed successfully               *
 *               FAIL - otherwise                                             *
 *                                                                            *
 ******************************************************************************/
int	zbx_query_xpath(zbx_variant_t *value, const char *params, char **errmsg)
{
#ifndef HAVE_LIBXML2
	ZBX_UNUSED(value);
	ZBX_UNUSED(params);
	*errmsg = zbx_dsprintf(*errmsg, "Zabbix was compiled without libxml2 support");
	return FAIL;
#else
	xmlDoc		*doc = NULL;
	xmlXPathContext	*xpathCtx;
	xmlXPathObject	*xpathObj;
	xmlNodeSetPtr	nodeset;
	xmlErrorPtr	pErr;
	xmlBufferPtr	xmlBufferLocal;
	int		ret = FAIL, i;
	char		buffer[32], *ptr;

	if (NULL == (doc = xmlReadMemory(value->data.str, strlen(value->data.str), "noname.xml", NULL, 0)))
	{
		if (NULL != (pErr = xmlGetLastError()))
			*errmsg = zbx_dsprintf(*errmsg, "cannot parse xml value: %s", pErr->message);
		else
			*errmsg = zbx_strdup(*errmsg, "cannot parse xml value");
		return FAIL;
	}

	xpathCtx = xmlXPathNewContext(doc);

	if (NULL == (xpathObj = xmlXPathEvalExpression((xmlChar *)params, xpathCtx)))
	{
		pErr = xmlGetLastError();
		*errmsg = zbx_dsprintf(*errmsg, "cannot parse xpath: %s", pErr->message);
		goto out;
	}

	switch (xpathObj->type)
	{
		case XPATH_NODESET:
			xmlBufferLocal = xmlBufferCreate();

			if (0 == xmlXPathNodeSetIsEmpty(xpathObj->nodesetval))
			{
				nodeset = xpathObj->nodesetval;
				for (i = 0; i < nodeset->nodeNr; i++)
					xmlNodeDump(xmlBufferLocal, doc, nodeset->nodeTab[i], 0, 0);
			}
			zbx_variant_clear(value);
			zbx_variant_set_str(value, zbx_strdup(NULL, (const char *)xmlBufferLocal->content));

			xmlBufferFree(xmlBufferLocal);
			ret = SUCCEED;
			break;
		case XPATH_STRING:
			zbx_variant_clear(value);
			zbx_variant_set_str(value, zbx_strdup(NULL, (const char *)xpathObj->stringval));
			ret = SUCCEED;
			break;
		case XPATH_BOOLEAN:
			zbx_variant_clear(value);
			zbx_variant_set_str(value, zbx_dsprintf(NULL, "%d", xpathObj->boolval));
			ret = SUCCEED;
			break;
		case XPATH_NUMBER:
			zbx_variant_clear(value);
			zbx_snprintf(buffer, sizeof(buffer), ZBX_FS_DBL, xpathObj->floatval);

			/* check for nan/inf values - isnan(), isinf() is not supported by c89/90    */
			/* so simply check the if the result starts with digit (accounting for -inf) */
			if (*(ptr = buffer) == '-')
				ptr++;
			if (0 != isdigit(*ptr))
			{
				del_zeros(buffer);
				zbx_variant_set_str(value, zbx_strdup(NULL, buffer));
				ret = SUCCEED;
			}
			else
				*errmsg = zbx_strdup(*errmsg, "Invalid numeric value");
			break;
		default:
			*errmsg = zbx_strdup(*errmsg, "Unknown result");
			break;
	}
out:
	if (NULL != xpathObj)
		xmlXPathFreeObject(xpathObj);

	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);

	return ret;
#endif
}

/******************************************************************************
 *                                                                            *
 * Function: compare_xml_nodes_by_name                                        *
 *                                                                            *
 * Purpose: compare two xml nodes by name                                     *
 *                                                                            *
 * Comments: This function is used to sort xml nodes by name             .    *
 *                                                                            *
 ******************************************************************************/
static int	compare_xml_nodes_by_name(const void *d1, const void *d2)
{
	zbx_xml_node_t	*p1 = *(zbx_xml_node_t **)d1;
	zbx_xml_node_t	*p2 = *(zbx_xml_node_t **)d2;
	int 		res;

	res = strcmp(p1->name, p2->name);
	if (0 == res)
	{
		p1->array = XML_JSON_TRUE;
		p2->array = XML_JSON_TRUE;
	}

	return res;
}


static void	zbx_xml_node_free(zbx_xml_node_t *node)
{
	zbx_vector_ptr_clear_ext(&node->chnodes, (zbx_clean_func_t)zbx_xml_node_free);
	zbx_vector_ptr_destroy(&node->chnodes);
	zbx_vector_str_clear_ext(&node->attributes, zbx_str_free);
	zbx_vector_str_destroy(&node->attributes);
	zbx_free(node->name);
	zbx_free(node->value);
	zbx_free(node);
}

static void xml_to_vector(xmlNode *xml_node, zbx_vector_ptr_t *nodes)
{
	xmlChar			*value;
	xmlAttr			*attr;

	for (;NULL != xml_node; xml_node = xml_node->next)
	{
		zbx_xml_node_t *node;

		node = (zbx_xml_node_t *)zbx_malloc(NULL, sizeof(zbx_xml_node_t));
		node->name = NULL;
		if (0 != xml_node->name)
			node->name = zbx_strdup(NULL,(char *)xml_node->name);
		node->value = NULL;
		node->array = XML_JSON_FALSE;

		zbx_vector_ptr_create(&node->chnodes);
		zbx_vector_str_create(&node->attributes);

		switch (xml_node->type)
		{
			case XML_TEXT_NODE:
				if (NULL == (value = xmlNodeGetContent(xml_node)))
					break;

				node->value = zbx_strdup(NULL,(char *)value);
				xmlFree(value);
				break;
			case XML_CDATA_SECTION_NODE:
				if (NULL == (value = xmlNodeGetContent(xml_node)))
					break;
				node->value = zbx_strdup(NULL,(char *)value);
				node->name = zbx_strdup(node->name, XML_CDATA_NAME);
				xmlFree(value);
				break;
			case XML_ELEMENT_NODE:
				for (attr = xml_node->properties; NULL != attr; attr = attr->next)
				{
					char	*attr_name = NULL;
					size_t	attr_name_alloc = 0, attr_name_offset = 0;

					if (NULL == attr->name)
						continue;

					zbx_snprintf_alloc(&attr_name, &attr_name_alloc, &attr_name_offset, "@%s",
							attr->name);
					value = xmlGetProp(xml_node, attr->name);
					zbx_vector_str_append(&node->attributes, attr_name);
					zbx_vector_str_append(&node->attributes, zbx_strdup(NULL, (char *)value));
					xmlFree(value);

				}
				break;
			default:
				break;

		}
		xml_to_vector(xml_node->children, &node->chnodes);
		zbx_vector_ptr_append(nodes, node);
		zbx_vector_ptr_sort(nodes, compare_xml_nodes_by_name);
	}
}

static int is_data(zbx_xml_node_t *node)
{
	if (0 == strcmp(XML_TEXT_NAME, node->name) || 0 == strcmp(XML_CDATA_NAME, node->name))
		return SUCCEED;

	return FAIL;
}

static void vector_to_json(zbx_vector_ptr_t *nodes, struct zbx_json *json, char **text)
{
	int		i, j;
	int		is_object;
	int		arr_cnt = 0;
	char		*arr_name = NULL;
	char		*tag, *out_text;
	zbx_xml_node_t	*node;

	*text = NULL;
	for (i = 0; i < nodes->values_num; i++)
	{
		node = (zbx_xml_node_t *)nodes->values[i];

		if ((XML_JSON_FALSE == node->array && 0 != arr_cnt) || (XML_JSON_TRUE == node->array &&
				NULL != arr_name && 0 != strcmp(arr_name, node->name)))
		{
			zbx_json_close(json);
			arr_name = NULL;
			arr_cnt = 0;
		}

		if (XML_JSON_TRUE == node->array && 0 == arr_cnt)
		{
			zbx_json_addarray(json, node->name);
			arr_name = node->name;
		}
		if (XML_JSON_TRUE == node->array)
			arr_cnt++;

		is_object = XML_JSON_FALSE;
		if (0 != node->chnodes.values_num)
		{
			zbx_xml_node_t	*chnode;

			chnode = (zbx_xml_node_t*)node->chnodes.values[0];
			if (FAIL == is_data(chnode))
				is_object = XML_JSON_TRUE;
		}
		if (0 != node->attributes.values_num)
			is_object = XML_JSON_TRUE;

		if (XML_JSON_TRUE == is_object)
		{
			tag = node->name;
			if (0 != arr_cnt)
				tag = NULL;
			zbx_json_addobject(json, tag);
		}

		for(j = 0; j < node->attributes.values_num; j+=2)
		{
			zbx_json_addstring(json, node->attributes.values[j], node->attributes.values[j + 1],
					ZBX_JSON_TYPE_STRING);
		}

		vector_to_json(&node->chnodes, json, &out_text);

		*text = node->value;

		if (NULL != out_text || (XML_JSON_FALSE == is_object && FAIL == is_data(node)))
		{
			tag = node->name;
			if (0 != node->attributes.values_num)
				tag = XML_TEXT_TAG;
			else if (0 != arr_cnt)
				tag = NULL;
			zbx_json_addstring(json, tag, out_text, ZBX_JSON_TYPE_STRING);
		}

		if (XML_JSON_TRUE == is_object)
		{
			zbx_json_close(json);
		}
	}
	if (0 != arr_cnt)
		zbx_json_close(json);
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_xml_to_json                                                  *
 *                                                                            *
 * Purpose: convert XML format value to JSON format                           *
 *                                                                            *
 * Parameters: data   - [IN] the XML data to process                          *
 *             jstr   - [OUT] the JSON output                                 *
 *             errmsg - [OUT] error message                                   *
 *                                                                            *
 * Return value: SUCCEED - the value was processed successfully               *
 *               FAIL - otherwise                                             *
 *                                                                            *
 ******************************************************************************/
int	zbx_xml_to_json(char *data, char **jstr, char **errmsg)
{
#ifndef HAVE_LIBXML2
	ZBX_UNUSED(data);
	ZBX_UNUSED(jstr);
	*errmsg = zbx_dsprintf(*errmsg, "Zabbix was compiled without libxml2 support");
	return FAIL;
#else
	struct zbx_json		json;
	xmlDoc			*doc = NULL;
	xmlErrorPtr		pErr;
	xmlNode			*node;
	int			ret = FAIL;
	zbx_vector_ptr_t	nodes;
	char*			out;

	if (NULL == (doc = xmlReadMemory(data, strlen(data), "noname.xml", NULL, 0)))
	{
		if (NULL != (pErr = xmlGetLastError()))
			*errmsg = zbx_dsprintf(*errmsg, "cannot parse xml value: %s", pErr->message);
		else
			*errmsg = zbx_strdup(*errmsg, "cannot parse xml value");
		goto exit;
	}

	zbx_json_init(&json, ZBX_JSON_STAT_BUF_LEN);

	if (NULL == (node = xmlDocGetRootElement(doc)))
	{
		*errmsg = zbx_dsprintf(*errmsg, "Cannot parse XML root");
		goto clean;
	}

	zbx_vector_ptr_create(&nodes);

	xml_to_vector(node, &nodes);
	vector_to_json(&nodes, &json, &out);
	*jstr = zbx_strdup(*jstr, json.buffer);
	ret = SUCCEED;

	zbx_vector_ptr_clear_ext(&nodes, (zbx_clean_func_t)zbx_xml_node_free);
	zbx_vector_ptr_destroy(&nodes);
clean:
	xmlFreeDoc(doc);
	zbx_json_free(&json);
exit:
	return ret;
#endif
}

static void json_to_node(struct zbx_json_parse *jp, xmlDoc *doc, xmlNode *parent_node, char *arr_name, int deep,
		char **attr, char **attr_val, char **text)
{
	xmlNode			*node;
	const char		*p = NULL, *p1 = NULL;
	struct zbx_json_parse	jp_data;
	char			name[MAX_STRING_LEN], value[MAX_STRING_LEN];
	char 			*attr_loc = NULL, *attr_val_loc = NULL, *text_loc = NULL, *array_loc;
	int			idx;
	int			set_attr, set_text;
	char 			*pname, *pvalue;
	zbx_json_type_t		type;

	idx = 0;
	pvalue = NULL;
	do
	{
		set_attr = 0;
		set_text = 0;
		array_loc = NULL;
		pname = NULL;

		if (NULL != (p = zbx_json_pair_next(jp, p, name, sizeof(name))))
		{
			pname = name;

			if (NULL == zbx_json_decodevalue(p, value, sizeof(value), &type))
				type = zbx_json_valuetype(p);
			else
				pvalue = xml_escape_dyn(value);
			if ('@' == name[0])
				set_attr = 1;
			else if (0 == strcmp(name, XML_TEXT_TAG))
				set_text = 1;
		}
		else
		{
			p = p1;
			if (NULL != (p = zbx_json_next_value(jp, p, value, sizeof(value), &type)))
			{
				pvalue = xml_escape_dyn(value);
			}
			else
			{
				p = p1;
				if (NULL != (p = zbx_json_next(jp, p)))
					type = zbx_json_valuetype(p);
			}
		}
		p1 = p;

		if (0 != set_attr)
		{
			*attr = zbx_strdup(*attr, &name[1]);
			if (NULL != pvalue)
				*attr_val = zbx_strdup(*attr_val, pvalue);
		}
		else if (0 != set_text && NULL != pvalue)
		{
			*text = zbx_strdup(*text, pvalue);
		}
		else if (NULL != p)
		{
			pname = (NULL == arr_name) ? pname : arr_name;
			node = NULL;

			if (ZBX_JSON_TYPE_ARRAY == type)
			{
				array_loc = name;
				node = parent_node;
			}
			else if (ZBX_JSON_TYPE_OBJECT == type || ZBX_JSON_TYPE_UNKNOWN == type)
			{
				node = xmlNewDocNode(doc, NULL, (xmlChar *)pname, NULL);
			}
			else
				node = xmlNewDocNode(doc, NULL, (xmlChar *)pname, (xmlChar *)pvalue);

			if (0 == deep)
			{
				if (0 < idx)
					break;
				if (node != NULL)
					xmlDocSetRootElement(doc, node);
			}
			else
			{
				if (node != NULL && node != parent_node)
					node = xmlAddChild(parent_node, node);
			}

			if (SUCCEED == zbx_json_brackets_open(p, &jp_data))
			{
				json_to_node(&jp_data, doc, node, array_loc, deep+1, &attr_loc, &attr_val_loc,
						&text_loc);
			}
		}

		if (NULL != attr_loc)
			xmlNewProp(node, (xmlChar *)attr_loc, (xmlChar *)attr_val_loc);
		if (NULL != text_loc)
			xmlNodeSetContent(node, (xmlChar *)text_loc);

		zbx_free(attr_loc);
		zbx_free(attr_val_loc);
		zbx_free(text_loc);
		zbx_free(pvalue);
		idx++;
	}
	while (NULL != p);
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_json_to_xml                                                  *
 *                                                                            *
 * Purpose: convert JSON format value to XML format                           *
 *                                                                            *
 * Parameters: data   - [IN] the JSON data to process                         *
 *             jstr   - [OUT] the XML output                                  *
 *             errmsg - [OUT] error message                                   *
 *                                                                            *
 * Return value: SUCCEED - the value was processed successfully               *
 *               FAIL - otherwise                                             *
 *                                                                            *
 ******************************************************************************/
int	zbx_json_to_xml(char *data, char **xstr, char **errmsg)
{
#ifndef HAVE_LIBXML2
	ZBX_UNUSED(data);
	ZBX_UNUSED(jstr);
	*errmsg = zbx_dsprintf(*errmsg, "Zabbix was compiled without libxml2 support");
	return FAIL;
#else
	struct zbx_json_parse	jp;
	char 			*attr = NULL, *attr_val = NULL, *text = NULL;

	xmlDoc 			*doc = NULL;
	xmlErrorPtr		pErr;
	xmlChar			*xmem;
	int			size, ret = FAIL;

	if (NULL == (doc = xmlNewDoc(BAD_CAST XML_DEFAULT_VERSION)))
	{
		if (NULL != (pErr = xmlGetLastError()))
			*errmsg = zbx_dsprintf(*errmsg, "cannot parse xml value: %s", pErr->message);
		else
			*errmsg = zbx_strdup(*errmsg, "cannot parse xml value");
		goto exit;
	}

	if (SUCCEED != zbx_json_open(data, &jp))
	{
		*errmsg = zbx_strdup(*errmsg, zbx_json_strerror());
		goto clean;
	}

	json_to_node(&jp, doc, NULL, NULL, 0, &attr, &attr_val, &text);

	xmlDocDumpMemory(doc, &xmem, &size);

	zbx_free(text);
	zbx_free(attr_val);
	zbx_free(attr);

	if (NULL == xmem)
	{
		if (NULL != (pErr = xmlGetLastError()))
			*errmsg = zbx_dsprintf(*errmsg, "Cannot save XML: %s", pErr->message);
		else
			*errmsg = zbx_strdup(*errmsg, "Cannot save XML");

		goto clean;
	}

	*xstr = zbx_malloc(*xstr, (size_t)size + 1);
	memcpy(*xstr, (const char *)xmem, (size_t)size + 1);
	xmlFree(xmem);
	ret = SUCCEED;
clean:
	xmlFreeDoc(doc);
exit:
	return ret;
#endif
}
