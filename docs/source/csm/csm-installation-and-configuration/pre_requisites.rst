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
     - * **BMC:** ibm-v2.0-0-r46-0-gbed584c or higher 
       * **Host:** IBM-witherspoon-ibm-OP9_v1.19_1.185 or higher
     - Found on Box


.. note:: this seems wrong. ( P9 Witherspoon firmware level) What is BMC? what is host? should those be 2 seperate software values and not a 'version'?

Soft Dependencies
^^^^^^^^^^^^^^^^^

These dependencies are highly suggested.

.. list-table:: Soft Dependencies
   :widths: 25 25 50
   :header-rows: 1

   * - Software
     - Version
     - Comments
   * - NVIDIA DCGM
     - datacenter-gpu-manager-1.4.2-1.ppc64le
     - Needed by: 
            
       * Diagnostics and health check
       * CSM GPU inventory  
       * All nodes with GPUs. 
           
       `IBM Knowledge Center DCGM Page <https://www.ibm.com/support/knowledgecenter/en/SSWRJV_10.1.0/lsf_gpu/lsf_gpu_nvidia_dcgm_features.html>`_
   * - NVIDIA Cuda Toolkit
     - cuda-9.2.148-1.ppc64le
     - Needed by: 
            
       * All nodes with GPUs.
   * - NVIDIA Driver
     - cuda-drivers-396.47-1.ppc64le
     - Needed by: 
            
       * Needed by NVIDIA Data Center GPU Manager (DCGM).
       * All nodes with GPUs.
   * - IBM HTX 
     - htxrhel72le-491-LE.ppc64le
     - Needed by: 
            
       * Diagnostics and health check
       * All nodes.
            
       HTX requires:
            
       * net-tools package (ifconfig command)
       * mesa-libGLU-devel and mesa-libGLU packages
   * - Spectrum MPI
     - 10.02.00.05 or higher
     - Needed by the Diagnostic tests: 

       * dgemm
       * dgemm-gpu
       * jlink
       * daxpy tests
             
       Spectrum MPI requires:
             
       * IBM ESSL (IBM Engineering and Scientific Subroutine Libray)
       * IBM XL Fortran
   * - sudo
     - sudo-1.8.19p2-13.el7.ppc64le
     - Required by the Diagnostic’s tests that needs to run as root
   * - lm-sensors
     - 3.4.0
     - Required by the Diagnostic’s temperature tests.


Software with dependencies on CSM
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. list-table:: Dependencies on CSM
   :widths: 25 25 50
   :header-rows: 1

   * - Software
     - Version
     - Comments
   * - Spectrum LSF
     - 10.1
     -  
   * - Burst Buffer
     - 1.3.0
     - 
   * - IBM POWER HW Monitor
     - ibm-crassd-0.8-15 or higher
     - If installed on the service nodes, ibm-crassd can be configured to create CSM RAS events from BMC sources via csmrestd.

IBM Spectrum LSF (LSF), Burst Buffer (BB), and Job Step Manager (JSM) all have dependencies on CSM. Whenever a new version of CSM is installed, all dependent software must be restarted to reload the updated version of the CSM API library.

If any of these packages are not already installed or for more information about these packages, please refer to CORAL Software Overview (CORAL_Readme_v1.0.dox) located in Box. In this document you will find where these packages are located and how to install them. 




















