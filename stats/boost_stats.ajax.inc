<?php

/**
 * AJAX Menu Callback.
 */
function boost_stats_ajax_callback() {
  if (!isset($_GET['js'])) {
    // stats not called via JS, send image out & close connection.
    boost_stats_async_image();
  }

  // Exit if nothing was passed to this.
  if (   !isset($_GET['nid'])
      && !isset($_GET['title'])
      && !isset($_GET['q'])
      && !isset($_GET['referer'])
      ) {
    exit;
  }

  // Set variables passed via GET.
  $nid = (isset($_GET['nid']) && is_numeric($_GET['nid'])) ? $_GET['nid'] : NULL;
  $title = (isset($_GET['title']) && $_GET['title'] != 'NULL') ? urldecode($_GET['title']) : NULL;
  $q = (isset($_GET['q']) && $_GET['q'] != 'NULL') ? $_GET['q'] : NULL;
  $referer = isset($_GET['referer']) ? $_GET['referer'] : NULL;

  // This only goes in the access log; only used for stats, not creds.
  $session_id = session_id();
  if (empty($session_id)) {
    if (empty($_COOKIE[session_name()])) {
      if (empty($_SERVER['HTTP_USER_AGENT'])) {
        $session_id = md5(ip_address());
      }
      else {
        $session_id = md5($_SERVER['HTTP_USER_AGENT'] . ip_address());
      }
    }
    else {
      $session_id = $_COOKIE[session_name()];
    }
  }
  $uid = 0;

  // Set node counter.
  if (variable_get('statistics_count_content_views', FALSE)) {
    boost_stats_update_node_counter($nid);
  }

  // Set access log.
  if (variable_get('statistics_enable_access_log', FALSE)) {
    boost_stats_add_access_log($title, $q, $referer, $session_id, $uid);
  }

  // Return Data
  if (isset($_GET['js'])) {
    if ($_GET['js'] == 1) {
      $json = array();
      // Get stats block html.
      $json = array_merge($json, boost_stats_output_stats_block());

      // Send JSON Back
      if (!empty($json)) {
        echo json_encode($json);
      }
    }
    // Send HTML back
    elseif ($_GET['js'] == 2) {
      echo array_pop(boost_stats_output_stats_block());
    }
  }
}

/**
 * Update database node count.
 *
 * @param $nid
 *  Node ID of page.
 */
function boost_stats_update_node_counter($nid) {
  // A node has been viewed, so update the node's counters.
  db_query('UPDATE {node_counter} SET daycount = daycount + 1, totalcount = totalcount + 1, timestamp = %d WHERE nid = %d', time(), $nid);
  // If we affected 0 rows, this is the first time viewing the node.
  if (!db_affected_rows()) {
    // We must create a new row to store counters for the new node.
    db_query('INSERT INTO {node_counter} (nid, daycount, totalcount, timestamp) VALUES (%d, 1, 1, %d)', $nid, time());
  }
}

/**
 * Add entry to access log
 *
 * @param $title
 *  Title of page.
 * @param $q
 *  URL directory structure
 * @param $referrer
 *  Javascript referrer
 * @param $session_id
 *  Session ID
 * @param $uid
 *  User ID; should be 0.
 */
function boost_stats_add_access_log($title, $q, $referer, $session_id, $uid) {
  db_query("INSERT INTO {accesslog} (title, path, url, hostname, uid, sid, timer, timestamp) values('%s', '%s', '%s', '%s', %d, '%s', %d, %d)", $title, $q, $referer, ip_address(), $uid, $session_id, timer_read('page'), time());
}

/**
 * Return array with stats block html.
 */
function boost_stats_output_stats_block() {
  if (variable_get('boost_block_show_stats', FALSE)) {
      $block = module_invoke('statistics', 'block', 'view', 0);
      $block = $block['content'];
  }
  else {
    $block = 'NULL';
  }

  return array('#boost-stats' => $block);
}

/**
 * Returns a 1px image and allows for php to run in the background.
 */
function boost_stats_async_image() {
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
  // Do background processing. Time taken after this should not effect page loading.
}