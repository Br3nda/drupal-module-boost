<?php
// $Id: boost.admin.inc,v 1.1.2.1.2.3.2.28 2009/06/10 02:54:15 mikeytown2 Exp $

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
function boost_admin_settings($form = array()) {
  // Take over the relevant existing settings
  $description = '<p>'. t("The normal cache mode is suitable for most sites and does not cause any side effects. The aggressive cache mode causes Drupal to skip the loading (boot) and unloading (exit) of enabled modules when serving a cached page. This results in an additional performance boost but can cause unwanted side effects.") .'</p>';

  $problem_modules = array_diff(array_unique(array_merge(module_implements('boot'), module_implements('exit'))), array(0 => 'boost'));
  sort($problem_modules);

  if (count($problem_modules) > 0) {
    $description .= '<p>'. t('<strong class="error">The following enabled modules are incompatible with aggressive mode caching and will probably not function properly: %modules</strong>', array('%modules' => implode(', ', $problem_modules))) .'.</p>';
  }
  else {
    $description .= '<p>'. t('<strong class="ok">Currently, all enabled modules are compatible with the aggressive caching policy.</strong> Please note, if you use aggressive caching and enable new modules, you will need to check this page again to ensure compatibility.') .'</p>';
  }

  // Inject Boost-specific settings
  $form['boost'] = array(
    '#type'          => 'radios',
    '#title'         => t('Boost - Static page cache'),
    '#default_value' => variable_get('boost', CACHE_DISABLED),
    '#options'       => array(CACHE_DISABLED => t('Disabled'), CACHE_NORMAL => t('Enabled'), CACHE_AGGRESSIVE => t('(Not Recommended) Set Boost & Core (if enabled) cache for each page')),
    '#description'   => t('Static page caching is a mechanism that stores dynamically generated web pages as HTML files in a special cache directory located under the Drupal installation directory. By caching a web page in this manner, the web server can serve it out in the fastest possible manner, without invoking PHP or Drupal at all. While this does provide a significant performance and scalability boost, you should note that it could have negative usability side-effects unless your site is targeted at an audience consisting mostly of "anonymous" visitors.'),
    '#weight'        => -10,
  );
  $form['cache'] = array(
    '#type'           => 'radios',
    '#title'          => t('Core - Database page cache'),
    '#default_value'  => variable_get('cache', CACHE_DISABLED),
    '#options'        =>  array(CACHE_DISABLED => t('Disabled'), CACHE_NORMAL => t('Normal (recommended for production sites, no side effects)'), CACHE_AGGRESSIVE => t('Aggressive (experts only, possible side effects)')),
    '#description'    => t("If boost can not or will not cache the page, then the database will try to cache it. If boost can cache the page then it will not appear in the database cache. This is the standard Drupal cache.") . $description,
    '#weight'         => -5,
  );
  $period = drupal_map_assoc(array(0, 60, 180, 300, 600, 900, 1800, 2700, 3600, 10800, 21600, 32400, 43200, 64800, 86400, 2*86400, 3*86400, 4*86400, 5*86400, 6*86400, 7*86400, 14*86400, 21*86400, 28*86400), 'format_interval');
  $period[0] = '<' . t('none') . '>';
  $form['cache_lifetime']['#title'] = t('Default minimum cache lifetime');
  $form['cache_lifetime']['#options'] = $period;

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
    '#title'         => t('Boost cacheability settings'),
    '#collapsible'   => TRUE,
    '#collapsed'     => TRUE,
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
    '#collapsible'   => TRUE,
    '#collapsed'     => TRUE,
  );
  $form['advanced']['boost_file_path'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Cache file path'),
    '#default_value' => BOOST_FILE_PATH,
    '#size'          => 60,
    '#maxlength'     => 255,
    '#required'      => TRUE,
    '#description'   => t('A file system path where the static cache files will be stored. This directory has to exist and be writable by Drupal. The default setting is to store the files in a directory named %default-path under the Drupal installation directory. If you change this, you must also change the URL rewrite rules in your web server configuration (.htaccess for Apache, lighttpd.conf for Lighttpd), or caching will not work.', array('%default-path' => boost_cache_directory(NULL, FALSE))),
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
  $form['advanced']['boost_pre_process_function'] = array(
    '#type'          => 'textfield',
    '#title'         => t('Pre-process function'),
    '#default_value' => BOOST_PRE_PROCESS_FUNCTION,
    '#maxlength'     => 255,
    '#description'   => t('The name of a PHP function used to pre-process the contents of each page before writing them out to static files. The function is called with the contents of the page passed as a string argument, and its return value is used as the data written out to the disk.'),
  );
  // TODO:
  /* $form['advanced']['boost_post_update_command'] = array( // not needed for now
    '#type'          => 'textfield',
    '#title'         => t('Post-update shell command'),
    '#default_value' => BOOST_POST_UPDATE_COMMAND,
    '#maxlength'     => 255,
    '#description'   => t('If you are synchronizing the generated static cache files to an external server through some means such as SFTP or rsync, you can enter a shell command to be executed following a successful cron-triggered cache update. Note that this is an advanced setting that should normally be left blank.'),
  ); */
   $form['advanced']['boost_cache_xml'] = array(
     '#type'          => 'checkbox',
     '#title'         => t('Cache .xml & /feed'),
     '#default_value' => BOOST_CACHE_XML,
     '#description'   => t('Boost will cache .xml and /feed urls as xml data.'),
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
    '#description'   => t('Disable this if you have to set settings for each dir/subdir, due to the way your server opperates (permissions, etc...).'),
  );
  $form['advanced']['boost_halt_on_errors'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Do not cache if error on page'),
    '#default_value' => BOOST_HALT_ON_ERRORS,
    '#description'   => t('Boost will not cache the page if there are PHP errors or drupal messages. Disable this if you want to cache pages even if it might contain errors.'),
  );
  $form['advanced']['boost_multisite_single_db'] = array(
    '#type'          => 'checkbox',
    '#title'         => t('Do not store the cache file path in the database'),
    '#default_value' => BOOST_MULTISITE_SINGLE_DB,
    '#description'   => t('If Drupal is running on a multi-site with a single/shared database, then enabling this will allow for correct multi-site caching.'),
  );

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
  $form['htaccess']['boost_document_root'] = array(
    '#type'          => 'radios',
    '#title'         => t('Document Root'),
    '#default_value' => variable_get('boost_document_root', '%{DOCUMENT_ROOT}'),
    '#options'       => array('%{DOCUMENT_ROOT}' => '%{DOCUMENT_ROOT}', getcwd() => getcwd()),
    '#description'   => t('Default value %{DOCUMENT_ROOT} is highly recommened.'),
  );
  $form['htaccess']['boost_generated'] = array(
    '#type'          => 'textarea',
    '#title'         => t('Generated Rules'),
    '#default_value' => $htaccess,
    '#rows'          => count(explode("\n", $htaccess))+1,
    '#description'   => t("Copy this into your .htaccess file below <pre><tt>  # RewriteBase / </tt></pre> and above <pre><tt>  # Rewrite URLs of the form 'x' to the form 'index.php?q=x'</tt></pre>"),
  );


  return $form;
}

function boost_admin_settings_validate($form, &$form_state) {
  $form_state['values']['boost_previously'] = variable_get('boost', '');
  extract($form_state['values'], EXTR_SKIP | EXTR_REFS);

  if (!empty($boost)) { // the cache is enabled
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

function boost_admin_settings_submit($form, &$form_state) {
  variable_del('boost_previously');
  extract($form_state['values'], EXTR_SKIP | EXTR_REFS);

  if (empty($boost) && !empty($boost_previously)) { // the cache was previously enabled
    if (boost_cache_clear_all()) {
      drupal_set_message(t('Static page cache cleared.'));
    }
  }
  else if (variable_get('preprocess_css', FALSE)==TRUE || variable_get('preprocess_js', FALSE)==TRUE) {
    boost_cache_clear_all();
  }
}

function boost_admin_clear_cache_submit($form, &$form_state) {
  boost_cache_clear_all();
  drupal_flush_all_caches();
  drupal_set_message(t('Static page cache and all other caches cleared.'));
}

function boost_admin_site_offline_submit($form, &$form_state) {
  if (!empty($form_state['values']['site_offline'])) {
    if (BOOST_CLEAR_CACHE_OFFLINE) {
      if (boost_cache_clear_all()) {
        drupal_set_message(t('Static page cache cleared.'));
      }
    }
  }
}

function boost_admin_modules_submit($form, &$form_state) {
  if (variable_get('preprocess_css', FALSE)==TRUE || variable_get('preprocess_js', FALSE)==TRUE) {
    boost_cache_clear_all();
  }
}

function boost_admin_themes_submit($form, &$form_state) {
  if (variable_get('preprocess_css', FALSE)==TRUE || variable_get('preprocess_js', FALSE)==TRUE) {
    boost_cache_clear_all();
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
 *   html document file extention
 */
function boost_admin_generate_htaccess($server_name = '%{SERVER_NAME}', $document_root = '%{DOCUMENT_ROOT}', $cache_dir = 'cache', $gzip_dir = 'gz', $html = '.html', $xml = '.xml') {
Global $base_path;
$drupal_subdir = rtrim($base_path, '/');
$document_root = str_replace("\\", '/', $document_root);
$document_root = str_replace($drupal_subdir, '', $document_root );

  $string = <<<ETO
  ### BOOST START ###
  <FilesMatch "($html.gz|$html)$">
    <IfModule mod_headers.c>
      Header set Expires "Sun, 19 Nov 1978 05:00:00 GMT"
      Header set Cache-Control "no-store, no-cache, must-revalidate, post-check=0, pre-check=0"
    </IfModule>
  </FilesMatch>
  <IfModule mod_mime.c>
    AddCharset utf-8 $html
    AddCharset utf-8 $xml
  </IfModule>
  <FilesMatch "($html.gz)$">
    AddEncoding x-gzip .gz
    ForceType text/html
  </FilesMatch>
  <FilesMatch "($xml.gz)$">
    AddEncoding x-gzip .gz
    ForceType text/xml
  </FilesMatch>

  # Skip boost IF not get request OR uri has wrong dir OR cookie is set
  RewriteCond %{REQUEST_METHOD} !^GET$ [OR]
  RewriteCond %{REQUEST_URI} (^$drupal_subdir(admin|cache|misc|modules|sites|system|themes))|(/(comment/reply|user|user/(login|password|register))$) [OR]
  RewriteCond %{HTTP_COOKIE} DRUPAL_UID
  RewriteRule .* - [S=4]

  # GZIP
  RewriteCond %{HTTP:Accept-encoding} gzip
  RewriteCond $document_root$drupal_subdir/$cache_dir/$gzip_dir/$server_name%{REQUEST_URI}_%{QUERY_STRING}$html.gz -s
  RewriteRule .* $cache_dir/$gzip_dir/$server_name%{REQUEST_URI}_%{QUERY_STRING}$html.gz [L]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$gzip_dir/$server_name%{REQUEST_URI}_%{QUERY_STRING}$xml.gz -s
  RewriteRule .* $cache_dir/$gzip_dir/$server_name%{REQUEST_URI}_%{QUERY_STRING}$xml.gz [L,T=text/xml]

  # NORMAL
  RewriteCond $document_root$drupal_subdir/$cache_dir/$server_name%{REQUEST_URI}_%{QUERY_STRING}$html -s
  RewriteRule .* $cache_dir/$server_name%{REQUEST_URI}_%{QUERY_STRING}$html [L]
  RewriteCond $document_root$drupal_subdir/$cache_dir/$server_name%{REQUEST_URI}_%{QUERY_STRING}$xml -s
  RewriteRule .* $cache_dir/$server_name%{REQUEST_URI}_%{QUERY_STRING}$xml [L,T=text/xml]
  ### BOOST END ###
ETO;

return $string;
}
