.. _CSM_INSTALLATION_AND_CONFIGURATION_uninstallation:

Uninstallation
==============

Stop all CSM daemons

.. code-block:: bash

  $ xdsh compute "systemctl stop csmd-compute"
  $ xdsh utility "systemctl stop csmd-utility"
  $ systemctl stop csmd-aggregator
  $ systemctl stop csmd-master


Delete CSMDB

.. code-block:: bash

  $ /opt/ibm/csm/db/csm_db_script.sh -d csmdb


Remove rpms

.. code-block:: bash

  $ xdsh compute,utility "rpm -e ibm-csm-core-1.5.0-*.ppc64le ibm-csm-api-1.5.0-*.ppc64le ibm-flightlog-1.5.0-*.ppc64le ibm-csm-hcdiag-1.5.0-*.noarch"

  $ rpm -e ibm-csm-core-1.5.0-*.ppc64le ibm-csm-hcdiag-1.5.0-*.noarch ibm-csm-db-1.5.0-*.noarch ibm-csm-api-1.5.0-*.ppc64le ibm-csm-restd-1.5.0-*.ppc64le ibm-flightlog-1.5.0-*.ppc64le


Clean up log and configuration files

.. code-block:: bash

  $ xdsh compute,utility "rm -rf /etc/ibm /var/log/ibm"
  $ rm -rf /etc/ibm/csm /var/log/ibm/csm


Stop NVIDIA host engine (DCGM)

.. code-block:: bash

  $ xdsh compute,service,utility "systemctl start nvidia-persistenced"
  $ xdsh compute,utility /usr/bin/nv-hostengine –t



