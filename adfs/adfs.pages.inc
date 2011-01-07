<?php

/**
 * @version     $Id: adfs.pages.inc $
 * @copyright   Copyright (C) 2005 - 2010 Open Source Matters. All rights reserved.
 * @license     http://www.gnu.org/licenses/gpl-2.0.html
 * @package     Drupal
 * @subpackage  ADFS
 * @since 7.x
 */
// --------------------------------------------------------------------------
// Page Handlers: User Identity Management

  require_once(dirname(__FILE__) . "/adfsbridge.php");
  require_once(dirname(__FILE__) . "/adfsuserdetails.php");
  require_once(dirname(__FILE__) . "/adfsconf.php");

/**
 * Menu callback; Manage ADFS identities for the specified user.
 */
function adfs_user_identities($account) {
  module_load_include('inc', 'adfs');
  
  drupal_set_title(format_username($account));
  drupal_add_css(drupal_get_path('module', 'adfs') . '/adfs.css');

  // If view is after delete then just display the one in DB.
  if (!isset ($_SESSION['adfs_account_delete'])) {
  
  // Check to see if we got a response
  $response = new AdfsUserDetails();
  if (isset($_SESSION['adfs_response'])) {
    $adfs_response = $_SESSION['adfs_response'];
    $response = unserialize($adfs_response);
  }
  if ($response->nameIdentifier != '') {
    $identity = $response->nameIdentifier;
    $exists = db_query("SELECT authname FROM {authmap} WHERE authname = :authname", (array(':authname' => $identity)))->fetchField();
    if ($exists === FALSE) {
      $query = db_insert('authmap')
                      ->fields(array(
                          'uid' => $account->uid,
                          'authname' => $identity,
                          'module' => 'adfs',
                      ))
                      ->execute();
      drupal_set_message(t('Successfully added %identity', array('%identity' => $identity)));
      unset($_SESSION['adfs_response']);
    }
    /*elseif (($account->uid != $exists) & isset($_SESSION['adfs_authenticated']))  { 
      drupal_set_message(t('Provided ADFS account already mapped with a different user in this site. Please use a different ADFS account.'), 'error');
    }*/
    
    // We are done with the session store
    unset($_SESSION['adfs']);
  }
  }
  unset($_SESSION['adfs_account_delete']);
  $header = array(t('adfs'), t('Operations'));
  $rows = array();

  $result = db_query("SELECT * FROM {authmap} WHERE module='adfs' AND uid=:uid", array(':uid' => $account->uid));
  foreach ($result as $identity) {
    $rows[] = array(check_plain($identity->authname), l(t('Delete'), 'user/' . $account->uid . '/adfs/delete/' . $identity->aid));
  }

  $build['adfs_table'] = array(
      '#theme' => 'table',
      '#header' => $header,
      '#rows' => $rows,
  );
  
  if (!$rows) {
      $build['adfs_user_add'] = drupal_get_form('adfs_user_add');
  }
  return $build;
}

/**
 * Form builder; Add an adfs identity.
 *
 * @ingroup forms
 * @see adfs_user_add_validate()
 */
function adfs_user_add() {
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Add an ADFS'));
  return $form;
}

function adfs_user_add_submit($form, &$form_state) {
  module_load_include('inc', 'adfs');
  $return_to = 'user/' . arg(1) . '/adfs';
  // adfs_begin($form_state['values']['adfs_identifier'], $return_to);
  adfs_sso_redirect($return_to);
}

/**
 * Menu callback; Delete the specified adfs identity from the system.
 */
function adfs_user_delete_form($form, $form_state, $account, $aid = 0) {
  $authname = db_query("SELECT authname FROM {authmap} WHERE uid = :uid AND aid = :aid AND module = 'adfs'", array(
                      ':uid' => $account->uid,
                      ':aid' => $aid,
                  ))
                  ->fetchField();
  return confirm_form(array(), t('Are you sure that you want to delete this?'), 'user/' . $account->uid . '/adfs');
}

function adfs_user_delete_form_submit($form, &$form_state) {
  global $user;
  module_load_include('inc', 'adfs');
  unset($_SESSION['adfs']);
  unset($_SESSION['adfs_response']);
  $_SESSION['adfs_account_delete'] = TRUE;
  $query = db_delete('authmap')
                  ->condition('uid', $form_state['build_info']['args'][0]->uid)
                  ->condition('aid', $form_state['build_info']['args'][1])
                  ->condition('module', 'adfs')
                  ->execute();
  if ($query) {
    drupal_set_message(t('ADFS deleted.'));
  }
  $form_state['redirect'] = 'user/' . $form_state['build_info']['args'][0]->uid . '/adfs';
  //if ($user->uid == $form_state['build_info']['args'][0]->uid) {
  //  adfs_slo_redirect('user/' . $form_state['build_info']['args'][0]->uid . '/adfs');
  //}
}

?>
