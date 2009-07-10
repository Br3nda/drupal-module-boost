<?php
// $Id: boost_stats.php,v 1.1.2.4 2009/07/10 00:19:49 mikeytown2 Exp $

if (isset($_GET['js']) && $_GET['js']==0) {
  // stats not called via JS, send image out & close connection.
  boost_stats_async_image();
}

// Exit script if nothing was passed to it.
if (   !isset($_GET['nid'])
    && !isset($_GET['title'])
    && !isset($_GET['q'])
    && !isset($_GET['referer'])
    ) {
  exit;
}

// connect to db & set variables.
boost_stats_init();

// Set node counter.
if ($count_views && isset($nid) && is_numeric($nid)) {
  boost_stats_update_node_counter();
}

// Set access log.
if ($access_log && isset($title) && isset($q)) {
  boost_stats_add_access_log();
}

// Send stats block html.
if ($stats) {
  boost_stats_output_stats_block();
}

// end of script, exit.
exit;

Function boost_stats_async_image() {
  // Script should take under 1mb of memory to work.
  // Prime php for background operations
  ob_end_clean();
  header("Connection: close");
  ignore_user_abort();

  // Output of 1 pixel transparent gif
  ob_start();
  header("Content-type: image/gif");
  header("Expires: Wed, 11 Nov 1998 11:11:11 GMT");
  header("Cache-Control: no-cache");
  header("Cache-Control: must-revalidate");
  header("Content-Length: 43");
  header("Connection: close");
  printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",71,73,70,56,57,97,1,0,1,0,128,255,0,192,192,192,0,0,0,33,249,4,1,0,0,0,0,44,0,0,0,0,1,0,1,0,0,2,2,68,1,0,59);
  ob_end_flush();
  flush();

  // Image returned and connection closed.
  // Do background processing. Time taken below should not effect page load times.
}

Function boost_stats_init() {
  Global $nid, $title, $q, $referer, $session_id, $uid, $stats, $count_views, $access_log, $stats_block;

  // Connect to DB.
  include_once './includes/bootstrap.inc';
  drupal_bootstrap(DRUPAL_BOOTSTRAP_DATABASE);

  // Set variables passed via GET.
  $nid = isset($_GET['nid']) ? $_GET['nid'] : NULL;
  $title = isset($_GET['title']) ? urldecode($_GET['title']) : NULL;
  $q = isset($_GET['q']) ? $_GET['q'] : NULL;
  $referer = isset($_GET['referer']) ? $_GET['referer'] : NULL;
  $stats = (isset($_GET['js']) && $_GET['js'] == 2) ? TRUE : NULL;
  $session_id = session_id();
  if (empty($session_id)) {
    $session_id = $_COOKIE[session_name()];
    if (empty($session_id)) {
      // This only goes in the access log; only used for stats, not creds.
      $session_id = md5($_SERVER['HTTP_USER_AGENT'] . ip_address());
    }
  }
  $uid = 0;

  // Node Counter
  if (isset($nid)) {
    $count_views = db_fetch_array(db_query_range("SELECT value FROM {variable} WHERE name = '%s'", 'statistics_count_content_views', 0, 1));
    $count_views = unserialize($count_views['value']);
  }

  // Access Log
  if (isset($title) && isset($q)) {
    $access_log = db_fetch_array(db_query_range("SELECT value FROM {variable} WHERE name = '%s'", 'statistics_enable_access_log', 0, 1));
    $access_log = unserialize($access_log['value']);
  }

  // stats block
  if (isset($stats)) {
    $stats_block = db_fetch_array(db_query_range("SELECT value FROM {variable} WHERE name = '%s'", 'boost_statistics_html', 0, 1));
    $stats_block = unserialize($stats_block['value']);
  }
}

Function boost_stats_update_node_counter() {
  Global $nid;

  // A node has been viewed, so update the node's counters.
  db_query('UPDATE {node_counter} SET daycount = daycount + 1, totalcount = totalcount + 1, timestamp = %d WHERE nid = %d', time(), $nid);
  // If we affected 0 rows, this is the first time viewing the node.
  if (!db_affected_rows()) {
    // We must create a new row to store counters for the new node.
    db_query('INSERT INTO {node_counter} (nid, daycount, totalcount, timestamp) VALUES (%d, 1, 1, %d)', $nid, time());
  }
}

Function boost_stats_add_access_log() {
  Global $title, $q, $referer, $session_id, $uid;

  db_query("INSERT INTO {accesslog} (title, path, url, hostname, uid, sid, timer, timestamp) values('%s', '%s', '%s', '%s', %d, '%s', %d, %d)", $title, $q, $referer, ip_address(), $uid, $session_id, timer_read('page'), time());
}

Function boost_stats_output_stats_block() {
  Global $stats_block;
  echo $stats_block;
}
