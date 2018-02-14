<?php

/**
 * @file
 * Contains end user UI functionality.
 */

/**
 * Implements hook_user_view().
 */
function commerce_gdpr_user_view($account, $view_mode, $langcode) {
  if ($account->uid == $GLOBALS['user']->uid || user_access('anonimize other users data')) {
    $account->content['gdpr_widget'] = drupal_get_form('commerce_gdpr_user_form', $account->uid);
    $account->content['gdpr_widget']['#weight'] = 999;
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
    '#value' => t('I want to be forgotten'),
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
        '#markup' => t('This will anonimize all your user, order and customer profile data so it will only be possible to use for statistical purposes. It will also make your account inactive and log you out of the site instantly. Are you sure?'),
      );
    }
    else {
      $form['message'] = array(
        '#markup' => t('Are you sure yuo wish to anonimize data of this user?'),
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
  $form['actions']['anonimize'] = array(
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
  if ($trigger === 'back' || $trigger === 'anonimize') {
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

  if ($trigger === 'anonimize') {

    $queue_items = array();

    // Better don't anonimize the superuser data.
    if ($form_state['uid'] != 1) {
      $queue_items[] = array(
        'type' => 'user',
        'id' => $form_state['uid'],
      );
    }

    // Get orders.
    $orders = commerce_order_load_multiple(array(), array('uid' => $form_state['uid']));
    foreach ($orders as $order) {
      $queue_items[] = array(
        'type' => 'commerce_order',
        'id' => $order->order_id,
      );
    }

    // Get profiles.
    $profiles = commerce_customer_profile_load_multiple(array(), array('uid' => $form_state['uid']));
    foreach ($profiles as $profile) {
      $queue_items[] = array(
        'type' => 'commerce_customer_profile',
        'id' => $profile->profile_id,
      );
    }

    $queue = DrupalQueue::get('commerce_gdpr_anonymization');
    foreach ($queue_items as $item) {
      $queue->createItem($item);
    }

    // Logout user if the current user is anonymizing his own account.
    if ($form_state['uid'] == $GLOBALS['user']->uid && $form_state['uid'] != 1) {
      $account = user_load($form_state['uid']);
      $account->status = 0;
      user_save($account);
      module_invoke_all('user_logout', $account);
      session_destroy();

      $form_state['redirect'] = '';
      drupal_set_message(t('Your account data has been scheduled for anonimization.'));
    }
    else {
      drupal_set_message(t('Account data has been scheduled for anonymization.'));
    }
  }
}
