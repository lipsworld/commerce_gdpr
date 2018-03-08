TABLE OF CONTENTS
-----------------

 * Introduction
 * Requirements
 * Installation
 * Configuration


INTRODUCTION
------------

This module helps Drupal Commere sites comply with the EU GDPR directive by
providing means to anonymize user and order data.
Anonymized data can still be used for statistical purposes but will not allow
to identify a person.
The module adds UI for users to anonimize their data and means for the site
administrators to setup automatic anonymization after a defined amount of time.


REQUIREMENTS
------------

* Order module (commerce_order, included in the Drupal Commerce package
  - https://www.drupal.org/project/commerce),
* Chaos tool suite (ctools, also required by Drupal Commerce
  - https://www.drupal.org/project/ctools).


INSTALLATION
------------

 * Install as you would normally install a contributed Drupal module. See:
   https://drupal.org/documentation/install/modules-themes/modules-7
   for further information.


CONFIGURATION
-------------

1. settings.php

By default, the module uses md5 algorithm and the $drupal_hash_salt value
for hashing user data. It is strongly advised to change at least the salt
for improved security by including the following code in settings.php:

<code>
$conf['commerce_gdpr_salt'] = 'some_salt';
$conf['commerce_gdpr_algo'] = 'some_algo';
</code>

For enhanced security, you may set this variable to a value using the
contents of a file outside your docroot that is never saved together
with any backups of your Drupal files and database.

Example:
<code>
$conf['commerce_gdpr_salt'] = file_get_contents('/home/example/anonimyzation_salt.txt');
</code>

The salt needs to be at least 22 characters long and can be generated once using
<code>hash('some_algorithm', mt_rand());</code>
or just entered manually.

For a list of supported hashing algorithms see
http://php.net/manual/en/function.hash-hmac-algos.php.

NOTE: Those config values must be entered only once when the module is
installed and should not be changed after data has already been anonimized,
otherwise comparing data from before and after the change will be impossible.


2. Admin UI

By default, only entity properties are anonimized and automatic anonimization
is switched off. To select entity felds that should also be anonimized and
time after automatic anonymization should take place, go to the module
configuration form (/admin/commerce/config/commerce-gdpr).

3. Custom entity properties

See commerce_gdpr.api.php.
