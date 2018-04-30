# How to Contribute

This is an open source project, and we appreciate your help!

In order to clarify the intellectual property license granted with Contributions from any person or entity made for the benefit of the CAST Community, a Contributor License Agreement ("CLA") must be on file that has been signed by each Contributor, indicating agreement to the license terms below. This license is for your protection as a Contributor as well as the protection of the CAST Community and its users; it does not change your rights to use your own Contributions for any other purpose.

Please print, fill out, and sign one of the following: 

 * Corporate Contributor License Agreement: [cast-project-cla-corporate.pdf](https://github.com/IBM/CAST/blob/cast_1.0.0/cast-project-cla-corporate.pdf) 
 * Individual Contributor License Agreement: [cast-project-cla-individual.pdf](https://github.com/IBM/CAST/blob/cast_1.0.0/cast-project-cla-individual.pdf)

Once completed, scan the document as a PDF file and email to the following email addresses: tgooding@us.ibm.com, pizzano@us.ibm.com 

Thank you for your interest in the Cluster Administration and Storage Tools (CAST) project!

# Build Machine Software Requirements

The following are the minimum recommended levels of software for a build machine.

- Red Hat Enterprise Linux 7.5 for IBM Power LE (Power 9)
- libibverbs-devel-41mlnx1-OFED (provides `<infiniband/verbs.h>`)
- cmake 3.7.1
- boost 1.55.0

# Build Instructions

1. Fork or clone the CAST project  
2. Install dependency rpms: 

   ```
   yum -y install cmake gcc* glib* openssl* fuse* doxygen* postgres* libuuid-devel perl* pam-devel rpm*
   ```
3. Obtain and install the [cast-boost RPMs](https://ibm.ent.box.com/s/xz86ufe0pwvzwe6wnjfb88g846txe3hb/folder/48985700581)
4. Run the `prereq.pl` script to update cmake and boost libraries 

   ```
   cd CAST
   scripts/prereq.pl
   ```

5. Build the code and generate RPM output files:

   ```
   scripts/configure.pl --rpmbuild --parallel
   . SETUP.sh
   build package
   ```

   To find more options for building, run `build help`


* The default install directory is CAST/work
* The default build directory is CAST/.build 

# How to contribute to the project

1. Sign a CLA or CCLA 
2. Fork the project and create pull requests!

# Location of Binaries

The CAST binary distribution can be found in [BOX Folder](https://ibm.box.com/s/xz86ufe0pwvzwe6wnjfb88g846txe3hb)
