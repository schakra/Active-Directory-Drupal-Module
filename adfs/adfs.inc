<?php

/**
 * @version     $Id: adfs.inc $
 * @copyright   Copyright (C) 2005 - 2010 Open Source Matters. All rights reserved.
 * @license     http://www.gnu.org/licenses/gpl-2.0.html
 * @package     Drupal
 * @subpackage  ADFS
 * @since 7.x
 */

/**
 * Performs an HTTP 302 redirect (for the 1.x protocol).
 */
function adfs_redirect_http($url, $message) {
  $query = array();
  foreach ($message as $key => $val) {
    $query[] = $key . '=' . urlencode($val);
  }

  $sep = (strpos($url, '?') === FALSE) ? '?' : '&';
  header('Location: ' . $url . $sep . implode('&', $query), TRUE, 302);
  /* Disable caching of this response. */
  header('Pragma: no-cache');
  header('Cache-Control: no-cache, must-revalidate');

  drupal_exit();
}

/**
 * Creates a js auto-submit redirect for (for the 2.x protocol)
 */
function adfs_redirect($url, $message) {
  global $language;

  $output = '<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "http://www.w3.org/TR/xhtml1/DTD/xhtml1-strict.dtd">' . "\n";
  $output .= '<html xmlns="http://www.w3.org/1999/xhtml" xml:lang="' . $language->language . '" lang="' . $language->language . '">' . "\n";
  $output .= "<head>\n";
  $output .= "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n";
  $output .= "<title>" . t('adfs redirect') . "</title>\n";
  $output .= "</head>\n";
  $output .= "<body>\n";
  $elements = drupal_get_form('adfs_redirect_form', $url, $message);
  $output .= drupal_render($elements);
  $output .= '<script type="text/javascript">document.getElementById("adfs-redirect-form").submit();</script>' . "\n";
  $output .= "</body>\n";
  $output .= "</html>\n";
  print $output;

  drupal_exit();
}

function adfs_redirect_form($form, &$form_state, $url, $message) {
  $form['#action'] = $url;
  $form['#method'] = "post";
  foreach ($message as $key => $value) {
    $form[$key] = array(
        '#type' => 'hidden',
        '#name' => $key,
        '#value' => $value,
    );
  }
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
      '#type' => 'submit',
      '#prefix' => '<noscript><div>',
      '#suffix' => '</div></noscript>',
      '#value' => t('Send'),
  );

  return $form;
}

function adfs_sso_redirect($return_to = NULL) {

  $spentityid = variable_get('adfs_sp_identity'); // 'urn:federation:si-barneym-adfs';

  adfs_redirect_http(variable_get('adfs_idp_sso_target_url'), // 'https://adfsdemo2.com/adfs/ls',
          array(
              'wa' => 'wsignin1.0',
              'wct' => gmdate('Y-m-d\TH:i:s\Z', time()),
              'wtrealm' => $spentityid,
              'wctx' => $return_to,
  ));
  drupal_exit();
}

function adfs_slo_redirect($return_to = NULL) {

  $spentityid = variable_get('adfs_sp_identity'); // 'urn:federation:si-barneym-adfs';

  if ($_SESSION['adfs_authenticated']) {
    adfs_redirect_http(variable_get('adfs_idp_sso_target_url'), // 'https://adfsdemo2.com/adfs/ls',
            array(
                'wa' => 'wsignout1.0',
                'wct' => gmdate('Y-m-d\TH:i:s\Z', time()),
                'wtrealm' => $spentityid,
                'wctx' => $return_to
    ));
  } else {
    drupal_goto($return_to);
  }
  drupal_exit();
}

/**
 * parseSAML2Time is from simpleSAMLphp Utilities
 *
 * This function converts a SAML2 timestamp on the form
 * yyyy-mm-ddThh:mm:ss(\.s+)?Z to a UNIX timestamp. The sub-second
 * part is ignored.
 *
 * Andreas comments:
 *  I got this timestamp from Shibboleth 1.3 IdP: 2008-01-17T11:28:03.577Z
 *  Therefore I added to possibliity to have microseconds to the format.
 * Added: (\.\\d{1,3})? to the regex.
 *
 *
 * @param string $time  The time to convert in SAML2 format
 * @return string  $time converted to a unix timestamp.
 */
function parseSAML2Time($time) {
  $matches = array();


  /* We use a very strict regex to parse the timestamp. */
  if (preg_match('/^(\\d\\d\\d\\d)-(\\d\\d)-(\\d\\d)' .
                  'T(\\d\\d):(\\d\\d):(\\d\\d)(?:\\.\\d+)?Z$/D',
                  $time, $matches) == 0) {
    throw new Exception(
            'Invalid SAML2 timestamp passed to' .
            ' parseSAML2Time: ' . $time);
  }

  /* Extract the different components of the time from the
   * matches in the regex. intval will ignore leading zeroes
   * in the string.
   */
  $year = intval($matches[1]);
  $month = intval($matches[2]);
  $day = intval($matches[3]);
  $hour = intval($matches[4]);
  $minute = intval($matches[5]);
  $second = intval($matches[6]);

  /* We use gmmktime because the timestamp will always be given
   * in UTC.
   */
  $ts = gmmktime($hour, $minute, $second, $month, $day, $year);

  return $ts;
}

/**
 * checkCurrentTime is from simpleSAMLphp Utilities
 *
 * Check to verify that the current time is between
 * the specified start and end boundary
 *
 * @param string $start time in SAML2 format
 * @param string $end time in SAML2 format
 * @return boolean
 */
function checkCurrentTime($start=NULL, $end=NULL) {
  $currentTime = time();

  if (!empty($start)) {
    $startTime = parseSAML2Time($start);
    /* Allow for a 10 minute difference in Time */
    if (($startTime < 0) || (($startTime - 600) > $currentTime)) {
      return FALSE;
    }
  }
  if (!empty($end)) {
    $endTime = parseSAML2Time($end);
    if (($endTime < 0) || ($endTime <= $currentTime)) {
      return FALSE;
    }
  }
  return TRUE;
}

  /**
   * Decrypts the xmlToken if it is encrypted, using the private key specified in the configuration.
   */

/**
 *
 * @param DOMElement $rootElement the XML SAML document possibly containing encrypted data
 * @param array $response response array with details about the processing
 * @return array ($response) 'decrypted'=FALSE indicates failure
 */
function adfs_decrypt($rootElement, $response = array('status' => 'failed')) {
  $cryptoKey = variable_get('adfs_private_certificate_path','');
  $cryptoKeyPass = variable_get('adfs_private_certificate_password','');

  // This will be set to TRUE if there is no decryption, or we successfuly decrypt
  $response['decrypted'] = FALSE;

  $rootElement = $xpath->query('/wst:RequestSecurityTokenResponse/wst:RequestedSecurityToken/xenc:EncryptedData');
  $rootElement = $rootElement->item(0);
  if (preg_match('/EncryptedData/i', $rootElement->nodeName) > 0) {
    $topNode = $rootElement->firstChild;
    if (preg_match('/EncryptionMethod/i', $topNode->nodeName) > 0) {
      if ( ($blockAlgorithm = $topNode->getAttribute("Algorithm")) ) {
        switch ($blockAlgorithm) {
          case "http://www.w3.org/2001/04/xmlenc#aes256-cbc":
            $mcrypt_cipher = MCRYPT_RIJNDAEL_128;
            $mcrypt_mode = MCRYPT_MODE_CBC;
            $iv_length = 16;
            break;

          case "http://www.w3.org/2001/04/xmlenc#aes128-cbc":
            $mcrypt_cipher = MCRYPT_RIJNDAEL_128;
            $mcrypt_mode = MCRYPT_MODE_CBC;
            $iv_length = 16;
            break;

          default:
            $response['message'] = "Unknown encryption blockAlgorithm: " . $blockAlgorithm . ".";
            return $response;
            break;
        }
        # Alg. has been determined, check to make sure an error hasn't been thrown, and proceed.
        if ($this_error == false) {
          $topNode = $topNode->nextSibling;
          if (preg_match('/KeyInfo/i', $topNode->nodeName) > 0) {
            $encryptionMethods = $topNode->getElementsByTagname("EncryptionMethod");
            $encryptionMethod = $encryptionMethods->item(0);
            $keyWrapAlgorithm = $encryptionMethod->getAttribute("Algorithm");
            switch ($keyWrapAlgorithm) {
              case "http://www.w3.org/2001/04/xmlenc#rsa-oaep-mgf1p":
                $ssl_padding = OPENSSL_PKCS1_OAEP_PADDING;
                break;

              case "http://www.w3.org/2001/04/xmlenc#rsa-1_5":
                $ssl_padding = OPENSSL_NO_PADDING;
                break;

              default:
            $response['message'] = "Unrecognized keyWrapAlgorithm: " . $keyWrapAlgorithm . ".";
            return $response;
                break;
            }
            if ($this_error == false) {
              if ($cipherValueNodes = $topNode->getElementsByTagname("CipherValue")) {
                $cipherValueNode = $cipherValueNodes->item(0);
                $keyWrapCipher = $cipherValueNode->nodeValue;
                $keyWrapCipher = base64_decode($keyWrapCipher);
                $private_key = openssl_pkey_get_private($this_cryptoKey, $this_cryptoKeyPass);
                if (!$private_key) {
                  $response['message'] = "Unable to load private key for decryption.";
                  return $response;
                } else {
                  if (openssl_private_decrypt($keyWrapCipher, $blockCipherKey, $private_key, $ssl_padding)) {
                    openssl_free_key($private_key);
                    switch ($keyWrapAlgorithm) {
                      case "http://www.w3.org/2001/04/xmlenc#rsa-1_5":
                        $blockCipherKey = substr($blockCipherKey, 2);
                        $keystart = strpos($blockCipherKey, 0) + 1;
                        $blockCipherKey = substr($blockCipherKey, $keystart);
                        break;

                      default:
                        break;
                    }
                    $topNode = $topNode->nextSibling;
                    if (preg_match('/CipherData/i', $topNode->nodeName) > 0) {
                      if (!$cipherValueNodes = $topNode->getElementsByTagname("CipherValue")) {
                        $response['message'] = "No block cipher data found.";
                        return $response;
                      } else {
                        $cipherValueNode = $cipherValueNodes->item(0);
                        $blockCipher = $cipherValueNode->nodeValue;
                        $blockCipher = base64_decode($blockCipher);

                        if ($iv_length > 0) {
                          $mcrypt_iv = substr($blockCipher, 0, $iv_length);
                          $blockCipher = substr($blockCipher, $iv_length);
                        }
                        $token = mcrypt_decrypt($mcrypt_cipher, $blockCipherKey, $blockCipher, $mcrypt_mode, $mcrypt_iv);
                        if (!$token) {
                          $response['message'] = "Decryption of token failed.";
                          return $response;
                        } else {
                          // Is this an error condition?
                          $response['infocardObj'] = $token;
                          //return $response;
                        }
                      }
                    } else {
                      $response['message'] = "Unable to locate cipher data.";
                      return $response;
                    }
                  } else {
                    $response['message'] = "Unable to decrypt token, check private key configuration.";
                    return $response;
                  }
                }
              } else {
                $response['message'] = "No wrapping cipher found.";
                return $response;
              }
            }
          } else {
            $response['message'] = "Unable to continue, keyInfo is not present.";
            return $response;
          }
        }
      } else {
        $response['message'] = "Encryption method BlockAlgorithm not specified.";
        return $response;
      }
    } else {
      $response['message'] = "Unable to determine Encryption method.";
      return $response;
    }
  } else {
    $response['message'] = "Unable to find encrypted data.";
    return $response;
  }


//  $cert_key = openssl_pkey_get_private($certFile);

  $response['decrypted'] = TRUE;
  return $response;
}

/**
 * adfs_process_prp is based on simpleSAMLphp prp.php
 *
 * Process an inbound WSFED request and extract assertions and if necessary
 * validate against configured certificate
 *
 * @return array containing details from the IdP Request
 */
function adfs_process_prp() {
  $response = array();

  $response['status'] = 'failed';

  // This message is sent by the authentication server to inform the site to
  // clean up its session data.
  if (!empty($_GET['wa']) and ($_GET['wa'] == 'wsignoutcleanup1.0')) {
    print 'Logged Out';
    user_logout();
    drupal_exit();
  }

  // Optional Response Items
  $response['destination'] = (!empty($_POST['wctx']) ? $_POST['wctx'] : NULL);

  // Required Response Items
  if (empty($_POST['wresult']) || empty($_POST['wa'])) {
    $response['message'] = 'ADFS authentication response missing required items.';
    return $response;
  }
  $wa = $_POST['wa'];
  $wresult = $_POST['wresult'];


  $document = new DOMDocument();


  // MS-ADFS returns escaped quotes
  $wresult = str_replace('\"', '"', $wresult);
  $document->loadXML(str_replace("\r", "", $wresult));

  $xpath = new DOMXpath($document);
  $xpath->registerNamespace('wst', 'http://schemas.xmlsoap.org/ws/2005/02/trust');
  $xpath->registerNamespace('saml', 'urn:oasis:names:tc:SAML:1.0:assertion');

  // Query for saml:Assertion element
  $assertions = $xpath->query('/wst:RequestSecurityTokenResponse/wst:RequestedSecurityToken/saml:Assertion');
  if ($assertions->length === 0) {
    throw new Exception('Received a response without an assertion on the WS-Fed PRP handler.');
  }
  if ($assertions->length > 1) {
    throw new Exception('The WS-Fed PRP handler currently only supports a single assertion in a response.');
  }
  $assertion = $assertions->item(0);

  // Find the entity id of the issuer.
  $idpEntityId = $assertion->getAttribute('Issuer');

  // Check time constraints of response conditions
  foreach ($xpath->query('./saml:Conditions', $assertion) as $condition) {
    $notBefore = $condition->getAttribute('NotBefore');
    $notOnOrAfter = $condition->getAttribute('NotOnOrAfter');
    if (!checkCurrentTime($notBefore, $notOnOrAfter)) {
      $response['message'] = 'The response has expired.';
    }
  }

  // Extract the attributes from the response
  $attributes = array();
  $attributeValues = $xpath->query('./saml:AttributeStatement/saml:Attribute/saml:AttributeValue', $assertion);
  foreach ($attributeValues as $attribute) {
    $name = $attribute->parentNode->getAttribute('AttributeName');
    $value = $attribute->textContent;
    if (!array_key_exists($name, $attributes)) {
      $attributes[$name] = array();
    }
    $attributes[$name][] = $value;
  }


  // Extract the name identifier from the response
  if ($certData) {
    $nameid = $xpath->query('./saml:AuthenticationStatement/saml:Subject/saml:NameIdentifier', $assertion);

    // If we did not get a name then authentication failed
    if ($nameid->length === 0) {
      $response['message'] = 'Could not find the name identifier in the response from the WS-Fed IdP \'' .
              $idpEntityId . '\'.';
      return $response;
    }
    $nameid = array(
        'Format' => $nameid->item(0)->getAttribute('Format'),
        'value' => $nameid->item(0)->textContent,
    );
  } else {
    $nameid = array(
        'Format' => 'Plain',
        'value' => $attributes[variable_get('adfs_user_attribute', 'name')][0],
    );
  }

  // We have a successful login, setup response details
  $response['adfs_identifier'] = $nameid['value'];


  $response['uid'] = $attributes[variable_get('adfs_user_attribute', 'name')][0];

  $userId = variable_get('adfs_user_attribute', 'name');
  if (isset($userId) && isset($attributes[$userId])) {
    $response['uid'] = $attributes[$userId][0];
  } else {
    $response['uid'] = $response['adfs_identifier'];
  }

  $emailId = variable_get('adfs_email_attribute', 'email');
  if (isset($emailId) && isset($attributes[$emailId])) {
    $response['email'] = $attributes[$emailId][0];
  }
  $response['authenticated'] = TRUE;
  $response['protocol'] = 'wsfed';
  $response['attributes'] = $attributes;
  $response['idp'] = $idpEntityId;
  $response['status'] = 'success';
  // Track Session Identity
  $_SESSION['adfs_authenticated'] = TRUE;
  $_SESSION['adfs_identifier'] = $userid;
  return $response;
}

?>
