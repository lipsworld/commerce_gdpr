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
 * Allows altering information about anonymizable entity properties.
 *
 * NOTE: because of inability to save a customer profile that is bound
 * to an order, it is not allowed to add properties for
 * commerce_customer_profile entity type here.
 *
 * @param array $entity_property_info
 *   Array containing information about anonymizable entity properties.
 *
 * @see _commerce_gdpr_get_entity_property_info
 */
function hook_commerce_gdpr_entity_property_info_alter(array &$entity_property_info) {
  $entity_property_info['user']['mail']['type'] = 'clear';
}

/**
 * Allows to act on an entity just before the anonymization takes place.
 *
 * @param string $type
 *   Entity type.
 * @param object $entity
 *   Drupal entity.
 * @param array $properties_data
 *   Array of entity property anonymization data.
 * @param array $field_data
 *   Array of field anonymization data.
 */
function hook_commerce_gdpr_entity_anonymization($type, $entity, array $properties_data, array $field_data) {
  if ($type === 'node') {
    db_delete('some_custom_table')->condition('nid', $entity->nid)->execute();
  }
}
