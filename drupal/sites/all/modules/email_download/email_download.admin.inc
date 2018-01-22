<?php
/**
 * @file
 * Provides the admin and stats function
 */

function email_download_page_form($form_state) {
  $form['#attributes'] = array('enctype' => "multipart/form-data");
  $form['upload']['file'] = array(
    '#type' => 'file',
    '#title' => t('Upload file to download'),
    '#size' => 40,
  );
  $form['upload']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Upload'),
  );

  return $form;
}

function email_download_page_form_validate($form, &$form_state) {
  // Ensure the directory exists, create if not
  $dir = file_directory_path() . '/' . variable_get('email_download_directory', 'secure');
  if (!file_check_directory($dir)) {
    file_check_directory($dir, TRUE);
    $data = ("Allow From none\n" .
            "RewriteEngine On\n" .
            "RewriteRule ^.*$ index.php\n");
    file_save_data($data, $dir . "/.htaccess", FALSE);
  }
}

function email_download_page_form_submit($form, &$form_state) {
  $dir = file_directory_path() . '/' . variable_get('email_download_directory', 'secure');
  $save = file_save_upload('file', array(), $dir, TRUE);

  if ($save) {
    file_set_status($save, FILE_STATUS_PERMANENT);
    drupal_set_message(t('Upload successful. It can be downloaded under !file',
        array('!file' => l($save->filename, 'download/'. $save->filename))));
  }
}

function email_download_stats($limit = 20) {
  $result = pager_query('SELECT e.*, d.*, f.* FROM {email_download_email} e LEFT JOIN {email_download_download} d ON e.eid = d.eid LEFT JOIN {email_download_fields} f ON f.eid = e.eid WHERE d.file IS NOT NULL', $limit);
  
  $rows = array();
  while ($row = db_fetch_object($result)) {
    $rows[] = $row;
  }

  return theme('email_download_stats', $rows);
}

function email_download_settings() {
  $form = array();

  $files = _email_download_list_files();
  if (!count($files)) {
    drupal_set_message(t('To upload files to be available for download go to the <a href="!uploads">content uploads page</a>.', array('!uploads' => url('admin/content/email_download'))));
  }

  $form['email_download_directory'] = array(
    '#type' => 'textfield',
    '#title' => t('Download directory'),
    '#description' => t("Path relative to your files directory. Don't include the start or end slashes."),
    '#default_value' => variable_get('email_download_directory', 'secure'),
  );

  $form_settings = variable_get('email_download_form_settings', array());

  $form['form_items'] = array(
    '#tree' => TRUE,
  );
  
  $elements = array('first_name' => t('First name'), 'last_name' => t('Last name'), 'company' => t('Company'), 'title' => t('Title'));
  foreach ($elements as $name => $title) {
    $form['form_items'][$name]['title'] = array(
      '#type' => 'value',
      '#value' => $title,
    );
    
    $form['form_items'][$name]['enabled'] = array(
      '#type' => 'checkbox',
      '#title' => $title,
      '#description' => t('Select this if the @title element should be shown on the form.', array('@title' => strtolower($title))),
      '#default_value' => $form_settings[$name]['enabled'],
    );
    $form['form_items'][$name]['required'] = array(
      '#type' => 'checkbox',
      '#title' => $title,
      '#description' => t('Select this if the @title should be required.', array('@title' => strtolower($title))),
      '#default_value' => $form_settings[$name]['required'],
    );
  }

  $form['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );
  
  return $form;
}

function email_download_settings_submit($form, &$form_state) {
  variable_set('email_download_form_settings', $form_state['values']['form_items']);
  variable_set('email_download_directory', $form_state['values']['email_download_directory']);
}


/****** Theme functions *******/

function theme_email_download_page_form($form) {
  $output .= drupal_render($form);

  $files = _email_download_list_files();
  $output .= theme('email_download_files_list', $files, t('Uploaded files'), TRUE);
  $output .= email_download_stats();

  return $output;
}

function theme_email_download_stats($data) {
  $header = array(t('Email'), t('Last Download'), t('File'), t('Count'));

  $form_settings = variable_get('email_download_form_settings', array());
  foreach ($form_settings as $name => $item) {
    if ($item['enabled']) {
      $header[] = $item['title'];
    }
  }
  
  $rows = array();
  foreach ($data as $row) {
    $tmp_row = array(
      $row->email,
      format_date(strtotime($row->updated_at), 'small'),
      $row->file,
      array('data' => $row->count, 'style' => 'text-align: center;'),
      //l('reset', '') . ' ' . l('delete', ''),
    );
    foreach ($form_settings as $name => $item) {
      if ($item['enabled']) {
        $tmp_row[] = $row->$name;
      }
    }
    $rows[] = $tmp_row;
  }

  $output = theme('table', $header, $rows, array('class' => 'email-download'));
  $output .= theme('pager');
  return $output;
}


function theme_email_download_settings($form) {
  $fields = array();
  foreach (element_children($form['form_items']) as $name) {
    $fields[] = $form['form_items'][$name];
  }

  $rows = array();
  for ($i = 0; $i < count($fields); $i++) {
    $row = array();
    unset($fields[$i]['enabled']['#title']);
    unset($fields[$i]['enabled']['#description']);
    $row[] = array('data' => drupal_render($fields[$i]['enabled']));

    unset($fields[$i]['required']['#title']);
    unset($fields[$i]['required']['#description']);
    $row[] = array('data' => drupal_render($fields[$i]['required']));

    // Append the title field to the column. It's not displayed.
    $row[] = array('data' => $fields[$i]['title']['#value'] . drupal_render($fields[$i]['title']));
    $rows[] = $row;
  }
  unset($form['form_items']);

  $buttons = drupal_render($form['submit']);
  $output .= drupal_render($form);

  $output .= '<h3>' . t('Configure the email download form') . '</h3>';
  
  $header = array(t('Enabled'), t('Required'), t('Name'));
  $output .= theme('table', $header, $rows, array('width' => '100%'));
  $output .= $buttons;

  return $output;
}
