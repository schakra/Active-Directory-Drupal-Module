<?php

/*
 * To change this template, choose Tools | Templates
 * and open the template in the editor.
 */

/**
 * Handle the SAML settings.
 */
function adfs_settings_page() {
  global $adfs_config;
  
  $form = array();

  $form['adfs_idp_sso_target_url'] = array(
      '#type' => 'textfield',
      '#title' => t('Active Directory Federation Service URL'),
      '#description' => t('This URL should be provided by the ADFS IdP SSO provider, and is where the user will be redirected in order to authenticate.'),
      '#default_value' => variable_get('adfs_idp_sso_target_url', ''), //https://adfsdemo2.com/adfs/ls
      '#required' => TRUE,
  );

  $form['adfs_sp_identity'] = array(
      '#type' => 'textfield',
      '#title' => t('SP Identity - Realm'),
      '#description' => t('This is the identity of your local server provided to the ADFS IdP SSO provider, and defines how the server will communicate with your installation.'),
      '#default_value' => variable_get('adfs_sp_identity', ''),  //urn:federation:phpinterop2
      '#required' => TRUE,
  );
  
  $form['adfs_private_certificate_path'] = array(
      '#type' => 'textfield',
      '#title' => t('Private certificate Path'),
      '#description' => t('This is a private certificate used to decrypt responses from the IdP SSO provider, it is used when the server is setup to encrypt message responses.'),
      '#default_value' => variable_get('adfs_private_certificate_path', ''),
      '#required' => FALSE,
  );

  $form['adfs_private_certificate_password'] = array(
      '#type' => 'textfield',
      '#title' => t('Private certificate password'),
      '#description' => t('This is needed if your private certificate requires a password to use.'),
      '#default_value' => variable_get('adfs_private_certificate_password', ''),
      '#required' => FALSE,
  );

  // This is placed here so that a adfs_validated variable is set upon
  // successful submit. None of the ADFS services will probably work without
  // this set.
  $form['adfs_validated'] = array(
      '#type' => 'value',
      '#value' => TRUE,
  );

  return system_settings_form($form);
}

function adfs_settings_page_validate($form, &$form_state) {
  // Test the sso target URL.
  if (parse_url($form_state['values']['adfs_idp_sso_target_url'], PHP_URL_SCHEME) != 'https') {
    form_error($form['adfs_idp_sso_target_url'], t('URL must be a valid https URL.'));
  }
}

function adfs_config_array() {
  $config = array();

  require_once drupal_get_path('module', 'simplesamlphp') . '/config/config.php';

  $config['baseurlpath'] = drupal_get_path('module', 'simplesamlphp');
  $config['enable.wsfed-sp'] = TRUE;
//  $config['default-wsfed-idp'] = variable_get('adfs_idp_sso_target_url', 'urn:federation:pingfederate:localhost');
  //$config['session.handler'] = 'drupal';

  return $config;
}

?>
