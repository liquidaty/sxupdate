# sxupdate

## Simple Cross-platform Update Utility

`sxupdate` is a simple, lightweight, cross-platform update utility designed to work with code in any language. It can be used from the command-line or within GUI applications, and supports either synchronous or asynchronous operations for fetching data and/or user interaction. Updates can be fetched over the internet or from a local file system.

This library was designed to make it easy to add auto-update to a command-line utility. It is written
in C, so should be easy to port for other purposes if desired. It is designed to be easily adopted
for web assembly, though that has not yet been done.

Unlike other updaters such as Sparkle, WinSparkle, Squirrel, GoUpdate, and Google Omaha,
`sxupdate` uses no UI components, allowing it to be extremely lean and platform-independent, and
avoiding duplication of and potential inconsistency between UI components.

Meanwhile, you can still use UI components by calling them in your callbacks, which will be invoked at
the appropriate time (for example, to ask the user whether to proceed with an update).

`sxupdate` reads metadata from a JSON file that has a similar structure to the XML file used
by [Sparkle](https://sparkle-project.org), which must conform to [schema/appcast.schema.json](schema/appcast.schema.json) and an example of which can be found [here](examples/appcast.json).

`sxupdate` uses [semantic versioning](https://semver.org/).

### Features
- Extremely lightweight and simple to use
  * total static library size is approximately 100kb, which is literally tens of thousands of times
    smaller than many other auto-updater static libraries that may be a Gb in size

- Cross-platform support (Windows, MacOS, Linux)

- Works with code in any language that can call a C function and pass it a C callback
  * please feel free to open an issue ticket, and/or a PR, if you are looking to use this,
    or have already used it, with a different programming language

- Suitable for both command-line and UI-based applications

- Supports synchronous and asynchronous operations

- Allows updates over the internet or via local file system


### Getting Started

1. **Building and installing**

    To install sxupdate, you can use the following command:

    ```
    git clone https://github.com/liquidaty/sxupdate.git
    ```

    Then navigate to the `sxupdate` directory and use the following commands
    to configure, build and install:

    ```
    cd sxupdate
    ./configure && make -C src install
    ```

    to run a simple file-based test:
    ```
    make -C examples test-simple
    ```

2. **Usage**

    To use sxupdate, you provide a callback that handles any interaction with the user
    such as asking whether to proceed with an update, as well as other necessary information.

    sxupdate will fetch the JSON metadata, compare the first version found with the current version

```
    /* get an update handle */
    sxupdate_t sxu = sxupdate_new();

    /* set the key used for signature verification */
    if(sxupdate_set_public_key_from_file(sxu, pem_path) != sxupdate_status_ok)
        err = 1;
    else {

      /* set a callback used to get info about my currently-running app */
      sxupdate_set_current_version(sxu, get_version);

      /* set a callback used to interact with the user (which can call my UI components) */
      sxupdate_set_interaction_handler(sxu, interaction_handler);

      /* set the URL that the metadata will be fetched from */
      sxupdate_set_url(sxu, url);

      /* set any custom headers, for example for authorization */
      if(have_custom_header)
        sxupdate_add_header(sxu, header_name, header_value);

      /* fetch the metadata and if appropriate, ask the user to confirm and proceed to download and install */
      if(sxupdate_execute(sxu))
        fprintf(stderr, "Error: %s\n", sxupdate_err_msg(sxu));
    }
    sxupdate_delete(sxu);

```

### Documentation

A simple but fully featured example is available at [examples/test.c](examples/test.c).

More comprehensive documentation is planned...


### TO DO

* CI/CD
* documentation. maybe use https://github.com/libgit2/docurium
* if/as new use cases demand: adapt for use with other programming languages or contexts


### Contributing

We welcome contributions! Please see our [contributing guide](CONTRIBUTING.md) for more details.

### License

sxupdate is released under the MIT License

### Support

If you encounter any issues or require further assistance, please file an issue in the GitHub issue tracker.

### Code of Conduct

Please note that this project is released with a [Contributor Code of Conduct](CODE_OF_CONDUCT.md). By participating in this project, you agree to abide by its terms.