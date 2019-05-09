.. _CSM_INSTALLATION_AND_CONFIGURATION_appendices:

Appendices
==========

Diskless images
---------------

Please ensure the following steps have been completed on the xCAT Management node:

1. xCAT has been installed and the basic configuration has been completed.
2. Section 2.2 has been completed and the cast-boost rpms are currently accessible at ``${HOME}/rpmbuild/RPMS/ppc64le``.
3. The ``ibm-flightlog-1.5.0-*.ppc64le`` is present on the xCAT Management node.
4. Install ``createrepo`` for building the other packages directory.

After verifying the above steps have been completed do the following:

1. Generate the osimages for your version of red hat:

.. code-block:: bash

  $ copycds RHEL-7.5-Server-ppc64le-dvd1.iso
  $ lsdef –t osimage 


2. Copy the “netboot” image of the osimages created in the previous step and rename it:

.. code-block:: bash

  $ image_input="rhels7.5-ppc64le-netboot-compute"
  $ image_output="rhels7.5-ppc64le-diskless-compute"
  $ lsdef -t osimage -z $image_input | sed -e "s/$image_input/$image_output/g" | mkdef -z

3. Move the cast-boost rpms to the otherpkgdir directory for the generation of the diskless image:

.. code-block:: bash

  $ lsdef -t osimage rhels7.5-ppc64le-diskless-compute
  $ cp cast-boost* /install/post/otherpkgs/rhels7.5/ppc64le/cast-boost
  $ createrepo /install/post/otherpkgs/rhels7.5/ppc64le/cast-boost


4. Move the CSM rpms to the otherpkgdir directory:

.. code-block:: bash

  $ cp csm_rpms/* /install/post/otherpkgs/rhels7.5/ppc64le/csm
  $ createrepo /install/post/otherpkgs/rhels7.5/ppc64le/csm

5. Run creatrepo one last time:

.. code-block:: bash

  $ createrepo /install/post/otherpkgs/rhels7.5/ppc64le

6. Add the following to a package list in the otherpkgdir, then add the package list to the osimage:

.. code-block:: bash

  $ vi /install/post/otherpkgs/rhels7.5/ppc64le/csm.pkglist
	    cast-boost/cast-boost-*
        csm/ibm-flightlog-1.5.0-*.ppc64le
        csm/ibm-csm-core-1.5.0-*.ppc64le
        csm/ibm-csm-api-1.5.0-*.ppc64le

  $ chdef -t osimage rhels7.5-ppc64le-diskless-compute otherpkglist=/install/post/otherpkgs/rhels7.5/ppc64le/csm.pkglist


7. Generate and package the diskless image:

.. code-block:: bash

  $ genimage rhels7.5-ppc64le-diskless-compute
  $ packimage rhels7.5-ppc64le-diskless-compute


