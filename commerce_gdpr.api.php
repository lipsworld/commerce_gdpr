<?php

/**
 * @file
 * Commerce GDPR API documentation.
 *
 * This file contains no working PHP code; it exists to provide additional
 * documentation for doxygen as well as to document hooks in the standard
 * Drupal manner.
 */

/**
 * Allows altering information about anonimizable entity properties.
 *
 * NOTE: because of inability to save a customer profile that is bound
 * to an order, it is not allowed to add properties for
 * commerce_customer_profile entity type here.
 *
 * @param array $entity_property_info
 *   Array containing information about anonimizable entity properties.
 *
 * @see _commerce_gdpr_get_entity_property_info
 */
function hook_commerce_gdpr_entity_property_info_alter(array &$entity_property_info) {
  $entity_property_info['user']['mail']['type'] = 'clear';
}
