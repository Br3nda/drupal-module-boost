<?php
// $Id: boost.admin.inc,v 1.1.2.1.2.3.2.56 2009/08/21 02:37:23 mikeytown2 Exp $

/**
 * @file
 * All the code for the Boost module's administrative interface.
 */

//////////////////////////////////////////////////////////////////////////////
// Boost settings form

/**
 * Performs alterations to the system settings form before it is rendered.
 *
 * @see hook_form_alter()
 *   in boost.module
 */
function boost_admin_performance_page($form = array()) {
  // Clear old settings
  unset($form['cache']);
  unset($form['cache_lifetime']);

  // Set text
  $description_core = '<p>'. t("The normal cache mode is suitable for most sites and does not cause any side effects. The aggressive cache mode causes Drupal to skip the loading (boot) and unloading (exit) of enabled modules when serving a cached page. This results in an additional performance boost but can cause unwanted side effects.") .'</p>';
  $problem_modules = array_unique(array_merge(module_implements('boot'), module_implements('exit')));
  sort($problem_modules);
  if (count($problem_modules) > 0) {
    $description_core .= '<p>'. t('<strong class="error">The following enabled modules are incompatible with aggressive mode caching and will probably not function properly: %modules</strong>', array('%modules' => implode(', ', $problem_modules))) .'.</p>';
  }
  else {
    $description_core .= '<p>'. t('<strong class="ok">Currently, all enabled modules are compatible with the aggressive caching policy.</strong> Please note, if you use aggressive caching and enable new modules, you will need to check this page again to ensure compatibility.') .'</p>';
  }
  $period = drupal_map_assoc(array(0, 60, 180, 300, 600, 900, 1800, 2700, 3600, 10800, 21600, 32400, 43200, 64800, 86400, 2*86400, 3*86400, 4*86400, 5*86400, 6*86400, 604800, 2*604800, 3*604800, 4*604800, 8*604800, 16*604800, 52*604800), 'format_interval');
  $period[0] = '<' . t('none') . '>';

  // Take over the relevant existing settings
  $form['page_compression']['#title'] = t('Gzip page compression (Boost & Core)');

  $form['boost'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Boost File Cache'),
  );
  // Inject Boost-specific settings
  $form['boost']['boost_enabled'] = array(
    '#type'          => 'radios',
    '#title'         => t('Boost - Static page cache'),
    '#default_value' => variable_get('boost_enabled', CACHE_DISABLED),
    '#options'       => array(CACHE_DISABLED => t('Disabled'), CACHE_NORMAL => t('Enabled'), CACHE_AGGRESSIVE => t('(Not Recommended) Set Boost & Core (if enabled) cache for each page')),
    '#description'   => t('Static page caching is a mechanism that stores dynamically generated web pages as HTML files in a special cache directory located under the Drupal installation directory. By caching a web page in this manner, the web server can serve it out in the fastest possible manner, without invoking PHP or Drupal at all. While this does provide a significant performance and scalability boost, you should note that it could have negative usability side-effects unless your site is targeted at an audience consisting mostly of "anonymous" visitors.'),
  );
  if (BOOST_CACHE_HTML) {
    $form['boost']['boost_cache_lifetime'] = array(
      '#type' => 'select',
      '#title' => t('Boost - HTML - Default minimum cache lifetime'),
      '#default_value' => BOOST_CACHE_LIFETIME,
      '#options' => $period,
      '#description' => t('The minimum cache lifetime is the minimum amount of time that will elapse before the cache is emptied. Cache lifetime gets checked on cron runs.'),
    );
    $form['boost']['boost_push_html'] = array(
      '#type' => 'checkbox',
      '#title' => t('Preemptive Cache HTML'),
      '#default_value' => BOOST_PUSH_HTML,
      '#description' => t('Crawl Site after cron runs, so the cache is primed.'),
    );
  }
  if (BOOST_CACHE_XML) {
    $form['boost']['boost_cache_xml_lifetime'] = array(
      '#type' => 'select',
      '#title' => t('Boost - XML - Default minimum cache lifetime'),
      '#default_value' => BOOST_CACHE_XML_LIFETIME,
      '#options' => $period,
      '#description' => t('The minimum cache lifetime is the minimum amount of time that will elapse before the cache is emptied. Cache lifetime gets checked on cron runs.'),
    );
    $form['boost']['boost_push_xml'] = array(
      '#type' => 'checkbox',
      '#title' => t('Preemptive Cache XML'),
      '#default_value' => BOOST_PUSH_XML,
      '#description' => t('Crawl Site after cron runs, so the cache is primed.'),
    );
  }
  if (BOOST_CACHE_JSON) {
    $form['boost']['boost_cache_json_lifetime'] = array(
      '#type' => 'select',
      '#title' => t('Boost - JSON - Default minimum cache lifetime'),
      '#default_value' => BOOST_CACHE_JSON_LIFETIME,
      '#options' => $period,
      '#description' => t('The minimum cache lifetime is the minimum amount of time that will elapse before the cache is emptied. Cache lifetime gets checked on cron runs.'),
    );
    $form['boost']['boost_push_json'] = array(
      '#type' => 'checkbox',
      '#title' => t('Preemptive Cache AJAX/JSON'),
      '#default_value' => BOOST_PUSH_JSON,
      '#description' => t('Crawl Site after cron runs, so the cache is primed.'),
    );
  }
  $form['boost']['boost_clear'] = array(
    '#type' => 'submit',
    '#value' => t('Clear ALL Boost cached data: !count pages', array('!count' => boost_count_db(0))),
    '#submit' => array('boost_clear_cache_submit'),
  );
  $form['boost']['boost_clear_expired'] = array(
    '#type' => 'submit',
    '#value' => t('Clear Boost expired data: !count pages', array('!count' => boost_count_db(1))),
    '#submit' => array('boost_clear_expired_cache_submit'),
  );

  // Alter core page cache settings.
  $form['core'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Core Database Cache'),
  );
  $form['core']['cache'] = array(
    '#type'           => 'radios',
    '#title'          => t('Core - Database page cache'),
    '#default_value'  => variable_get('cache', CACHE_DISABLED),
    '#options'        =>  array(CACHE_DISABLED => t('Disabled'), CACHE_NORMAL => t('Normal (recommended for production sites, no side effects)'), CACHE_AGGRESSIVE => t('Aggressive (experts only, possible side effects)')),
    '#description'    => t("If boost can not or will not cache the page, then the database will try to cache it. If boost can cache the page then it will not appear in the database cache. This is the standard Drupal cache.") . $description_core,
  );
  $form['core']['cache_lifetime'] = array(
    '#type' => 'select',
    '#title' => t('Core - Default minimum cache lifetime'),
    '#default_value' => variable_get('cache_lifetime', 0),
    '#options' => $period,
    '#description' => t('On high-traffic sites, it may be necessary to enforce a minimum cache lifetime. The minimum cache lifetime is the minimum amount of time that will elapse before the cache is emptied and recreated, and is applied to both page and block caches. A larger minimum cache lifetime offers better performance, but users will not see new content for a longer period of time.')
  );
  $form['core']['boost_core_clear'] = array(
    '#type' => 'submit',
    '#value' => t('Clear core page cached data: !count pages', array('!count' => boost_count_core_db())),
    '#submit' => array('boost_clear_core_page_cache_submit'),
  );

  return $form;
}

/**
 * Process system_themes_form form submissions.
 */
function boost_admin_performance_page_submit($form, &$form_state) {
  variable_del('boost_previously');
  extract($form_state['values'], EXTR_SKIP | EXTR_REFS);

  if (empty($boost_enabled) && !empty($boost_previously)) { // the cache was previously enabled
    variable_set('boost_ignore_flush', 0);
    if (boost_cache_clear_all()) {
      drupal_set_message(t('Boost: Static page cache cleared.'));
    }
  }
  else if ($boost_ignore_flush == 0 && (variable_get('preprocess_css', FALSE)==TRUE || variable_get('preprocess_js', FALSE)==TRUE)) {
    if (boost_cache_clear_all()) {
      drupal_set_message(t('Boost: Static page cache cleared.'), 'warning');
    }
  }
}

/**
 * Submit callback; clear system caches.
 */
function boost_admin_clear_cache_submit($form, &$form_state) {
  drupal_flush_all_caches();
  if (boost_cache_clear_all()) {
    drupal_set_message(t('Static page cache (boost) and all other caches cleared.'));
  }
  else {
    drupal_set_message(
      t('Boost: Static page cache NOT cleared. To clear Boosts cache set "Ignore cache flushing:" to \'Disabled\' in the <a href="@link">boost advanced settings</a> & try again.', array('@link' => url('admin/settings/performance/boost', array('fragment' => 'edit-boost-ignore-flush-0-wrapper')))), 'warning');
    drupal_set_message(t('All other caches cleared.'));
  }
}

/**
 * Form builder; Displays Boost's configuration page.
 *
 * @see system_settings_form()
 *
 * TODO: Cacheability settings use core code. Look at Blocks.
 *  http://drupal.org/node/337756
 */
function boost_admin_boost_performance_page() {
  Global $base_path;

  // Cacheability settings
  $options = array(t('Cache every page except the listed pages.'), t('Cache only the listed pages.'));
  $description = t("Enter one page per line as Drupal paths. The '*' character is a wild-card. Example paths are '%blog' for the blog page and %blog-wild-card for every personal blog. %front is the front page.", array('%blog' => 'blog', '%blog-wild-card' => 'blog/*', '%front' => '<front>'));
  if (user_access('use PHP for block visibility')) {
    $options[] = t('Cache pages for which the following PHP code returns <code>TRUE</code> (PHP-mode, experts only).');
    $description .= t('If the PHP-mode is chosen, enter PHP code between %php. Note that executing incorrect PHP-code can severely break your Drupal site.', array('%php' => '<?php ?>'));
  }
  $form['cacheability'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Boost cacheability settings'),
  );
  $form['cacheability']['boost_cacheability_option'] = array(
    '#type'          => 'radios',
    '#title'         => t('Statically cache specific pages'),
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
    '#title'         => t('Boost advanced settings'),
  );
  $form['advanced']['boost_multisite_single_db'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Do not store the cache file path in the database'),
    '#default_value' => BOOST_MULTISITE_SINGLE_DB,
    '#description'   => t('Enabling will allow for correct multi-site caching, in cases where different content is served from the same Drupal installation, based on domain. Examples: Multi-site with a single/shared database, site translation detection based on domain, and the contributed "Domain Access" module.'),
  );
  if (!BOOST_MULTISITE_SINGLE_DB) {
    $form['advanced']['boost_file_path'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Cache file path'),
      '#default_value' => BOOST_FILE_PATH,
      '#size'          => 60,
      '#maxlength'     => 255,
      '#required'      => TRUE,
      '#description'   => t('A file system path where the static cache files will be stored. This directory has to exist and be writable by Drupal. The default setting is to store the files in a directory named %default-path under the Drupal installation directory. If you change this, you must also change the URL rewrite rules in your web server configuration (.htaccess for Apache, lighttpd.conf for Lighttpd), or caching will not work.', array('%default-path' => boost_cache_directory(NULL, FALSE))),
    );
  }
  $form['advanced']['boost_expire_cron'] = array(
    '#type'          => 'radios',
    '#title'         => t('Purge expired cache files on cron runs'),
    '#default_value' => (int)variable_get('boost_expire_cron', TRUE),
    '#options'       => array(0 => t('Disabled'), 1 => t('Enabled')),
    '#description'   => t('Whether to perform a purge of all expired content from the static page cache on cron runs. How often your cron runs works together with the minimum cache lifetime setting, further above, to determine the <strong>effective</strong> minimum lifetime for items in the static page cache. If you don\'t enable this setting, you will need to put in place some other custom mechanism for purging expired files, or else they will never be removed (unless manually cleared from the administrative interface).'),
  );
  $form['advanced']['boost_check_before_cron_expire'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Expire content on cron only if the sites content has changed.'),
    '#default_value' => BOOST_CHECK_BEFORE_CRON_EXPIRE,
    '#description'   => t('Check timestamps in the database for any changes to the site. If theres been a change, flush the expired content.'),
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
  $form['advanced']['boost_pre_process_function'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Pre-process function'),
    '#default_value' => BOOST_PRE_PROCESS_FUNCTION,
    '#maxlength'     => 255,
    '#description'   => t('The name of a PHP function used to pre-process the contents of each page before writing them out to static files. The function is called with the contents of the page passed as a string argument, and its return value is used as the data written out to the disk.'),
  );
    $form['advanced']['boost_cache_query'] = array(
      '#type'          => 'checkbox',
      '#title'         => t('Cache pages that contain URL Variables'),
      '#default_value' => BOOST_CACHE_QUERY,
      '#description'   => t('Boost will cache pages that end with ?page=1 among others.'),
    );
   $form['advanced']['boost_cache_html'] = array(
     '#type'          => 'checkbox',
     '#title'         => t('Cache html documents/pages'),
     '#default_value' => BOOST_CACHE_HTML,
     '#description'   => t('Boost will cache most drupal pages.'),
   );
   $form['advanced']['boost_cache_xml'] = array(
     '#type'          => 'checkbox',
     '#title'         => t('Cache .xml & /feed'),
     '#default_value' => BOOST_CACHE_XML,
     '#description'   => t('Boost will cache .xml and /feed urls as xml data.'),
   );
   $form['advanced']['boost_cache_css'] = array(
     '#type'          => 'checkbox',
     '#title'         => t('Cache .css'),
     '#default_value' => BOOST_CACHE_CSS,
     '#description'   => t('Boost will cache CSS files.'),
   );
   $form['advanced']['boost_cache_js'] = array(
     '#type'          => 'checkbox',
     '#title'         => t('Cache .js'),
     '#default_value' => BOOST_CACHE_JS,
     '#description'   => t('Boost will cache javascript files.'),
   );
   $form['advanced']['boost_cache_json'] = array(
     '#type'          => 'checkbox',
     '#title'         => t('Cache ajax/json'),
     '#default_value' => BOOST_CACHE_JSON,
     '#description'   => t('Boost will cache ajax/json responses.'),
   );
  $form['advanced']['boost_only_ascii_path'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Only allow ASCII characters in path'),
    '#default_value' => BOOST_ONLY_ASCII_PATH,
    '#description'   => t('Only allowing ACSII characters is a safe way to cache pages. It severely limits i18n support so this can be turned off. Fair warning, disabling this may cause "page not found" errors depending on your url structure (spaces are bad, ect...). If you follow RFC 3986 you should be ok.'),
  );
  $form['advanced']['boost_clear_cache_offline'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Clear Boosts cache when site goes offline'),
    '#default_value' => BOOST_CLEAR_CACHE_OFFLINE,
    '#description'   => t('Under site maintenance when the status is set to offline, boost clears its cache. If you do not want this to happen, clear this checkbox. Pages that are not cached will still send out a Site off-line message, so be smart if turning this off.'),
  );
  $form['advanced']['boost_flush_dir'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Flush directories from cache if they are empty'),
    '#default_value' => BOOST_FLUSH_DIR,
    '#description'   => t('Disable this if you have to set settings for each dir/subdir, due to the way your server operates (permissions, etc...).'),
  );
  $form['advanced']['boost_flush_node_terms'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Flush all cached terms pages associated with a node on insert/update/delete'),
    '#default_value' => BOOST_FLUSH_NODE_TERMS,
    '#description'   => t('Works with view\'s taxonomy/term/% path as well as core.'),
  );
  $form['advanced']['boost_overwrite_file'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Overwrite the cached file if it already exits'),
    '#default_value' => BOOST_OVERWRITE_FILE,
    '#description'   => t('This is useful if crawling a site before it goes live.'),
  );
  $form['advanced']['boost_halt_on_errors'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Do not cache if error on page'),
    '#default_value' => BOOST_HALT_ON_ERRORS,
    '#description'   => t('Boost will not cache the page if there are PHP errors or drupal messages. Disable this if you want to cache pages even if it might contain errors.'),
  );
  $form['advanced']['boost_disable_clean_url'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Turn off clean url\'s for logged in users'),
    '#default_value' => BOOST_DISABLE_CLEAN_URL,
    '#description'   => t('Drupal will output non clean url\'s for non anonymous users. This allows for the browser to cache the page and still have logging in work. This is more on the extreme side of tweaks.'),
  );
  $form['advanced']['boost_permissions_file'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Files: Enter in a 4 digit number (octal) that will be used by !link. Example 0664', array('!link' => l(t('chmod()'), 'http://php.net/chmod'))),
    '#default_value' => BOOST_PERMISSIONS_FILE,
    '#description'   => t('Sometimes because of funky servers you need it use a different file mode then the default.'),
  );
  $form['advanced']['boost_permissions_dir'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Directories: Enter in a 4 digit number (octal) that will be used by !link. Example 0775', array('!link' => l(t('chmod()'), 'http://php.net/chmod'))),
    '#default_value' => BOOST_PERMISSIONS_DIR,
    '#description'   => t('Sometimes because of funky servers you need it use a different file mode then the default.'),
  );
  $form['advanced']['boost_ignore_flush'] = array(
    '#type'          => 'radios',
    '#title'         => t('Ignore cache flushing'),
    '#default_value' => BOOST_IGNORE_FLUSH,
    '#options'       => array(0 => t('Disabled'), 1 => t('Only Ignore Complete Flushes'), 2 => t('Ignore Flushes & Expiration'), 3 => t('Ignore All Delete Commands (Not Recommended)')),
    '#description'   => t('Enable to put your site into a static state. Recommend turning on CSS & JS caching if enabled.'),
  );
  $form['advanced']['boost_verbose'] = array(
    '#type'          => 'select',
    '#title'         => t('Verbose Setting'),
    '#default_value' => BOOST_VERBOSE,
    '#options'       => array(
      1 => t('1 Show almost no errors'),
      3 => t('3'),
      5 => t('5 Show all'),
    ),
  );

  // Crawler
  $form['crawler'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Boost crawler'),
  );
  $form['crawler']['boost_crawl_on_cron'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Preemptive Cache: Crawl URL\'s with the boost block setting to yes'),
    '#default_value' => BOOST_CRAWL_ON_CRON,
    '#description'   => t('Touch some select URL\'s so they are cached before anyone accesses them.'),
  );
  $form['crawler']['boost_crawler_throttle'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Crawler Throttle'),
    '#default_value' => BOOST_CRAWLER_THROTTLE,
    '#description'   => t('Wait X micro seconds inbetween hitting each url. 1000000 is 1 second.'),
  );
  $form['crawler']['boost_crawler_batch_size'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Crawler Batch Size'),
    '#default_value' => BOOST_CRAWLER_BATCH_SIZE,
    '#description'   => t('Number of URL\'s each thread grabs per database opperation.'),
  );
  $form['crawler']['boost_crawler_threads'] = array(
    '#type'          => 'select',
    '#title'         => t('Number Of Threads'),
    '#default_value' => BOOST_CRAWLER_THREADS,
    '#options' => array(
      1 => 1,
      2 => 2,
      3 => 3,
      4 => 4,
      5 => 5,
      6 => 6,
      7 => 7,
      8 => 8,
    ),
    '#description'   => t('Be careful when choosing more then 2 threads.'),
  );
  $total = boost_crawler_total_count();
  //variable_get
  $number_done = min($total, unserialize(db_result(db_query("SELECT value FROM {variable} WHERE name = 'boost_crawler_position'"))));
  if (variable_get('boost_crawler_number_of_threads', 0) > 0) {
    $form['crawler']['live'] = array(
      '#type'          => 'fieldset',
      '#title'         => t('Boost crawler - Live info'),
    );
    $crawl_rate = round((BOOST_TIME - variable_get('boost_crawler_start_time', BOOST_TIME))/$number_done, 2);
    $form['crawler']['live']['boost_crawler_rate'] = array(
      '#type'          => 'textfield',
      '#title'         => t('Crawl Rate - Seconds Per Node (highly inaccurate)'),
      '#default_value' => $crawl_rate,
      '#disabled'      => TRUE,
    );
    $form['crawler']['live']['boost_stop_crawler'] = array(
      '#type' => 'submit',
      '#value' => t('Stop Crawler. !count URL\'s left.', array('!count' => $total - $number_done)),
      '#submit' => array('boost_stop_crawler_submit'),
    );
  }

  // Apache .htaccess settings generation
  $htaccess = boost_admin_generate_htaccess(variable_get('boost_server_name_http_host', '%{SERVER_NAME}'), variable_get('boost_document_root', '%{DOCUMENT_ROOT}'), 'cache', 'gz', variable_get('boost_file_extension', '.html'));
  $form['htaccess'] = array(
    '#type'          => 'fieldset',
    '#title'         => t('Boost Apache .htaccess settings generation'),
    '#collapsible'   => TRUE,
    '#collapsed'     => TRUE,
    '#description'   => t('!link', array('!link' => l(t('Explanation of .htaccess variables'), 'http://www.askapache.com/htaccess/mod_rewrite-variables-cheatsheet.html'))),
  );
  $form['htaccess']['boost_server_name_http_host'] = array(
    '#type'          => 'radios',
    '#title'         => t('Servers URL or Name'),
    '#default_value' => variable_get('boost_server_name_http_host', '%{SERVER_NAME}'),
    '#options'       => array('%{SERVER_NAME}' => '%{SERVER_NAME}', '%{HTTP_HOST}' => '%{HTTP_HOST}', $_SERVER['SERVER_NAME'] => $_SERVER['SERVER_NAME'], $_SERVER['HTTP_HOST'] => $_SERVER['HTTP_HOST'], ),
    '#description'   => t('Best to leave these as %{}, only try the last option(s) if boost is still not working.'),
  );

  // Set DOCUMENT_ROOT
  $drupal_subdir = rtrim($base_path, '/');
  $document_root = str_replace("\\", '/', getcwd()); // fix windows dir slashes
  $document_root = str_replace($drupal_subdir, '', $document_root); // remove subdir
  $options = array('%{DOCUMENT_ROOT}' => '%{DOCUMENT_ROOT}', $document_root => $document_root); // initial options
  $rejects = array('SCRIPT_FILENAME', 'DOCUMENT_ROOT'); // values to ignore
  $output = boost_array_find($document_root, $_SERVER, $rejects); //search for values that match getcwd
  if (!empty($output)) {
    foreach ($output as $key=>$value) {
      $temp = '%{ENV:' . $key . '}';
      $options[$temp] = $temp; // adding values to options
      if ($value == $document_root) {
        $best = $temp; // set best since it's a match
      }
    }
  }
  if ($_SERVER['DOCUMENT_ROOT'] == $document_root) {
    $best = '%{DOCUMENT_ROOT}';
  }
  $description = t("Value of $best is recommended for this server.");
  if (!isset($best)) {
    $best = $document_root;
    $description .= t(' Please open an boost issue on drupal.org, since apache and php are not configured correctly.');
  }
  $form['htaccess']['boost_document_root'] = array(
    '#type'          => 'radios',
    '#title'         => t('Document Root'),
    '#default_value' => variable_get('boost_document_root', '%{DOCUMENT_ROOT}'),
    '#options'       => $options,
    '#description'   => $description,
  );

  $form['htaccess']['boost_generated'] = array(
    '#type'          => 'textarea',
    '#title'         => t('Generated Rules'),
    '#default_value' => $htaccess,
    '#rows'          => count(explode("\n", $htaccess))+1,
    '#wysiwyg'       => FALSE,
    '#description'   => t("Copy this into your .htaccess file below <pre><tt>  # RewriteBase / </tt></pre> and above <pre><tt>  # Rewrite URLs of the form 'x' to the form 'index.php?q=x'</tt></pre>"),
  );


  $form['clear'] = array(
    '#type' => 'fieldset',
    '#title' => t('Clear Boost\'s Database'),
    '#description' => t('Warning: This gives you a clean slate for the boost database, use with caution.'),
  );
  $form['clear']['boost_reset'] = array(
    '#type' => 'submit',
    '#value' => t('Reset boost database: !records records', array('!records' => boost_count_db(2))),
    '#submit' => array('boost_reset_database_submit'),
  );

  $form['#validate'][] = 'boost_admin_boost_performance_page_validate';
  return system_settings_form($form);
}

/**
 * validate system_themes_form form submissions.
 */
function boost_admin_boost_performance_page_validate($form, &$form_state) {
  $form_state['values']['boost_previously'] = variable_get('boost_enabled', '');
  extract($form_state['values'], EXTR_SKIP | EXTR_REFS);

  if (!empty($boost_enabled)) { // the cache is enabled
    // Ensure the cache directory exists or can be created:
    _boost_mkdir_p($boost_file_path);
    file_check_directory($boost_file_path, FILE_CREATE_DIRECTORY, 'boost_file_path');
  }

  if (strpos($boost_file_extension, '.') !== 0) {
    form_set_error('boost_file_extension', t('Cache file extension %extension must begin with a period.', array('%extension' => $boost_file_extension)));
  }

  if (!empty($boost_pre_process_function) && !is_callable($boost_pre_process_function)) {
    form_set_error('boost_pre_process_function', t('Pre-process function %function() does not exist.', array('%function' => $boost_pre_process_function)));
  }
}

function boost_admin_site_offline_submit($form, &$form_state) {
  if (!empty($form_state['values']['site_offline'])) {
    if (BOOST_CLEAR_CACHE_OFFLINE) {
      if (boost_cache_clear_all()) {
        drupal_set_message(t('Boost: Static page cache cleared.'), 'warning');
      }
    }
  }
}

function boost_admin_modules_submit($form, &$form_state) {
  if ((variable_get('preprocess_css', FALSE)==TRUE || variable_get('preprocess_js', FALSE)==TRUE)) {
    if (boost_cache_clear_all()) {
      drupal_set_message(t('Boost: Static page cache cleared.'), 'warning');
    }
  }
}

function boost_admin_themes_submit($form, &$form_state) {
  if ((variable_get('preprocess_css', FALSE)==TRUE || variable_get('preprocess_js', FALSE)==TRUE)) {
    if (boost_cache_clear_all()) {
      drupal_set_message(t('Boost: Static page cache cleared.'), 'warning');
    }
  }
}

/**
 * Generate htaccess code.
 *
 * http://www.askapache.com/htaccess/mod_rewrite-variables-cheatsheet.html
 * @param $server_name
 *   %{SERVER_NAME} [OR] %{HTTP_HOST} [OR] www.example.com
 * @param $document_root
 *   %{DOCUMENT_ROOT} [OR] getcwd() [OR] path to webroot from ~/
 * @param $cache_dir
 *   cache dir
 * @param $gzip_dir
 *   gz dir
 * @param $html
 *   html document file extension
 */
function boost_admin_generate_htaccess($server_name = '%{SERVER_NAME}', $document_root = '%{DOCUMENT_ROOT}', $cache_dir = 'cache', $gzip_dir = 'gz', $html = '.html', $xml = '.xml') {
  Global $base_path;
  $drupal_subdir = rtrim($base_path, '/');
  $html = '\\' . $html;
  $xml = '\\' . $xml;
  $css = '\\.css';
  $js = '\\.js';
  $gz = '\\.gz';
  $char = BOOST_CHAR;

  $string = <<<ETO
  ### BOOST START ###
  <FilesMatch "($html$gz|$html)$">
    <IfModule mod_headers.c>
      Header set Expires "Sun, 19 Nov 1978 05:00:00 GMT"
      Header set Cache-Control "no-store, no-cache, must-revalidate, post-check=0, pre-check=0"
    </IfModule>
  </FilesMatch>
  <IfModule mod_mime.c>
    AddCharset utf-8 .html
    AddCharset utf-8 .xml
  </IfModule>
  AddEncoding x-gzip .gz
  <FilesMatch "$html$gz$">
    ForceType text/html
  </FilesMatch>
  <FilesMatch "$xml$gz$">
    ForceType text/xml
  </FilesMatch>
  <FilesMatch "$css$gz$">
    ForceType text/css
  </FilesMatch>
  <FilesMatch "$js$gz$">
    ForceType text/javascript
  </FilesMatch>

  # GZIP - Cached css & js files
  RewriteCond %{HTTP:Accept-encoding} !gzip
  RewriteRule .* - [S=2]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char$char$css$gz -s
  RewriteRule .* $cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char$char$css$gz [L,QSA,T=text/css]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char$char$js$gz -s
  RewriteRule .* $cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char$char$js$gz [L,QSA,T=text/javascript]

  # NORMAL - Cached css & js files
  RewriteCond $document_root$drupal_subdir/$cache_dir/$server_name%{REQUEST_URI}$char$char$css -s
  RewriteRule .* $cache_dir/$server_name%{REQUEST_URI}$char$char$css [L,QSA,T=text/css]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$server_name%{REQUEST_URI}$char$char$js -s
  RewriteRule .* $cache_dir/$server_name%{REQUEST_URI}$char$char$js [L,QSA,T=text/javascript]

  # Caching for anonymous users
  # Skip boost IF not get request OR uri has wrong dir OR cookie is set OR https request
  RewriteCond %{REQUEST_METHOD} !^GET$ [OR]
  RewriteCond %{REQUEST_URI} (^$drupal_subdir(admin|cache|misc|modules|sites|system|themes|node/add))|(/(comment/reply|edit|user|user/(login|password|register))$) [OR]
  RewriteCond %{HTTP_COOKIE} DRUPAL_UID [OR]
  RewriteCond %{HTTPS} on
  RewriteRule .* - [S=7]

  # GZIP
  RewriteCond %{HTTP:Accept-encoding} !gzip
  RewriteRule .* - [S=3]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$html$gz -s
  RewriteRule .* $cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$html$gz [L,T=text/html]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$xml$gz -s
  RewriteRule .* $cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$xml$gz [L,T=text/xml]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$js$gz -s
  RewriteRule .* $cache_dir/$gzip_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$js$gz [L,T=text/javascript]

  # NORMAL
  RewriteCond $document_root$drupal_subdir/$cache_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$html -s
  RewriteRule .* $cache_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$html [L,T=text/html]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$xml -s
  RewriteRule .* $cache_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$xml [L,T=text/xml]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$js -s
  RewriteRule .* $cache_dir/$server_name%{REQUEST_URI}$char%{QUERY_STRING}$js [L,T=text/javascript]

  ### BOOST END ###
ETO;

return $string;
}

/**
 * Counts the number of pages in the static cache.
 */
function boost_count_db($all = 0) {
  if ($all == 0) {
    return db_result(db_query("SELECT COUNT(*) FROM {boost_cache} WHERE expire <> 0 and base_dir = '%s'", BOOST_FILE_PATH));
  }
  elseif ($all == 1) {
    return db_result(db_query("SELECT COUNT(*) FROM {boost_cache} WHERE expire BETWEEN 1 AND %d AND base_dir ='%s'", BOOST_TIME, BOOST_FILE_PATH));
  }
  elseif ($all == 2) {
    return db_result(db_query("SELECT COUNT(*) FROM {boost_cache}"));
  }
}

/**
 * Counts the number of pages in the core cache.
 */
function boost_count_core_db($all = FALSE) {
  return db_result(db_query("SELECT COUNT(*) FROM {cache_page}"));
}

/**
 * Flushes boost page cache
 */
function boost_clear_cache_submit () {
  if (boost_cache_clear_all()) {
    drupal_set_message(t('Boost: Static page cache cleared.'));
  }
  else {
    drupal_set_message(t('Boost: Set "Ignore cache flushing:" to \'Disabled\' in the <a href="!link">boost advanced settings</a> & try again.', array('!link' => url('admin/settings/performance/boost', array('fragment' => 'edit-boost-ignore-flush-0-wrapper')))), 'warning');
  }
}


/**
 * Flushes all expired pages from database
 */
function boost_clear_expired_cache_submit () {
  if (boost_cache_db_expire()) {
    drupal_set_message(t('Boost: Expired stale files from static page cache.'));
  }
  else {
   drupal_set_message(t('Boost: Set "Ignore cache flushing:" to \'Disabled\' OR \'Only Ignore Complete Flushes\' in the <a href="@link">boost advanced settings</a> & try again.', array('@link' => url('admin/settings/performance/boost', array('fragment' => 'edit-boost-ignore-flush-0-wrapper')))), 'warning');
  }
}

/**
 * Flushes core page cache
 */
function boost_clear_core_page_cache_submit () {
  cache_clear_all('*', 'cache_page', TRUE);
  drupal_set_message(t('Core page cache cleared.'));
}

/**
 * Resets boost database & cache
 */
function boost_reset_database_submit () {
  if (boost_cache_clear_all()) {
    db_query("TRUNCATE {boost_cache}");
    db_query("TRUNCATE {boost_cache_settings}");
    drupal_set_message(t('Boost: Static page cache & database cleared.'));
  }
  else {
    drupal_set_message(t('Boost: Set "Ignore cache flushing:" to \'Disabled\' in the <a href="!link">boost advanced settings</a> & try again.', array('!link' => url('admin/settings/performance/boost', array('fragment' => 'edit-boost-ignore-flush-0-wrapper')))), 'warning');
  }
}

/**
 * Stop Crawler
 */
function boost_stop_crawler_submit() {
  variable_set('boost_crawler_stopped', TRUE);
  variable_set('boost_crawler_number_of_threads', 0);
}

/**
 * Returns all key/values in array that are equal.
 *
 * @param $needle
 *  What your searching for
 * @param $haystack
 *  Array of values
 * @param $a_not
 *  Optional array of key names to exclude
 */
function boost_array_find($needle, $haystack, $a_not = array()) {
  $out = array();
  foreach($haystack as $key=>$value) {
    if ($value == $needle) {
      $good = TRUE;
      foreach($a_not as $not) {
        if (strpos($key, $not) !== FALSE) {
          $good = FALSE;
        }
      }
      if ($good) {
        $out[$key] = $value;
      }
    }
  }
  return $out;
}
