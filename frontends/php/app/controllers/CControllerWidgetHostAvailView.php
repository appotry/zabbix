<?php
/*
** Zabbix
** Copyright (C) 2001-2019 Zabbix SIA
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


class CControllerWidgetHostAvailView extends CControllerWidget {

	public function __construct() {
		parent::__construct();

		$this->setType(WIDGET_HOST_AVAIL);
		$this->setValidationRules([
			'name' => 'string',
			'fields' => 'json',
		]);
	}

	// 1 - agent;
	// 2 - SNMP;
	// 3 - IPMI;
	// 4 - JMX.

	protected function doAction() {
		$fields = $this->getForm()->getFieldsData();

		$type_fields = [
			INTERFACE_TYPE_AGENT => 'available',
			INTERFACE_TYPE_SNMP => 'snmp_available',
			INTERFACE_TYPE_JMX => 'jmx_available',
			INTERFACE_TYPE_IPMI => 'ipmi_available'
		];

		$groupids = $fields['groupids'] ? getSubGroups($fields['groupids']) : null;

		$hosts_types = count($fields['interface_type']) === 0 ? array_keys($type_fields) : $fields['interface_type'];

		// $hosts_total = array_fill(0, count($type_fields), 0);
		$hosts_total = array_map(function() {
			return 0;
		}, $type_fields);
		$hosts_count = array_map(function() {
			return [
				HOST_AVAILABLE_UNKNOWN => 0,
				HOST_AVAILABLE_TRUE => 0,
				HOST_AVAILABLE_FALSE => 0,
			];
		}, $type_fields);

		$db_hosts = API::Host()->get([
			'output' => ['available', 'snmp_available', 'jmx_available', 'ipmi_available'],
			'selectInterfaces' => ['interfaceid', 'type'],
			'groupids' => $groupids,
			'filter' => ($fields['maintenance'] == HOST_MAINTENANCE_STATUS_OFF)
				? ['status' => HOST_STATUS_MONITORED, 'maintenance_status' => HOST_MAINTENANCE_STATUS_OFF]
				: ['status' => HOST_STATUS_MONITORED]
		]);

		foreach ($db_hosts as $host) {
			$interface = [];
			foreach ($host['interfaces'] as $val) {
				$interface[] = $val['type'];
			}

			foreach ($interface as $type) {
				$hosts_count[$type][$host[$type_fields[$type]]]++;
				$hosts_total[$type]++;
			}
		}

		$this->setResponse(new CControllerResponseData([
			'name' => $this->getInput('name', $this->getDefaultHeader()),
			'layout' => $fields['layout'],
			'hosts_types' => $hosts_types,
			'hosts_count' => $hosts_count,
			'hosts_total' => $hosts_total,
			'user' => [
				'debug_mode' => $this->getDebugMode()
			]
		]));
	}
}
