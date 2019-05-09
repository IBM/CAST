.. _CSM_INSTALLATION_AND_CONFIGURATION_Pre_Requisites:

Pre-Requisites
==============

All nodes participating in :ref:`CSM` must be running Red Hat Enterprise Linux for PPC64LE.

Software Dependencies
---------------------

CSM has software dependencies. Verify that the following packages are installed: 

Hard dependencies
^^^^^^^^^^^^^^^^^

Without these dependencies CSM cannot run.

.. list-table:: Hard Dependencies
   :widths: 25 25 50
   :header-rows: 1

   * - Software
     - Version
     - Comments
   * - xCAT
     - 2.13.3 or higher
     - 
   * - Postgres SQL
     - 9.2.18 or higher
     - `xCAT document for migration <https://xcat-docs.readthedocs.io/en/stable/advanced/hierarchy/databases/postgres_configure.html>`_
   * - openssl-libs
     - 1.0.1e-60 or higher
     - 
   * - perl-YAML
     - 0.84-5 or higher
     - Required by the Diagnostic's tests.
   * - perl-JSON
     - 2.59 or higher
     - Required by the Diagnostic's tests that get information from the UFM. 
   * - cast-boost
     - 1.60.0-4
     - Found on Box
   * - P9 Witherspoon firmware level
     - **BMC:** ibm-v2.0-0-r46-0-gbed584c or higher **Host:** IBM-witherspoon-ibm-OP9_v1.19_1.185 or higher
     - Found on Box


.. note:: this seems wrong. ( P9 Witherspoon firmware level) What is BMC? what is host? should those be 2 seperate software values and not a 'version'?

Soft Dependencies
^^^^^^^^^^^^^^^^^