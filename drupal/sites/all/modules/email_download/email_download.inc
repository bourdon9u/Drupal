<?php
/**
 * @file
 * Provides the download file form and process
 */

// TODO: The redirection doesn't work properly on this form on the first
// run through when the cookie hasn't been set yet.
function email_download_download_file_form($form_state, $download) {
  $form = array();

  $step = isset($form_state['storage']['step']) ? $form_state['storage']['step'] + 1 : 1;

  if ($_COOKIE['download_auth'] != '') {
    list($email, $auth) = explode("|", $_COOKIE['download_auth']);
    $result = db_query("SELECT * FROM {email_download_email} WHERE email = '%s' AND auth = '%s'", $email, $auth);
    $auth_obj = db_fetch_object($result);
    if ($auth_obj != NULL && $auth_obj->email == $email) {
      email_download_log($email, $download);
      email_download_get_file($download);
    }
  }

  if (user_access('download without email verification')) {
    global $user;
    $email = $user->mail;
    email_download_log($email, $download); // Trying to insert user's email
    email_download_get_file($download);
  }

  $form['download']['email'] = array(
    '#type' => 'textfield',
    '#title' => t('Please provide your email'),
  );

  $form_settings = variable_get('email_download_form_settings', array());
  foreach ($form_settings as $name => $item) {
    if ($item['enabled']) {
      $form['download'][$name] = array(
        '#type' => 'textfield',
        '#title' => $item['title'],
        '#required' => $item['required'],
      );
    }
  }

  $form['download']['buttons']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  $form['download']['buttons']['back'] = array(
    '#value' => l(t('Back to downloads page'), 'downloads'),
  );

  $form['download']['download'] = array(
    '#type' => 'value',
    '#value' => $download,
  );

  $form['#redirect'] = 'downloads';

  return $form;
}

function email_download_download_file_form_validate($form, &$form_state) {
  if (!valid_email_address($form_state['values']['email'])) {
    form_set_error('download', t('Email is invalid!'));
  }
}

function email_download_download_file_form_submit($form, &$form_state) {
  $download = $form_state['values']['download'];
  $email = $form_state['values']['email'];

  $result = db_query("SELECT * FROM {email_download_email} WHERE email='%s'", $email);
  $email_obj = db_fetch_object($result);

  if ($email_obj != NULL && $email_obj->email == $email) {
    drupal_set_message(t('Your username already exists in the database. Please enable cookies for these downloads to work.'));
    $auth = $email_obj->auth;
  }
  else {
    $auth = md5(uniqid(""));
    db_query(
      "INSERT INTO {email_download_email} " .
      "(email, auth, updated_at) " .
      "VALUES ('%s', '%s', '". date("Y-m-d H:i") ."')",
      $email, $auth
    );
    $eid = db_last_insert_id('email_download_email', 'eid');
    db_query("INSERT INTO {email_download_fields} (eid, first_name, last_name, company, title) VALUES (%d, '%s', '%s', '%s', '%s')", $eid, $form_state['values']['first_name'], $form_state['values']['last_name'], $form_state['values']['company'], $form_state['values']['title']);
  }
  setcookie("download_auth", $email ."|". $auth, strtotime('+10 years'));

  email_download_log($email, $download);
  email_download_get_file($download);
}

function email_download_get_file($download) {
  $dir = file_directory_path() . '/' . variable_get('email_download_directory', 'secure');
  $fullpath = $dir ."/". $download;

  // Make sure the file exists
  if (!file_exists($fullpath)) {
    drupal_not_found();
    exit();
  }
  
  $mm_type="application/octet-stream";

  header("Cache-Control: public, must-revalidate");
  header("Pragma: hack");
  header("Content-Type: " . $mm_type);
  header("Content-Length: " . (string)(filesize($fullpath)));
  header('Content-Disposition: attachment; filename="' . $download . '"');
  header("Content-Transfer-Encoding: binary\n");

  readfile($fullpath);
  exit();
}

function email_download_log($email, $download) {
  $result = db_query(
    "SELECT * FROM {email_download_email} e " .
    "JOIN {email_download_download} d ON e.eid = d.eid " .
    "WHERE e.email = '%s' AND d.file = '%s'",
    $email, $download
  );

  if (db_fetch_object($result)) {
    db_query(
      "UPDATE {email_download_download} d " .
      "LEFT JOIN {email_download_email} e ON d.eid = e.eid " .
      "SET d.count = d.count+1, d.updated_at = '" . date("Y-m-d H:i:s") . "' " .
      "WHERE e.email='%s' AND d.file='%s'",
      $email, $download
    );
  }
  else {
    $result = db_query("SELECT eid FROM {email_download_email} WHERE email='%s'", $email);
    $eid = db_fetch_object($result)->eid;
    db_query(
      "INSERT INTO {email_download_download} " .
      "(eid, did, updated_at, count, file) VALUES " .
      "(%d, null, '". date("Y-m-d H:i:s") ."', 1, '%s')",
      $eid, $download
    );
  }
}
