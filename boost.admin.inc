<?php
// $Id: boost.admin.inc,v 1.1.2.1.2.3.2.10 2008/10/25 16:09:24 arto Exp $

/**
 * @file
 * All the code for the Boost module's administrative interface.
 */

//////////////////////////////////////////////////////////////////////////////
// Boost settings form

/**
 * Performs alterations to the system settings form before it is rendered.
 *
 * @see hook_form_alter() in boost.module.
 */
function boost_admin_settings($form = array()) {
  // Take over the relevant existing settings
  $form['#title'] = t('Static page cache');
  $form['cache'] = array('#type' => 'hidden','#value' => CACHE_DISABLED);
  unset($form['page_compression']); // we don't support this

  // Inject Boost-specific settings
  $form['boost'] = array(
    '#type'          => 'radios',
    '#title'         => t('Static page cache'),
    '#default_value' => variable_get('boost', CACHE_DISABLED),
    '#options'       => array(CACHE_DISABLED => t('Disabled'), 1 => t('Enabled')),
    '#description'   => t('Static page caching is a mechanism that stores dynamically generated web pages as HTML files in a special cache directory located under the Drupal installation directory. By caching a web page in this manner, the web server can serve it out in the fastest possible manner, without invoking PHP or Drupal at all. While this does provide a significant performance and scalability boost, you should note that it could have negative usability side-effects unless your site is targeted at an audience consisting mostly of "anonymous" visitors.'),
    '#weight'        => -10,
  );
  $form['boost_file_path'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Cache file path'),
    '#default_value' => BOOST_FILE_PATH,
    '#size'          => 60,
    '#maxlength'     => 255,
    '#required'      => TRUE,
    '#description'   => t('A file system path where the static cache files will be stored. This directory has to exist and be writable by Drupal. The default setting is to store the files in a directory named %default-path under the Drupal installation directory. If you change this, you must also change the URL rewrite rules in your web server configuration (.htaccess for Apache, lighttpd.conf for Lighttpd), or caching will not work.', array('%default-path' => boost_cache_directory(NULL, FALSE))),
    '#weight'        => -5,
  );

  // Cacheability settings
  // TODO: update to use Drupal 6.x core code.
  $options = array(t('Cache every page except the listed pages.'), t('Cache only the listed pages.'));
  $description = t("Enter one page per line as Drupal paths. The '*' character is a wildcard. Example paths are '%blog' for the blog page and %blog-wildcard for every personal blog. %front is the front page.", array('%blog' => 'blog', '%blog-wildcard' => 'blog/*', '%front' => '<front>'));
  if (user_access('use PHP for block visibility')) {
    $options[] = t('Cache pages for which the following PHP code returns <code>TRUE</code> (PHP-mode, experts only).');
    $description .= t('If the PHP-mode is chosen, enter PHP code between %php. Note that executing incorrect PHP-code can severely break your Drupal site.', array('%php' => '<?php ?>'));
  }
  $form['cacheability'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Cacheability settings'),
    '#collapsible'   => TRUE,
    '#collapsed'     => TRUE,
  );
  $form['cacheability']['boost_cacheability_option'] = array(
    '#type'          => 'radios',
    '#title'         => t('Cache specific pages'),
    '#options'       => $options,
    '#default_value' => BOOST_CACHEABILITY_OPTION,
  );
  $form['cacheability']['boost_cacheability_pages'] = array(
    '#type'          => 'textarea',
    '#title'         => t('Pages'),
    '#default_value' => BOOST_CACHEABILITY_PAGES,
    '#description'   => $description,
  );

  // Advanced settings
  $form['advanced'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Advanced settings'),
    '#collapsible'   => TRUE,
    '#collapsed'     => TRUE,
  );
  $form['advanced']['boost_expire_cron'] = array(
    '#type'          => 'radios',
    '#title'         => t('Purge expired cache files on cron runs'),
    '#default_value' => (int)variable_get('boost_expire_cron', TRUE),
    '#options'       => array(0 => t('Disabled'), 1 => t('Enabled')),
    '#description'   => t('Whether to perform a purge of all expired content from the static page cache on cron runs. How often your cron runs works together with the minimum cache lifetime setting, further above, to determine the <strong>effective</strong> minimum lifetime for items in the static page cache. If you don\'t enable this setting, you will need to put in place some other custom mechanism for purging expired files, or else they will never be removed (unless manually cleared from the administrative interface).'),
  );
  $form['advanced']['boost_file_extension'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Cache file extension'),
    '#default_value' => BOOST_FILE_EXTENSION,
    '#size'          => 10,
    '#maxlength'     => 32,
    '#required'      => TRUE,
    '#description'   => t('The file extension to append to the file name of the generated cache files. Note that this setting is of no relevance to any public URLs, and it is strongly recommended to leave this as the default \'.html\' unless you know what you are doing. If you change this, you must also change the URL rewrite rules in your web server configuration (.htaccess for Apache, lighttpd.conf for Lighttpd), or caching will not work.'),
  );
  /*$form['advanced']['boost_fetch_method'] = array( // not needed for now
    '#type'          => 'select',
    '#title'         => t('Page fetch method'),
    '#default_value' => BOOST_FETCH_METHOD,
    '#options'       => array('php' => t('PHP fopen() wrapper'), 'wget' => t('Wget shell command'), 'curl' => t('curl shell command')),
    '#description'   => t('The method used to retrieve the contents of the Drupal pages to be cached. The default should work in most cases.'),
  );*/
  $form['advanced']['boost_pre_process_function'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Pre-process function'),
    '#default_value' => BOOST_PRE_PROCESS_FUNCTION,
    '#maxlength'     => 255,
    '#description'   => t('The name of a PHP function used to pre-process the contents of each page before writing them out to static files. The function is called with the contents of the page passed as a string argument, and its return value is used as the data written out to the disk.'),
  );
  // TODO:
  /*$form['advanced']['boost_post_update_command'] = array( // not needed for now
    '#type'          => 'textfield',
    '#title'         => t('Post-update shell command'),
    '#default_value' => BOOST_POST_UPDATE_COMMAND,
    '#maxlength'     => 255,
    '#description'   => t('If you are synchronizing the generated static cache files to an external server through some means such as SFTP or rsync, you can enter a shell command to be executed following a successful cron-triggered cache update. Note that this is an advanced setting that should normally be left blank.'),
  );*/

  return $form;
}

function boost_admin_settings_validate($form, &$form_state) {
  $form_state['values']['boost_previously'] = variable_get('boost', '');
  extract($form_state['values'], EXTR_SKIP | EXTR_REFS);

  if (!empty($boost)) { // the cache is enabled
    // Ensure the cache directory exists or can be created:
    file_check_directory($boost_file_path, FILE_CREATE_DIRECTORY, 'boost_file_path');
  }

  if (strpos($boost_file_extension, '.') !== 0) {
    form_set_error('boost_file_extension', t('Cache file extension %extension must begin with a period.', array('%extension' => $boost_file_extension)));
  }

  if (!empty($boost_pre_process_function) && !is_callable($boost_pre_process_function)) {
    form_set_error('boost_pre_process_function', t('Pre-process function %function() does not exist.', array('%function' => $boost_pre_process_function)));
  }
}

function boost_admin_settings_submit($form, &$form_state) {
  variable_del('boost_previously');
  extract($form_state['values'], EXTR_SKIP | EXTR_REFS);

  // Forcibly disable Drupal's built-in SQL caching to prevent any conflicts of interest:
  variable_set('cache', CACHE_DISABLED);

  if (empty($boost) && !empty($boost_previously)) { // the cache was previously enabled
    if (boost_cache_clear_all()) {
      drupal_set_message(t('Static page cache cleared.'));
    }
  }
}

function boost_admin_clear_cache_submit($form, &$form_state) {
  boost_cache_clear_all();
  drupal_flush_all_caches();
  drupal_set_message(t('Static page cache and all other caches cleared.'));
}

function boost_admin_site_offline_submit($form, &$form_state) {
  if (!empty($form_state['values']['site_offline'])) {
    if (boost_cache_clear_all()) {
      drupal_set_message(t('Static page cache cleared.'));
    }
  }
}
