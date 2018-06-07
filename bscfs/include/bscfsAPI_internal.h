/******************************************************************************
 |    bscfsAPI_internal.h
 |
 |  © Copyright IBM Corporation 2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
 ******************************************************************************/

#ifndef BSCFS_BSCFSAPI_INTERNAL_H_
#define BSCFS_BSCFSAPI_INTERNAL_H_

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "bscfs_ioctl.h"

#ifdef __cplusplus
extern "C" {
#endif

static int __BSCFS_ControlFD = -2;

static inline int __BSCFS_CheckConnection()
{
    if (__BSCFS_ControlFD >= 0) return 1;
    if (__BSCFS_ControlFD == -1) return 0;

    char *bscfs_path = getenv("BSCFS_MNT_PATH");
    if (bscfs_path == NULL) {
	fprintf(stderr, "Environment variable \"BSCFS_MNT_PATH\" not set\n");
	__BSCFS_ControlFD = -1;
	return 0;
    }
    char control_path[BSCFS_PATH_MAX];
    snprintf(control_path, BSCFS_PATH_MAX, "%s/%s",
	     bscfs_path, BSCFS_CONTROL_FILE_NAME);
    __BSCFS_ControlFD = open(control_path, O_RDONLY, 0);
    if (__BSCFS_ControlFD < 0) {
	fprintf(stderr, "Could not open BSCFS control file \"%s\": %s\n",
		control_path, strerror(errno));
	__BSCFS_ControlFD = -1;
	return 0;
    }
    return 1;
}

// These routines pass various path names to bscfsAgent via ioctls. We don't
// check the string lengths here but instead let bscfsAgent do that. If a name
// is too long, strncpy() will truncate it and will not terminate it with a
// NULL, allowing bscfsAgent to detect the truncation and return ENAMETOOLONG.

static inline int __BSCFS_StartLocalFlush(const char *pathname,
					  const char *mapfile,
					  const char *cleanup_script,
					  BBTransferHandle_t *handle_p)
{
    if (!__BSCFS_CheckConnection()) return ENOSYS;

    if (pathname == NULL) return EFAULT;

    if (mapfile == NULL) mapfile = "";
    if (cleanup_script == NULL) cleanup_script = "";

    struct bscfs_setup_local_flush request;
    request.return_code = 0;
    request.handle = 0;
    strncpy(request.pathname, pathname, BSCFS_PATH_MAX);
    strncpy(request.mapfile, mapfile, BSCFS_PATH_MAX);
    strncpy(request.cleanup_script, cleanup_script, BSCFS_PATH_MAX);

    int rc = ioctl(__BSCFS_ControlFD, BSCFS_IOC_START_LOCAL_FLUSH, &request);
    if (rc < 0) return ENOSYS;

    if (request.return_code != 0) return request.return_code;

    if (handle_p != NULL) (*handle_p) = (BBTransferHandle_t) request.handle;
    return 0;
}

static inline int __BSCFS_PrepareLocalFlush(const char *pathname,
					    const char *mapfile,
					    const char *cleanup_script)
{
    if (!__BSCFS_CheckConnection()) return ENOSYS;

    if (pathname == NULL) return EFAULT;

    if (mapfile == NULL) mapfile = "";
    if (cleanup_script == NULL) cleanup_script = "";

    struct bscfs_setup_local_flush request;
    request.return_code = 0;
    request.handle = 0;
    strncpy(request.pathname, pathname, BSCFS_PATH_MAX);
    strncpy(request.mapfile, mapfile, BSCFS_PATH_MAX);
    strncpy(request.cleanup_script, cleanup_script, BSCFS_PATH_MAX);

    int rc = ioctl(__BSCFS_ControlFD, BSCFS_IOC_PREPARE_LOCAL_FLUSH, &request);
    if (rc < 0) return ENOSYS;

    if (request.return_code != 0) return request.return_code;

    return 0;
}

static inline int __BSCFS_StartLocalPrefetch(const char *pathname,
					     const char *mapfile,
					     BBTransferHandle_t *handle_p)
{
    if (!__BSCFS_CheckConnection()) return ENOSYS;

    if (pathname == NULL) return EFAULT;
    if (mapfile == NULL) return EFAULT;

    struct bscfs_start_local_prefetch request;
    request.return_code = 0;
    request.handle = 0;
    strncpy(request.pathname, pathname, BSCFS_PATH_MAX);
    strncpy(request.mapfile, mapfile, BSCFS_PATH_MAX);

    int rc = ioctl(__BSCFS_ControlFD, BSCFS_IOC_START_LOCAL_PREFETCH, &request);
    if (rc < 0) return ENOSYS;

    if (request.return_code != 0) return request.return_code;

    if (handle_p != NULL) (*handle_p) = (BBTransferHandle_t) request.handle;
    return 0;
}

static inline int __BSCFS_AwaitLocalTransfer(BBTransferHandle_t handle)
{
    if (!__BSCFS_CheckConnection()) return ENOSYS;

    struct bscfs_check_local_transfer request;
    request.return_code = 0;
    request.handle = (uint64_t) handle;

    for (;;) {
	int rc = ioctl(__BSCFS_ControlFD, BSCFS_IOC_CHECK_LOCAL_TRANSFER,
		       &request);
	if (rc < 0) return ENOSYS;

	if (request.return_code == 0) break;

	if (request.return_code != EAGAIN) return request.return_code;

	sleep(1);
    }

    return 0;
}

static inline int __BSCFS_GlobalFlushCompleted(const char *pathname)
{
    if (!__BSCFS_CheckConnection()) return ENOSYS;

    if (pathname == NULL) return EFAULT;

    struct bscfs_global_flush_completed request;
    request.return_code = 0;
    strncpy(request.pathname, pathname, BSCFS_PATH_MAX);

    int rc = ioctl(__BSCFS_ControlFD, BSCFS_IOC_GLOBAL_FLUSH_COMPLETED,
		   &request);
    if (rc < 0) return ENOSYS;

    return request.return_code;
}

static inline int __BSCFS_Forget(const char *pathname)
{
    if (!__BSCFS_CheckConnection()) return ENOSYS;

    if (pathname == NULL) return EFAULT;

    struct bscfs_forget request;
    request.return_code = 0;
    strncpy(request.pathname, pathname, BSCFS_PATH_MAX);

    int rc = ioctl(__BSCFS_ControlFD, BSCFS_IOC_FORGET, &request);
    if (rc < 0) return ENOSYS;

    return request.return_code;
}

static inline int __BSCFS_QueryInternalFiles(const char *pathname,
					     size_t files_length,
					     char *files,
					     int *state_p)
{
    if (!__BSCFS_CheckConnection()) return ENOSYS;

    if (pathname == NULL) return EFAULT;

    if (files_length > BSCFS_INTERNAL_FILES_LENGTH_MAX) return ENOSPC;

    struct bscfs_install_internal_files request;
    request.return_code = 0;
    request.state = 0;
    request.files_length = files_length;
    strncpy(request.pathname, pathname, BSCFS_PATH_MAX);
    memset(request.files, 0, files_length);

    int rc = ioctl(__BSCFS_ControlFD, BSCFS_IOC_QUERY_INTERNAL_FILES,
		   &request);
    if (rc < 0) return ENOSYS;

    if (request.return_code != 0) return request.return_code;

    if (state_p != NULL) (*state_p) = (int) request.state;
    if (files != NULL) memcpy(files, request.files, files_length);

    return 0;
}

static inline int __BSCFS_InstallInternalFiles(const char *pathname,
					       size_t files_length,
					       const char *files,
					       int state)
{
    if (!__BSCFS_CheckConnection()) return ENOSYS;

    if (pathname == NULL) return EFAULT;
    if (files == NULL) return EFAULT;

    if (files_length > BSCFS_INTERNAL_FILES_LENGTH_MAX) return ENOSPC;

    struct bscfs_install_internal_files request;
    request.return_code = 0;
    request.state = (uint64_t) state;
    request.files_length = files_length;
    strncpy(request.pathname, pathname, BSCFS_PATH_MAX);
    memcpy(request.files, files, files_length);

    int rc = ioctl(__BSCFS_ControlFD, BSCFS_IOC_INSTALL_INTERNAL_FILES,
		   &request);
    if (rc < 0) return ENOSYS;

    return request.return_code;
}

#ifdef __cplusplus
}
#endif

#endif /* BSCFS_BSCFSAPI_INTERNAL_H_ */
