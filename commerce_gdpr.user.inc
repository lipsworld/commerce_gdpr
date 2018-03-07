<?php

/**
 * @file
 * Contains end user UI functionality.
 */

/**
 * Implements hook_user_view().
 */
function commerce_gdpr_user_view($account, $view_mode, $langcode) {
  if ($account->uid == $GLOBALS['user']->uid || user_access('anonymize other users data')) {

    // Check if the user is already anonymized or has been scheduled
    // for anonymization.
    $last_access = _commerce_gdpr_get_info('user', $account->uid);
    if ($last_access === 0) {
      drupal_set_message(t('This user account data has been anonymized.'));
    }
    elseif (!empty($account->data['commerce_gdpr_anonymization'])) {
      drupal_set_message(t('This account has been scheduled for anonymization.'), 'warning');
    }
    else {
      $account->content['gdpr_widget'] = drupal_get_form('commerce_gdpr_user_form', $account->uid);
      $account->content['gdpr_widget']['#weight'] = 999;
    }
  }
}

/**
 * Form builder for gdpr widget.
 */
function commerce_gdpr_user_form($form, &$form_state, $uid) {
  $form_state['uid'] = $uid;

  ctools_include('modal');
  ctools_modal_add_js();
  drupal_add_js(array(
    'commerce-gdpr-modal' => array(
      'modalSize' => array(
        'type' => 'fixed',
        'width' => 300,
        'height' => 300,
      ),
      'animation' => 'fadeIn',
    ),
  ), 'setting');

  $form['invoke_confirm'] = array(
    '#type' => 'submit',
    '#value' => variable_get('commerce_gdpr_user_button_text', 'I want to be forgotten'),
    '#ajax' => array(
      'callback' => 'commerce_gdpr_user_ajax',
    ),
    '#attributes' => array(
      'class' => array('ctools-modal-commerce-gdpr-modal'),
    ),
  );

  return $form;
}

/**
 * Modal form builder.
 */
function commerce_gdpr_modal_form($form, &$form_state) {

  $orders = commerce_order_load_multiple(array(), array('uid' => $form_state['uid']));
  $pending = 0;
  foreach ($orders as $order) {
    $status = commerce_order_status_load($order->status);
    if ($status['state'] == 'pending') {
      $pending++;
    }
  }

  if ($pending) {
    $form['message'] = array(
      '#markup' => t(
        '<strong>Warning</strong>: we detected you still have !n_orders. We will be unable to process those orders after your data will be anonymized. Please review your !orders_link. Do you still wish to continue?',
        array(
          '!n_orders' => format_plural($pending, '1 pending order', '!n pending orders', array('!n' => $pending)),
          '!orders_link' => l(t('orders'), 'user/' . $form_state['uid'] . '/orders'),
        )
      ),
    );
  }
  else {
    if ($form_state['uid'] == $GLOBALS['user']->uid) {
      $form['message'] = array(
        '#markup' => t('This will anonymize all your user, order and customer profile data so it will only be possible to use for statistical purposes. It will also make your account inactive and log you out of the site instantly. Are you sure?'),
      );
    }
    else {
      $form['message'] = array(
        '#markup' => t('Are you sure you wish to anonymize data of this user?'),
      );
    }
  }

  $form['actions'] = array(
    '#type' => 'actions',
  );
  $form['actions']['back'] = array(
    '#type' => 'submit',
    '#value' => t('Take me back'),
    '#ajax' => array(
      'callback' => 'commerce_gdpr_user_ajax',
    ),
  );
  $form['actions']['anonymize'] = array(
    '#type' => 'submit',
    '#value' => t("Yes, let's do it"),
    '#ajax' => array(
      'callback' => 'commerce_gdpr_user_ajax',
    ),
  );
  return $form;
}

/**
 * Ajax callback for the user GDPR form.
 */
function commerce_gdpr_user_ajax($form, $form_state) {

  ctools_include('ajax');
  ctools_include('modal');

  $commands = array();
  $trigger = end($form_state['triggering_element']['#parents']);
  if ($trigger === 'back' || $trigger === 'anonymize') {
    $commands[] = ctools_modal_command_dismiss();
    if ($trigger === 'back') {
      $messages = theme('status_messages');
      $commands[] = ajax_command_prepend($form_state['status_wrapper_id'], $messages);
    }
    else {
      $commands[] = ctools_ajax_command_redirect('');
    }
  }
  elseif ($trigger === 'invoke_confirm') {
    $form_state = array(
      'ajax' => TRUE,
      'title' => t('Please confirm'),
      'uid' => $form_state['build_info']['args'][0],
      'status_wrapper_id' => '#' . $form['#id'],
    );

    $commands = ctools_modal_form_wrapper('commerce_gdpr_modal_form', $form_state);
  }

  return array('#type' => 'ajax', '#commands' => $commands);
}

/**
 * User GDPR form submit handler.
 */
function commerce_gdpr_modal_form_submit($form, &$form_state) {
  $trigger = end($form_state['triggering_element']['#parents']);

  if ($trigger === 'anonymize') {

    $account = user_load($form_state['uid']);

    $queue_items = _commerce_gdpr_get_user_queue_items($account);

    if (variable_get('commerce_gdpr_direct_processing', FALSE)) {
      foreach ($queue_items as $item) {
        commerce_gdpr_anonymization_worker($item);
      }
    }
    else {
      $queue = DrupalQueue::get('commerce_gdpr_anonymization');
      foreach ($queue_items as $item) {
        $queue->createItem($item);
      }
    }

    // Logout user if the current user is anonymizing his own account.
    if ($form_state['uid'] == $GLOBALS['user']->uid && $form_state['uid'] != 1) {
      $account = user_load($form_state['uid']);
      $account->status = 0;
      module_invoke_all('user_logout', $account);
      session_destroy();

      $form_state['redirect'] = '';
      if (variable_get('commerce_gdpr_direct_processing', FALSE)) {
        drupal_set_message(t('Your account data has been anonymized.'));
      }
      else {
        drupal_set_message(t('Your account data has been scheduled for anonymization.'));
      }
    }
    else {
      if (variable_get('commerce_gdpr_direct_processing', FALSE)) {
        drupal_set_message(t('Account data has been anonymized.'));
      }
      else {
        drupal_set_message(t('Account data has been scheduled for anonymization.'));
      }
    }

    $account->data['commerce_gdpr_anonymization'] = TRUE;
    user_save($account);
  }
}

/**
 * Router function for bulk anonymize operation.
 */
function commerce_gdpr_user_operations_anonymize_router($accounts) {
  $_SESSION['commerce_gdpr_bulk_anonymize'] = $accounts;
  drupal_goto('admin/people/commerce-gdpr-confirm', array('query' => drupal_get_destination()));
}

/**
 * Bulk anonymization confirmation form builder.
 */
function commerce_gdpr_user_bulk_confirm_form($form, &$form_state) {
  $accounts = user_load_multiple($_SESSION['commerce_gdpr_bulk_anonymize']);
  $to_anonymize = array();
  $already_done = array();
  foreach ($accounts as $account) {
    if (empty($account->anonymized)) {
      $to_anonymize[] = $account;
    }
    else {
      $already_done[] = $account;
    }
  }

  if (!empty($to_anonymize)) {
    $form['to_anonymize'] = array(
      '#theme' => 'item_list',
      '#title' => t("Accounts subject to anonymization"),
      '#items' => array(),
    );
    foreach ($to_anonymize as $account) {
      $form['to_anonymize']['#items'][] = $account->name;
    }
  }
  if (!empty($already_done)) {
    $form['already_done'] = array(
      '#theme' => 'item_list',
      '#title' => t("Accounts already anonymized"),
      '#items' => array(),
    );
    foreach ($already_done as $account) {
      $form['already_done']['#items'][] = $account->name;
    }
  }

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['back'] = array(
    '#type' => 'submit',
    '#value' => t('Back'),
  );
  if (!empty($to_anonymize)) {
    $form['actions']['anonymize'] = array(
      '#type' => 'submit',
      '#value' => t('Anonymize accounts'),
    );
    $form_state['_accounts'] = array();
    foreach ($to_anonymize as $account) {
      $form_state['_accounts'][] = $account->uid;
    }
  }

  return $form;
}

/**
 * Bulk anonymize users operation submit handler.
 */
function commerce_gdpr_user_bulk_confirm_form_submit($form, &$form_state) {
  unset($_SESSION['commerce_gdpr_bulk_anonymize']);

  $op = end($form_state['triggering_element']['#parents']);
  if ($op !== 'anonymize') {
    return;
  }

  $accounts = user_load_multiple($form_state['_accounts']);
  $direct_processing = variable_get('commerce_gdpr_direct_processing', FALSE);
  if ($direct_processing) {
    $queue = DrupalQueue::get('commerce_gdpr_anonymization');
  }

  foreach ($accounts as $account) {
    if (empty($account->anonymized)) {
      $queue_items = _commerce_gdpr_get_user_queue_items($account);
      if ($direct_processing) {
        foreach ($queue_items as $item) {
          commerce_gdpr_anonymization_worker($item);
        }
      }
      else {
        foreach ($queue_items as $item) {
          $queue->createItem($item);
        }
      }
    }
  }
  if ($direct_processing) {
    drupal_set_message(format_plural(count($accounts), '1 account has been anonymized.', '@count accounts have been anonymized.'));
  }
  else {
    drupal_set_message(format_plural(count($accounts), '1 account has been scheduled for anonymization.', '@count accounts have been scheduled for anonymization.'));
  }
}
