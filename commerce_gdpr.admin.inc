<?php

/**
 * @file
 * Contains admin forms for the module.
 */

/**
 * Configuration form builder.
 */
function commerce_gdpr_admin_form($form, &$form_state) {

  // Issue a warining if a site-specific hashing salt is not set.
  if (!variable_get('commerce_gdpr_salt', FALSE) && empty($form_state['input'])) {
    drupal_set_message(t('To ensure increased safety of anonymized data, please set a platform-specific hashing salt. More information can be found on the module !help.', array(
      '!help' => l(t('help page'), 'admin/help/commerce_gdpr', array(
        'attributes' => array('target' => '_blank'),
      )),
    )), 'warning');
  }

  $form['data_retention'] = array(
    '#type' => 'textfield',
    '#title' => t('Order data retention period'),
    '#default_value' => variable_get('commerce_gdpr_data_retention', 0),
    '#description' => t('Enter number of days after which inactive data will be anonymized, 0 - no automatic anonymization.'),
  );

  $form['direct_processing'] = array(
    '#type' => 'checkbox',
    '#title' => t('Process items immediately'),
    '#description' => t('Process items immediately on the user form and during the "People" page bulk operation rather then enqueing the operations. NOTE: may cause performance issues in case of users with many orders.'),
    '#default_value' => variable_get('commerce_gdpr_direct_processing', 0),
  );

  $form['user_button_text'] = array(
    '#type' => 'textfield',
    '#title' => t('User account anonymization button text'),
    '#default_value' => variable_get('commerce_gdpr_user_button_text', 'I want to be forgotten'),
    '#required' => TRUE,
  );

  $form['anonymized_fields'] = array(
    '#type' => 'fieldset',
    '#title' => t('Fields subject to anonymization'),
    '#tree' => TRUE,
  );

  $entity_info = entity_get_info();
  $selected_fields = variable_get('commerce_gdpr_anonymized_fields', array());

  foreach (array_keys(_commerce_gdpr_get_entity_property_info()) as $type) {
    $entity_info = entity_get_info($type);
    $form['anonymized_fields'][$type] = array(
      '#type' => 'fieldset',
      '#title' => $entity_info['label'],
    );
    foreach ($entity_info['bundles'] as $bundle => $bundle_info) {
      if (count($entity_info['bundles']) > 1) {
        $form['anonymized_fields'][$type][$bundle] = array(
          '#type' => 'fieldset',
          '#title' => $bundle_info['label'],
        );
      }
      else {
        $form['anonymized_fields'][$type]['#title'] = $bundle_info['label'];
      }

      $field_instances = field_info_instances($type, $bundle);
      if (empty($field_instances)) {
        $form['anonymized_fields'][$type][$bundle]['info'] = array(
          '#markup' => t('No field instances available.'),
        );
      }
      else {
        foreach ($field_instances as $field_name => $instance_info) {

          // Excluse customer profile type fields, those will be anonymized
          // along with the order. Also exclude line items.
          // TODO: Actually all entity reference field types should be
          // handled differently, but it's not needed at the moment.
          $excluded_fields = array(
            'commerce_line_item_reference',
            'commerce_customer_profile_reference',
          );
          $info = field_info_field($field_name);
          if (in_array($info['type'], $excluded_fields)) {
            continue;
          }

          $form['anonymized_fields'][$type][$bundle][$field_name] = array(
            '#type' => 'checkbox',
            '#title' => $instance_info['label'],
            '#default_value' => !empty($selected_fields[$type][$bundle][$field_name]),
          );
        }
      }
    }
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save settings'),
  );

  return $form;
}

/**
 * Admin form validate handler.
 */
function commerce_gdpr_admin_form_validate($form, &$form_state) {
  if (!is_numeric($form_state['values']['data_retention'])) {
    form_set_error('data_retention', t('Enter a valid data retention value.'));
  }
}

/**
 * Admin form submit handler.
 */
function commerce_gdpr_admin_form_submit($form, &$form_state) {
  variable_set('commerce_gdpr_data_retention', $form_state['values']['data_retention']);
  variable_set('commerce_gdpr_direct_processing', $form_state['values']['direct_processing']);
  variable_set('commerce_gdpr_user_button_text', $form_state['values']['user_button_text']);

  $selected_fields = array();
  foreach ($form_state['values']['anonymized_fields'] as $type => $type_data) {
    foreach ($type_data as $bundle => $bundle_data) {
      foreach ($bundle_data as $field_name => $value) {
        if ($value) {
          $selected_fields[$type][$bundle][$field_name] = $field_name;
        }
      }
    }
  }
  variable_set('commerce_gdpr_anonymized_fields', $selected_fields);

  drupal_set_message(t('Settings saved.'));
}
