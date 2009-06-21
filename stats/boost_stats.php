<?php
// $Id:

//Script should take under 1mb of memory to work.
//prime php for background operations
ob_end_clean();
header("Connection: close");
ignore_user_abort();

// output of 1 pixel transparent gif
ob_start();
header("Content-type: image/gif");
header("Expires: Wed, 11 Nov 1998 11:11:11 GMT");
header("Cache-Control: no-cache");
header("Cache-Control: must-revalidate");
header("Content-Length: 45");
header("Connection: close");
printf("%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c",71,73,70,56,57,97,1,0,1,0,128,255,0,192,192,192,0,0,0,33,249,4,1,0,0,0,0,44,0,0,0,0,1,0,1,0,0,2,2,68,1,0,59);
ob_end_flush();
flush();

// Image returned, so time taken below should not effect page load times.
// Do background processing here - Connect to DB and set stats.
include_once './includes/bootstrap.inc';
drupal_bootstrap(DRUPAL_BOOTSTRAP_DATABASE);

$count_views = db_fetch_array(db_query_range("SELECT value FROM {variable} WHERE name = '%s'", 'statistics_count_content_views', 0, 1));
$count_views = unserialize($count_views['value']);
$access_log = db_fetch_array(db_query_range("SELECT value FROM {variable} WHERE name = '%s'", 'statistics_enable_access_log', 0, 1));
$access_log = unserialize($access_log['value']);

if ($count_views) {
  // We are counting content views.
  if (isset($_GET['nid']) && is_numeric($_GET['nid'])) {
    // A node has been viewed, so update the node's counters.
    db_query('UPDATE {node_counter} SET daycount = daycount + 1, totalcount = totalcount + 1, timestamp = %d WHERE nid = %d', time(), $_GET['nid']);
    // If we affected 0 rows, this is the first time viewing the node.
    if (!db_affected_rows()) {
      // We must create a new row to store counters for the new node.
      db_query('INSERT INTO {node_counter} (nid, daycount, totalcount, timestamp) VALUES (%d, 1, 1, %d)', $_GET['nid'], time());
    }
  }
}

if ($access_log && isset($_GET['title']) && isset($_GET['q'])) {
  // Log this page access.
  $session_id = session_id();
  if (empty($session_id)) {
    $session_id = $_COOKIE[session_name()];
    if (empty($session_id)) {
      $session_id = 'boost ' . ip_address();
    }
  }
  $referer = isset($GET['referer']) ? $GET['referer'] : '';
  $uid = 0;
  db_query("INSERT INTO {accesslog} (title, path, url, hostname, uid, sid, timer, timestamp) values('%s', '%s', '%s', '%s', %d, '%s', %d, %d)", urldecode($_GET['title']), $_GET['q'], $referer, ip_address(), $uid, $session_id, timer_read('page'), time());
}
exit;