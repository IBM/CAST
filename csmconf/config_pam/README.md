# Pam.d Authorized Users Configuration: #

This directory contains a collection of scripts and templates for the configuration of the pam.d
daemon for use with the csm libraries. 

## Contents: ##

* [Installation](#installation)
* [config_pamd](#config_pamd)
* [assign_cgroup](#assign_cgroup)
* [default_sshd](#default_sshd)
* [build_config_pamd](#build_config_pamd)


## Installation ## 

To install this authorized user configuration please use the following steps as root:

0. Modify [default_sshd](#default_sshd) for your environment.
1. Run [build_config_pamd](#build_config_pamd) to rebuild [config_pamd](#config_pamd) if any files have changed.
   * Specify `-i` if the generated script should automatically replace the existing sshd pam config.
2. Execute [config_pamd](#config_pamd) on the desired nodes (`xdsh <group> -e config_pamd` 
    if on a managment node).

After running `config_pamd`, the [assign_cgroup](#assign_cgroup) script will now be added to 
`/etc/pam.d/cgroups/` and [assign_cgroup](#assign_cgroup) will be invoked when a user logs into the node.

## config_pamd ##

The `config_pamd` will modify the following pam.d components:

* `/etc/pam.d/sshd`

The `config_pamd` will create the following files by default:

* `/etc/pam.d/cgroups/`
* `/etc/pam.d/cgroups/assign_cgroup`
* `/etc/pam.d/cgroups/cgroup_exempt_list`

## assign_cgroup ##

A script for moving a user's sshd process to a collection of cgroups.
At the present the flow of this script is as follows:

0. Get the sshd pid for the current session. 
1. Determine if the user should be assigned to a cgroup.
2. Place the user into any allocation cgroups.

This script assumes that the user has already been verified.

**WARNING:** In Beta 2 this script does not support shared allocations.

## cgroup_exempt_users ##

A whitelist detailing users which shouldn't be placed into an allocation cgroup.

## default_sshd ## 

Configures how pam controls ssh authorizations, the default config adds whitelist support.

Default configuration reproduced below:

```    
    #%PAM-1.0
    auth       required     pam_sepermit.so
    auth       substack     password-auth
    auth       include      postlogin
    # Used with polkit to reauthorize users in remote sessions
    -auth      optional     pam_reauthorize.so prepare
    account    required     pam_nologin.so
    account    include      password-auth
    password   include      password-auth
    # pam_selinux.so close should be the first session rule
    session    required     pam_selinux.so close
    session    required     pam_loginuid.so
    # pam_selinux.so open should only be followed by sessions to be executed in the user context
    session    required     pam_selinux.so open env_params
    session    required     pam_namespace.so
    session    optional     pam_keyinit.so force revoke
    session    include      password-auth
    session    include      postlogin
    # Route the user to a cgroup, if this fails they will be put in the default cgroup.
    session    optional     pam_exec.so stdout seteuid CGROUP_ACCESS_SCRIPT
    # Used with polkit to reauthorize users in remote sessions
    -session   optional     pam_reauthorize.so prepare
```
**NOTE:** Any changes to this file requires [build_config_pamd](#build_config_pamd) to be run.

## build_config_pamd ## 

| Flags | Description                                                                 |
|-------|-----------------------------------------------------------------------------|
| -i    | The generated `config_pamd` script will install the sshd pam.d config file. |

**NOTE:** If *-i* is not specified instructions will be printed for finishing the configuration when executing `config_pamd`.

The `build_config_pamd` script should be run after changing any of the following files:

* `default_sshd`                # *LOCAL_DEFAULT_SSHD* in `build_config_pamd`
* `cgroup_exempt_users`         # *LOCAL_CGROUP_EXEMPT* in `build_config_pamd`

By default `build_config_pamd` will assume pam.d is configured in `/etc/pam.d`.

**WARNING:** If the pam.d location differs modify PAMD_DIR in `build_config_pamd`

This script will replace the following in the above files:

**default_sshd**
It will replace **CGROUP_ACCESS_SCRIPT** in default_sshd with **${CGROUP_DIR}${LOCAL_ASSIGN_CGROUP}**.

**assign_cgroup**
**PAM_WHITELIST** will be replaced with the absolute path to the whitelist (**${CGROUP_DIR}${CGROUP_EXEMPT}** by default).

