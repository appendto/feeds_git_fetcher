<?php
// $Id: FeedsJSONPathParser.inc,v 1.1.2.4.2.4 2011/02/05 19:28:01 twistor Exp $

/**
 * @file
 * Home of the FeedsGITFetcher and related classes.
 */

/**
 * Definition of the import batch object created on the fetching stage by
 * FeedsGITFetcher.
 */
class FeedsGITFetcherResult extends FeedsFileFetcherResult {
  /**
   * Constructor.
   */
  public function __construct($repository_path) {
    parent::__construct('');
    $this->repository_path = $repository_path;
  }

  /**
   * Overrides parent::getRaw();
   */
  public function getRaw() {
    return $this->sanitizeRaw($this->repository_path);
  }

  /**
   * Overrides parent::getFilePath().
   */
  public function getFilePath() {
    if (!is_dir($this->repository_path) && !is_dir($this->repository_path."/.git/")) {
      throw new Exception(t('Repository @repopath is not accessible.', array('@repopath' => $this->repository_path)));
    }
    return $this->sanitizeRaw($this->repository_path);
  }
}

/**
 * Fetches data via HTTP.
 */
class FeedsGITFetcher extends FeedsFetcher {

  /**
   * Implements FeedsFetcher::fetch().
   */
  public function fetch(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);

    $repository = basename($source_config['source'],'.git');
    $path = $source_config['path'];
		$realpath = drupal_realpath($path);

    $cmd = "cd " 
      . escapeshellarg($realpath) 
      . "; "
		  . escapeshellcmd($this->config['git_bin_path']) 
      . " pull;";

    unset($output);
		@exec($cmd,$output,$return_status);

    // Make sure we've got a valid GIT repository that contains the .git directory
    if (is_dir($path) && is_dir($path . '/.git/')) {
      return new FeedsGITFetcherResult($path);
    }

    throw new Exception(t('Repository is not valid or it is an empty directory: %path', array('%path' => $source_config['path'])));
  }

  /**
   * Return an array of files in a directory.
   *
   * @param $dir
   *   A stream wreapper URI that is a directory.
   *
   * @return
   *   An array of stream wrapper URIs pointing to files. The array is empty
   *   if no files could be found. Never contains directories.
   */
  protected function listFiles($dir) {
    $dir = file_stream_wrapper_uri_normalize($dir);
    $files = array();
    if ($items = @scandir($dir)) {
      foreach ($items as $item) {
        if (is_file("$dir/$item") && strpos($item, '.') !== 0) {
          $files[] = "$dir/$item";
        }
      }
    }
    return $files;
  }

  /**
   * Source form.
   */
  public function sourceForm($source_config) {
    $form = array();
    $form['pre_source'] = array(
      '#type' => 'value',
      '#value' => empty($source_config['source']) ? 0 : $source_config['source'],
    );
    $form['path'] = array(
      '#type' => 'value',
      '#value' => empty($source_config['path']) ? 0 : $source_config['path'],
    );
    $form['source'] = array(
      '#type' => 'textfield',
      '#title' => t('Repository URL'),
      '#description' => t('Specify the repository URL.  ie. git://github.com:user/repository.git (<a href="http://www.kernel.org/pub/software/scm/git/docs/git-clone.html#URLS" target="_new">Docs</a>).  Unless you\'ve configured your server to successfully handle all transport protocols, a GIT Read-Only protocol URL is recommended.'),
      '#default_value' => empty($source_config['source']) ? '' : $source_config['source'],
    );
    return $form;
  }

  /**
   * Override parent::sourceFormValidate().
   */
  public function sourceFormValidate(&$values) {
    $source = $values['source'];

    // Verify that they actually submitted something
    if (empty($source)) {
      form_set_error('feeds][source', t('Please specify a valid GIT repository URL.'));
      return;
    }

    // Strip a trailing slash if necessary
    if(substr($source,-1) == '/') {
      $values['source'] = $source = substr($source,0,-1);
    }

    // Validate that the string provided ends in .git
    if(strtolower(substr($source,-4,4)) != '.git') {
      form_set_error('feeds][source', t('A Valid GIT Repository must end in \'.git\'. <a href="http://www.kernel.org/pub/software/scm/git/docs/git-clone.html#URLS" target="_new">See Documentation.</a>.'));
      return;
    }

    // Parse the repository name
    $repository = basename($source,'.git');
    if(empty($repository)) {
      form_set_error('feeds][source', t('Unable to successfully parse the repository name from the URL provided'));
    }
  }

  /**
   * Override parent::sourceSave().
   */
  public function sourceSave(FeedsSource $source) {
    // Ensure that our root directory has been created
    $home_dir = 'private://feeds_git_fetcher/';
    file_prepare_directory($home_dir, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS);

    // Extract and create all of the paths and names
    $source_config = $source->getConfigFor($this);


    // Editing a repository
    if($source_config['source'] === $source_config['pre_source']) {
      // DO NOTHING
    }
    // Creating a new repository
    else {
      // If we have an old repository on disk, clean it up
      if(!empty($source_config['pre_source']) && !empty($source_config['path'])) {
        $path = $source_config['path'];
		    file_unmanaged_delete_recursive($path);
        drupal_set_message("Repository at $path has been deleted");
      }

      // Parse the new repository
      $repository = basename($source_config['source'],'.git');
      $path = file_create_filename($repository,$home_dir);
      $source_config['path'] = $path;
      $source_config['pre_source'] = $source_config['source'];

      // Create the repositories home directory
      file_prepare_directory($path, FILE_CREATE_DIRECTORY | FILE_MODIFY_PERMISSIONS);

		  $realpath = drupal_realpath($path);

      // Clone the repository! Woot!
		  $cmd = escapeshellcmd($this->config['git_bin_path']) 
        . ' clone --origin origin ' 
        . escapeshellarg($source_config['source']) 
        . ' ' 
        . escapeshellarg($realpath);

      //drupal_set_message('Cloning: '.$cmd);

      // TODO - Return a realistic error if this fails
      unset($output);
		  @exec($cmd,$output,$return_status);

      if($return_status == 0) {
        drupal_set_message("Repository ".$source_config['source']." has been successfully cloned!");
      }
      else {
        drupal_set_message("Repository could not be cloned.  Please try changing the repository to a Read-Only URL or consult the documentation.");
      }
    }

    $source->setConfigFor($this, $source_config);
  }

  /**
   * Override parent::sourceDelete().
   */
  public function sourceDelete(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    $path = $source_config['path'];
		file_unmanaged_delete_recursive($path);
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    return array(
      'git_bin_path' => '/usr/bin/git'
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    $form = array();
    $form['git_bin_path'] = array(
      '#type' =>'textfield',
      '#title' => t('GIT Binary Path'),
      '#description' => t('Filesystem path to the GIT binary.'),
      '#required' => TRUE,
      '#default_value' => empty($this->config['git_bin_path']) ? '/usr/bin/git' : $this->config['git_bin_path']
    );
    return $form;
  }
}
