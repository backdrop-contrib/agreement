<?php

/**
 * @file
 * Agreement administration page callback.
 */

/**
 * Display a list of agreement types.
 *
 * @return array
 *   A render array.
 */
function agreement_type_overview() {
  $content = array();
  $header = array(
    t('Type'),
    t('Machine name'),
    t('Agreement page'),
    t('Visibility'),
    t('Visibility pages'),
    t('Operations'),
  );
  $rows = array();
  $types = agreement_type_load();
  $operation_attributes = array(
    'class' => array('links', 'inline'),
  );

  foreach ($types as $name => $type) {
    $row = array(
      check_plain($type['type']),
      check_plain($type['name']),
      l(check_plain($type['path']), $type['path']),
    );
    $row[] = $type['settings']['visibility_settings'] ? t('Show on only the listed pages.') : t('Show on every page except the listed pages.');
    $row[] = array(
      'data' => check_markup($type['settings']['visibility_pages']),
    );
    $operations = array(
      'edit' => array(
        'title' => t('Modify'),
        'href' => 'admin/config/people/agreement/manage/' . $name,
      ),
      'reset' => array(
        'title' => t('Reset'),
        'href' => 'admin/config/people/agreement/manage/' . $name . '/reset',
      ),
      'delete' =>  array(
        'title' => t('Remove'),
        'href' => 'admin/config/people/agreement/manage/' . $name . '/delete',
      ),
    );
    $row[] = array(
      'data' => array(
        '#type' => 'dropbutton',
        '#links' => $operations,
      ),
    );
    $rows[] = $row;
  }

  $content['agreement_types'] = array(
    '#theme' => 'table',
    '#header' => $header,
    '#rows' => $rows,
    '#sticky' => FALSE,
    '#empty' => t('There are no agreement types. Please add an agreement type.'),
  );

  return $content;
}

/**
 * Agreement settings form.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form state array.
 * @param array $info
 *   (Optional) The agreement type info.
 *
 * @return array
 *   The form array.
 */
function agreement_settings_form($form, &$form_state, $info = NULL) {
  $config = config('agreement.settings');

  $is_new = NULL === $info || !isset($info['name']);
  $form_state['is_new'] = $is_new;
  $destination = backdrop_get_destination();

  // Get a list of user role IDs without the anonymous user role.
  $role_options = user_roles();
  unset($role_options[BACKDROP_ANONYMOUS_ROLE]);

  if (!$is_new) {
    $form_state['agreement'] = $info;
  }
  else {
    $form_state['agreement'] = array();
  }

  $form['type'] = array(
    '#description' => t('Provide a human-readable label for this agreement type.'),
    '#default_value' => $is_new ? '' : check_plain($info['type']),
    '#required' => TRUE,
    '#title' => t('Label'),
    '#type' => 'textfield',
  );

  $form['name'] = array(
    '#default_value' => $is_new ? '' : check_plain($info['name']),
    '#disabled' => !$is_new,
    '#required' => TRUE,
    '#maxlength' => 21,
    '#machine_name' => array(
      'exists' => 'agreement_type_name_exists',
      'source' => array('type'),
    ),
    '#type' => 'machine_name',
  );

  $form['settings'] = array(
    '#collapsible' => FALSE,
    '#collapsed' => FALSE,
    '#title' => t('Configuration'),
    '#tree' => TRUE,
    '#type' => 'fieldset',
  );

  $form['settings']['role'] = array(
    '#description' => t('Which users need to accept the agreement?'),
    '#default_value' => !$is_new ? $info['settings']['role'] : BACKDROP_AUTHENTICATED_ROLE,
    '#options' => $role_options,
    '#required' => TRUE,
    '#title' => t('User role'),
    '#type' => 'select',
    '#multiple' => TRUE,
  );

  $form['settings']['frequency'] = array(
    '#description' => t('How often should users be required to accept the agreement?'),
    '#default_value' => !$is_new ? $info['settings']['frequency'] : 0,
    '#options' => array(
      -1 => t('Only once'),
      0 => t('On every log in'),
      365 => t('Once a year'),
    ),
    '#required' => TRUE,
    '#title' => t('Frequency'),
    '#type' => 'select',
  );

  $form['agreement'] = array(
    '#description' => t('This is the agreement text.'),
    '#default_value' => !$is_new ? $info['agreement'] : '',
    '#title' => t('Agreement Text'),
    '#rows' => 12,
    '#type' => 'text_format',
    '#format' => !$is_new ? $info['settings']['format'] : agreement_get_translated_message('AGREEMENT_FORMAT'),
  );

  $form['settings']['title'] = array(
    '#description' => t('What should the title of the agreement page be?'),
    '#default_value' => !$is_new ? $info['settings']['title'] : $config->get('agreement_page_title'),
    '#required' => TRUE,
    '#title' => t('Agreement Page Title'),
    '#type' => 'textfield',
  );

  $form['path'] = array(
    '#description' => t('At what URL should the agreement page be located? Relative to site root. No leading or trailing slashes.'),
    '#default_value' => !$is_new ? $info['path'] : $config->get('agreement_page_url'),
    '#element_validate' => array('agreement_path_validate'),
    '#required' => TRUE,
    '#title' => t('Agreement Page URL'),
    '#type' => 'textfield',
  );

  $form['settings']['checkbox'] = array(
    '#description' => t('This text will be displayed next to the "I agree" checkbox.'),
    '#default_value' => !$is_new ? $info['settings']['checkbox'] : agreement_get_translated_message('AGREEMENT_CHECKBOX_TEXT'),
    '#required' => TRUE,
    '#title' => t('Agreement Checkbox Text'),
    '#type' => 'textfield',
  );

  $form['settings']['submit'] = array(
    '#description' => t('This text will be displayed on the "Submit" button.'),
    '#default_value' => !$is_new ? $info['settings']['submit'] : agreement_get_translated_message('AGREEMENT_SUBMIT_TEXT'),
    '#required' => TRUE,
    '#title' => t('Agreement Submit Text'),
    '#type' => 'textfield',
  );

  $form['settings']['success'] = array(
    '#description' => t('What message should be displayed to the users once they accept the agreement?'),
    '#default_value' => !$is_new ? $info['settings']['success'] : agreement_get_translated_message('AGREEMENT_MESSAGE_SUCCESS'),
    '#title' => t('Agreement Success Message'),
    '#type' => 'textfield',
  );

  $form['settings']['destination'] = array(
    '#description' => t('What page should be displayed after the user accepts the agreement? Leave blank
                            to go to original destination that triggered the agreement. %front is the front
                            page. Users who log in via the one-time login link will always be redirected to
                            their user profile to change their password.',
                            array('%front' => '<front>')
    ),
    '#default_value' => !$is_new ? $info['settings']['destination'] : '',
    '#title' => t('Agreement Success Destination'),
    '#type' => 'textfield',
  );

  $form['settings']['failure'] = array(
    '#description' => t('What message should be displayed to the users if they do not accept the agreement?'),
    '#default_value' => !$is_new ? $info['settings']['failure'] : agreement_get_translated_message('AGREEMENT_MESSAGE_FAILURE'),
    '#title' => t('Agreement Failure Message'),
    '#type' => 'textfield',
  );

  $form['settings']['revoked'] = array(
    '#description' => t('What message should be displayed to the users if they revoke their agreement?'),
    '#default_value' => !$is_new ? $info['settings']['revoked'] : agreement_get_translated_message('AGREEMENT_MESSAGE_REVOKED'),
    '#title' => t('Agreement Revoke Message'),
    '#type' => 'textfield',
  );

  $form['settings']['visibility_settings'] = array(
    '#type' => 'radios',
    '#title' => t('Show agreement on specific pages'),
    '#options' => array(
      t('Show on every page except the listed pages.'),
      t('Show on only the listed pages.'),
    ),
    '#required' => TRUE,
    '#default_value' => !$is_new ? $info['settings']['visibility_settings'] : 0,
  );

  $form['settings']['visibility_pages'] = array(
    '#type' => 'textarea',
    '#title' => t('Pages'),
    '#default_value' => !$is_new ? $info['settings']['visibility_pages'] : '<front>',
    '#description' => t("Enter one page per line as Backdrop paths. The '*' character is a wildcard. Example paths
                            are %blog for the blog page and %blog-wildcard for every personal blog. %front is the
                            front page.", array(
      '%blog' => 'blog',
      '%blog-wildcard' => 'blog/*',
      '%front' => '<front>',
    )
    ),
  );

  $form['settings']['email_recipient'] = array(
    '#type' => 'textfield',
    '#title' => t('Email notification of agreement acceptance to'),
    '#default_value' => !$is_new ? $info['settings']['email_recipient'] : '',
    '#description' => t('If an email address is entered here, an email will be sent to that email address each time a user agrees. To have no email sent, leave this blank.'),
    '#element_validate' => array('valid_email_address'),
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Save configuration'),
  );

  // Provide delete button if not default agreement.

  if (!$is_new && $info['name'] !== 'default') {
    $form['actions']['delete'] = array(
      '#submit' => array('agreement_settings_form_delete'),
      '#type' => 'submit',
      '#value' => t('Remove'),
    );
  }
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#href' => 'admin/config/people/agreement',
    '#title' => t('Cancel'),
  );
  return $form;
}

/**
 * Agreement path validate callback.
 *
 * @param array $element
 *   The element array.
 * @param array &$form_state
 *   The form state array.
 */
function agreement_path_validate($element, &$form_state) {
  $types = (array) agreement_type_load();
  unset($types[$form_state['values']['name']]);

  foreach ($types as $type) {
    if ($type['path'] == $element['#value']) {
      form_error($element, t('Path in use by another agreement.'));
      return;
    }
  }

  if ($form_state['is_new'] || $form_state['agreement']['path'] !== $element['#value']) {
    $router_item = db_select('menu_router')
      ->fields('menu_router', array('path'))
      ->condition('path', $element['#value'])
      ->execute()
      ->fetchField(0);

    if ($router_item) {
      form_error($element, t('Path in use by system.'));
    }
  }
}

/**
 * Agreement settings form validate callback.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form state array.
 */
function agreement_settings_form_validate($form, &$form_state) {
  $types = agreement_type_load();
  $pages = preg_split('/\r?\n/', $form_state['values']['settings']['visibility_pages']);

  // See if there are any agreement types that have the same visibility page

  // when visibility is set to match path.

  if ($form_state['values']['settings']['visibility_settings']) {
    foreach ($types as $name => $info) {
      if ($name <> $form_state['values']['name'] && $info['settings']['visibility_settings']) {
        $type_pages = preg_split('/\r?\n/', $info['settings']['visibility_pages']);
        $diff = array_diff($pages, $type_pages);
        if (count($diff) < count($pages)) {
          form_set_error('settings[visibility_pages]', t('Visibility page in-use by another agreement.'));
          break;
        }
      }
    }
  }
}

/**
 * Get the redirect path based on destination, if it exists.
 *
 * @param string $default_path
 *   The default path to use if destination does not exist.
 *
 * @return array
 *   The path array to use in the link or redirect.
 */
function agreement_settings_form_get_destination($default_path) {
  $destination = backdrop_get_destination();
  $options = array('path' => $default_path);
  if (isset($destination['destination'])) {
    $options = backdrop_parse_url($destination['destination']);
  }

  return $options;
}

/**
 * Agreement settings form submit callback.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form state array.
 */
function agreement_settings_form_submit($form, &$form_state) {
  $rebuild = FALSE;
  $is_new = empty($form_state['agreement'])?TRUE : FALSE;
  $info = _agreement_extract_values_to_type($form_state['values']);
  $new = array_replace_recursive($form_state['agreement'], $info);

  // Rebuilds the menu cache if the path URL or page title changes.
  if ($is_new || $form_state['values']['path'] !== $form_state['agreement'] ||
      $form_state['values']['settings']['title'] !== $form_state['agreement']['settings']['title']) {
    $rebuild = TRUE;
  }

  $saved = agreement_type_save($new, $rebuild);

  backdrop_set_message(t('Agreement type %type saved successfully.', array('%type' => $saved['type'])));
  $form_state['redirect'] = 'admin/config/people/agreement';
}

/**
 * Extract form values into an agreement type.
 *
 * @param array $values
 *   Form state values.
 *
 * @return array
 *   An agreement type array.
 */
function _agreement_extract_values_to_type($values) {
  $keys = array('settings', 'name', 'type', 'path', 'agreement');

  $role_values = isset($values['settings']['role']) ? $values['settings']['role'] : array();
  $values['settings']['role'] = array();
  foreach ($role_values as $key => $value) {
    if ($value) {
      $values['settings']['role'][] = $key;
    }
  }

  $info = backdrop_map_assoc($keys, function ($key) use ($values){
    if (isset($values[$key])) {
      if ($key === 'agreement'){
        return $values[$key]['value'];
      }
      return $values[$key];
    }
    return NULL;
  });

  $info['settings']['format'] = $values['agreement']['format'];

  // Enforce data types on options values.
  $options_keys = array('frequency', 'visibility_settings');
  foreach ($options_keys as $option) {
    $info['settings'][$option] = (integer) $info['settings'][$option];
  }

  return $info;
}

/**
 * Agreement settings form delete submit callback.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form state array.
 */
function agreement_settings_form_delete($form, &$form_state) {
  $form_state['redirect'] = 'admin/config/people/agreement/manage/' . $form_state['agreement']['name'] . '/delete';
}

/**
 * Agreement type delete form.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form state array.
 * @param array $info
 *   The agreement type.
 *
 * @return array
 *   The form array.
 */
function agreement_type_delete_form($form, &$form_state, $info) {
  $form_state['name'] = $info['name'];
  $form_state['label'] = $info['type'];

  return confirm_form($form,
               t('Are you sure you want to delete this agreement type?'),
               'admin/config/people/agreement/manage',
               t('Removing an agreement type will delete all user agreements for that type.')
  );
}

/**
 * Agreement type delete form submit.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form state array.
 */
function agreement_type_delete_form_submit($form, &$form_state) {
  
  try {
    $transaction = db_transaction();
    db_delete('agreement')
      ->condition('type', $form_state['name'])
      ->execute();

    $config = config('agreement_type.' . $form_state['name']);
    $config->delete();

    backdrop_set_message(t('Successfully deleted agreement type, %label', array('%label' => $form_state['label'])));
    $form_state['redirect'] = 'admin/config/people/agreement';
    agreement_cache_clear();
    state_set('menu_rebuild_needed', TRUE);
  }
  catch (\PDOException $e) {
    $transaction->rollback();

    backdrop_set_message(
      t('An error occurred deleting agreement type %name. %message',
        array('%name' => $form_state['name'], '%message' => $e->getMessage())
      ),
      'error'
    );
  }
}

/**
 * Provides an interface to reset all agreements for an agreement type.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form state array.
 * @param array $info
 *   The agreement type.
 *
 * @return array
 *   The form array.
 */
function agreement_type_reset_form($form, &$form_state, $info) {
  $form_state['agreement'] = $info;

  $form['confirm_message'] = array(
    '#markup' => t('Force all users to re-accept the @type agreement?', array('@type' => $info['type'])),
  );
  $form['confirm'] = array(
    '#type' => 'hidden',
    '#value' => 1,
  );

  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array(
    '#type' => 'submit',
    '#value' => t('Confirm'),
  );
  $form['actions']['cancel'] = array(
    '#type' => 'link',
    '#title' => t('Cancel'),
    '#href' => 'admin/config/people/agreement',
    '#options' => array(),
  );

  return $form;
}

/**
 * Reset agreement type form submit.
 *
 * @param array $form
 *   The form array.
 * @param array &$form_state
 *   The form state array.
 */
function agreement_type_reset_form_submit($form, &$form_state) {
  $info = $form_state['agreement'];
  $info['settings']['reset_date'] = time();

  agreement_type_save($info);

  backdrop_set_message(t('All users will be forced to re-accept the @type agreement.', array('@type' => $info['type'])));

  $form_state['redirect'] = 'admin/config/people/agreement';
}
