To build:
   ./configure
   make -C src install

To see configure options:
   ./configure --help

To build and run a simple example and test:
   make -C src install && make -C examples test-simple

To save a configuration to a specific configuration file:
   ./configure CONFIGFILE=/path/to/my-config.mk

which can be used with make via:
      make -C src install CONFIGFILE=/path/to/my-config.mk


TO DO:
  - use EVP_PKEY_verify() instead of RSA_verify()
  - support options to use verification hash other than SHA256