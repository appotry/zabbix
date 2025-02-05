<?php
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

?>

<script>
	const mediaView = new class {
		init({form_name, medias, severity_config, mediatypes}) {
			this.form_name = form_name;
			this.medias = medias;
			this.severity_config = severity_config;
			this.row_index = 1;
			this.mediatypes = mediatypes;

			this._handleAction();
			this._initMedias();
		}

		_handleAction() {
			document.querySelector('#mediaTab').addEventListener('click', ({target}) => {
				if (target.classList.contains('js-add')) {
					this._addMedia();
				}
				else if (target.classList.contains('js-edit')) {
					this._editMedia(JSON.parse(target.dataset.parameters));
				}
				else if (target.classList.contains('js-remove')) {
					this._removeMedia(target);
				}
				else if (target.classList.contains('js-status')) {
					this._statusMedia(target);
				}
			});
		}

		_addMedia() {
			const overlay = PopUp('popup.media', {'dstfrm': this.form_name}, {
				dialogueid: 'user-media-edit',
				dialogue_class: 'modal-popup-generic'
			});

			overlay.$dialogue[0].addEventListener('dialogue.submit', ({detail}) => {
				this._generateRow(detail);
			});
		}

		_editMedia(parameters) {
			const overlay = PopUp('popup.media', parameters, {
				dialogueid: 'user-media-edit',
				dialogue_class: 'modal-popup-generic'
			});

			overlay.$dialogue[0].addEventListener('dialogue.submit', ({detail}) => {
				this._removeRow(parameters.media);
				this._generateRow(detail, parameters.media, true);
			});
		}

		_removeMedia(target) {
			this._removeRow(target.closest('tr').id.replace('medias_', ''), false);
		}

		_statusMedia(target) {
			const btn_edit = target.closest('tr').querySelector('button.js-edit');
			const parsed_parameters = JSON.parse(btn_edit.dataset.parameters);

			if (target.dataset.status_type === 'enable_media') {
				parsed_parameters.active = '0';
				this._createButtonEnabled(target);
			}
			else {
				parsed_parameters.active = '1';
				this._createButtonDisabled(target);
			}

			document.querySelector(`#medias_${parsed_parameters.media}_active`).value = parsed_parameters.active;
			btn_edit.dataset.parameters = JSON.stringify(parsed_parameters);
		}

		_initMedias() {
			for (const index in this.medias) {
				if (Number.isInteger(parseInt(index))) {
					this.row_index = index;
					this._generateRow(this.medias[index], index, false);
				}
			}
		}

		_generateRow(data, index = undefined, edit = false) {
			const table_body = document.querySelector('#media-table tbody');
			const temp = this._createTemplate(data, index);

			if (edit === false) {
				table_body.prepend(temp);
			}
			else {
				table_body.replaceChild(temp, document.querySelector(`#medias_${data.row_index}`));
			}
		}

		_createButtonEnabled(button) {
			button.classList.replace('<?= ZBX_STYLE_RED ?>', '<?= ZBX_STYLE_GREEN ?>');
			button.textContent = t('Enabled');

			Object.assign(button.dataset, {status_type: 'disable_media'});
		}

		_createButtonDisabled(button) {
			button.classList.replace('<?= ZBX_STYLE_GREEN ?>', '<?= ZBX_STYLE_RED ?>');
			button.textContent = t('Disabled');

			Object.assign(button.dataset, {status_type: 'enable_media'});
		}

		_createInput(index, key, value) {
			const input = document.createElement('input');

			input.type = 'hidden';
			input.name = `medias[${index}][${key}]`;
			input.value = value;
			input.id = `medias_${index}_${key}`;

			return input;
		}

		_createSeverity(data, severities_span) {
			let severity = <?= TRIGGER_SEVERITY_NOT_CLASSIFIED ?>;

			for (;severity < <?= TRIGGER_SEVERITY_COUNT ?>; severity++) {
				const media_active = (data.severity & (1 << severity)) !== 0;
				const span = severities_span[severity];
				const hintboxData = {hintbox: 1, hintboxContents: this.severity_config.names[severity]};

				if (media_active) {
					span.classList.replace('<?= ZBX_STYLE_STATUS_DISABLED ?>', this.severity_config.colors[severity]);
					hintboxData.hintboxContents += ' (on)';
				} else {
					span.classList.replace(this.severity_config.colors[severity], '<?= ZBX_STYLE_STATUS_DISABLED ?>');
					hintboxData.hintboxContents += ' (off)';
				}

				hintboxData.hintboxContents = t(hintboxData.hintboxContents)
				Object.assign(span.dataset, hintboxData);
			}
		}

		_createHiddenInputs(data) {
			const form = document.querySelector(`[name="${this.form_name}"]`);

			for (const key in data) {
				form.appendChild(this._createInput(data.row_index, key, data[key]));
			}
		}

		_createWarningButton(text) {
			const button = document.createElement('button');

			button.type = 'button';
			button.classList.add('btn-icon', 'zi-i-warning', 'btn-small');

			Object.assign(button.dataset, {
				hintboxContents: t(text),
				hintbox: 1,
				hintboxClass: 'hintbox-wrap',
				hintboxStatic: 1,
				expanded: 'true'
			});

			return button;
		}

		_truncateText(text) {
			return text.length > 50 ? `${text.substring(0, 50)}...` : text;
		}

		_removeRow(index, only_inputs = true) {
			document.querySelectorAll(`[name^="medias[${index}]["]`).forEach((element) => element.remove());

			if (!only_inputs) {
				document.querySelector(`#medias_${index}`).remove();
			}
		}

		_createDataTemplate(data, index) {
			data.row_index = index === undefined ? ++this.row_index : index;
			data.dstfrm =  this.form_name;
			data.media = data.row_index;
			data.parameters = structuredClone(data);
			data.sendto_short = this._truncateText(data.sendto);

			if (data.name === undefined) {
				data.name = this.mediatypes[data.mediatypeid].name;
			}

			if (this.mediatypes[data.mediatypeid] === undefined) {
				data.name = t('Unknown');
			}

			if (data.mediatype == <?= MEDIA_TYPE_EMAIL ?>) {
				data.parameters.sendto_emails = data.sendto;
			} else {
				if (Array.isArray(data.sendto)) {
					data.sendto = data.sendto.join(', ');
				}
				data.parameters.sendto = data.sendto;
			}

			data.parameters = JSON.stringify(data.parameters);
		}

		_createTemplate(data, index) {
			this._createDataTemplate(data, index);

			const template = new Template(document.getElementById('media-row-tmpl').innerHTML);
			const temp = template.evaluateToElement(data);

			if (data.sendto.length <= 50) {
				temp.querySelector('[data-hintbox]').dataset.hintbox = '0';
			}

			const button = temp.querySelector('button.js-status');

			if (this.mediatypes[data.mediatypeid] === undefined) {
				temp.querySelector('td').classList.add('<?= ZBX_STYLE_DISABLED ?>');
				this._createButtonDisabled(button);
				button.classList.remove('js-status');
			}
			else if (this.mediatypes[data.mediatypeid].status == <?= MEDIA_TYPE_STATUS_ACTIVE ?>) {
				if (data.active == <?= MEDIA_STATUS_ACTIVE ?>) {
					this._createButtonEnabled(button);
				}
				else {
					this._createButtonDisabled(button);
				}
			}
			else {
				this._createButtonDisabled(button);
				button.classList.remove('js-status');

				const warning_button = this._createWarningButton('Media type disabled by Administration.');

				temp.querySelector('td').appendChild(warning_button);
			}

			this._createSeverity(data, temp.querySelectorAll('.status-container span'));
			this._createHiddenInputs(data);

			return temp;
		}
	}
</script>
