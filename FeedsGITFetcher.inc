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
class FeedsGITFetcherResult extends FeedsFetcherResult {
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
    // TODO
    return $this->sanitizeRaw(file_get_contents($this->file_path));
  }

  /**
   * Overrides parent::getFilePath().
   */
  public function getFilePath() {
    if (!is_dir($this->repository_path) && !is_dir($this->repository_path."/.git/")) {
      throw new Exception(t('Repository @repopath is not accessible.', array('@repopath' => $this->repository_path)));
    }
    return $this->sanitizeFile($this->repository_path);
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

    $source_path = pathinfo($source_config['source']);
    $respository_name = $source_path['filename'];
    $uri = "public://feeds_git_fetcher/".$respository_name;
    $path = drupal_realpath($uri);

    $cmd = "cd $path; ".GIT_PATH." pull";
		

    // Just return a file fetcher result if this is a file.
    if (is_file($source_config['source'])) {
      return new FeedsGITFetcherResult($source_config['source']);
    }

    // Batch if this is a directory.
    $state = $source->state(FEEDS_FETCH);
    $files = array();
    if (!isset($state->files)) {
      $state->files = $this->listFiles($source_config['source']);
      $state->total = count($state->files);
    }
    if (count($state->files)) {
      $file = array_shift($state->files);
      $state->progress($state->total, $state->total - count($state->files));
      return new FeedsGITFetcherResult($file);
    }

    throw new Exception(t('Resource is not a file or it is an empty directory: %source', array('%source' => $source_config['source'])));
  }

  /**
   * Source form.
   */
  public function sourceForm($source_config) {
    $form = array();
// TODO - Deal with changing the URL
    $form['source'] = array(
      '#type' => 'textfield',
      '#title' => t('Repository URL'),
      '#description' => t('Specify the repository URL.  ie. git@github.com:user/repo-name.git'),
      '#default_value' => empty($source_config['source']) ? '' : $source_config['source'],
    );
    return $form;
  }

  /**
   * Override parent::sourceFormValidate().
   */
  public function sourceFormValidate(&$values) {
    // TODO
		// Verify the path ends in '.git'
    // Verify that we can pull a pathinfo['filename'] from the path
    // Attempt to clone, see if we have access
    // Return an error message if we do not

		// Check if that repository exists
    $source_path = pathinfo($values['source']);
		$respository_name = $source_path['filename'];
		$uri = "public://feeds_git_fetcher/".$respository_name;
		if(!file_prepare_directory($uri,FILE_CREATE_DIRECTORY)) {
			exit('Couldnt create dir');
		}
  }

  /**
   * Override parent::sourceSave().
   */
  public function sourceSave(FeedsSource $source) {
    $source_config = $source->getConfigFor($this);
    $source_path = pathinfo($source_config['source']);
		$respository_name = $source_path['filename'];

		$uri = "public://feeds_git_fetcher/".$respository_name;
		$path = drupal_realpath($uri);
		

    //print_r($source_path['filename']);
		$cmd = GIT_PATH . ' clone --recursive --origin feedsgitfetcher ' . $source_config['source'] . ' ' . $path;
		@exec($cmd);

    $source->setConfigFor($this, $source_config);
  }

  /**
   * Override parent::sourceDelete().
   */
  public function sourceDelete(FeedsSource $source) {
    $source_path = pathinfo($source_config['source']);
		$respository_name = $source_path['filename'];
		$uri = "public://feeds_git_fetcher/".$respository_name;
		file_unmanaged_delete_recursive($uri);
  }

  /**
   * Override parent::configDefaults().
   */
  public function configDefaults() {
    // TODO
    return array(
      'allowed_extensions' => 'txt csv tsv xml opml',
      'direct' => FALSE,
    );
  }

  /**
   * Override parent::configForm().
   */
  public function configForm(&$form_state) {
    // TODO
    $form = array();
/*
    $form['allowed_extensions'] = array(
      '#type' =>'textfield',
      '#title' => t('Allowed file extensions'),
      '#description' => t('Allowed file extensions for upload.'),
      '#default_value' => $this->config['allowed_extensions'],
    );
    $form['direct'] = array(
      '#type' =>'checkbox',
      '#title' => t('Supply path to file or directory directly'),
      '#description' => t('For experts. Lets users specify a path to a file <em>or a directory of files</em> directly,
        instead of a file upload through the browser. This is useful when the files that need to be imported
        are already on the server.'),
      '#default_value' => $this->config['direct'],
    );
*/
    return $form;
  }
}
