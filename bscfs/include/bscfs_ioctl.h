/******************************************************************************
 |    bscfs_ioctl.h
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

#ifndef BSCFS_IOCTL_H
#define BSCFS_IOCTL_H

#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/param.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BSCFS_CONTROL_FILE_NAME "bscfs_control"
#define BSCFS_PATH_MAX 2048

typedef enum bscfs_file_state {
    BSCFS_INACTIVE,
    BSCFS_MODIFIED,
    BSCFS_FLUSH_PENDING,
    BSCFS_FLUSHING,
    BSCFS_FLUSH_COMPLETED,
    BSCFS_PREFETCHING,
    BSCFS_STABLE,

    BSCFS_CONTROL // artificial state just for the control file
} bscfs_file_state_t;

/*
 * StartLocalFlush and PrepareLocalFlush use the same structure.
 */
struct bscfs_setup_local_flush {
	uint64_t return_code;			// out
	uint64_t handle;			// out
	char pathname[BSCFS_PATH_MAX];		// in
	char mapfile[BSCFS_PATH_MAX];		// in
	char cleanup_script[BSCFS_PATH_MAX];	// in
};

struct bscfs_start_local_prefetch {
	uint64_t return_code;			// out
	uint64_t handle;			// out
	char pathname[BSCFS_PATH_MAX];		// in
	char mapfile[BSCFS_PATH_MAX];		// in
};

struct bscfs_check_local_transfer {
	uint64_t return_code;			// out
	uint64_t handle;			// in
};

struct bscfs_global_flush_completed {
	uint64_t return_code;			// out
	char pathname[BSCFS_PATH_MAX];		// in
};

struct bscfs_forget {
	uint64_t return_code;			// out
	char pathname[BSCFS_PATH_MAX];		// in
};

/*
 * To simplify things, we set an upper bound on the combined lengths of the
 * pathnames of BSCFS internal files. We allow space for 2 full pathnames plus
 * the extra terminating NULL byte.
 */
#define BSCFS_INTERNAL_FILES_LENGTH_MAX ((2 * BSCFS_PATH_MAX) + 1)

struct bscfs_query_internal_files {
	uint64_t return_code;				// out
	uint64_t state;					// out
	uint64_t files_length;				// in
	char pathname[BSCFS_PATH_MAX];			// in
	char files[BSCFS_INTERNAL_FILES_LENGTH_MAX];	// out
};

struct bscfs_install_internal_files {
	uint64_t return_code;				// out
	uint64_t state;					// in
	uint64_t files_length;				// in
	char pathname[BSCFS_PATH_MAX];			// in
	char files[BSCFS_INTERNAL_FILES_LENGTH_MAX];	// in
};

struct bscfs_get_parameter {
	uint64_t return_code;				// out
	char parameter[BSCFS_PATH_MAX];		// in
	char value[256];					// out
};

#define BSCFS_IOC_START_LOCAL_FLUSH \
	_IOWR('B', 1, struct bscfs_setup_local_flush)

#define BSCFS_IOC_PREPARE_LOCAL_FLUSH \
	_IOWR('B', 2, struct bscfs_setup_local_flush)

#define BSCFS_IOC_START_LOCAL_PREFETCH \
	_IOWR('B', 3, struct bscfs_start_local_prefetch)

#define BSCFS_IOC_CHECK_LOCAL_TRANSFER \
	_IOWR('B', 4, struct bscfs_check_local_transfer)

#define BSCFS_IOC_GLOBAL_FLUSH_COMPLETED \
	_IOWR('B', 5, struct bscfs_global_flush_completed)

#define BSCFS_IOC_FORGET \
	_IOWR('B', 6, struct bscfs_forget)

#define BSCFS_IOC_QUERY_INTERNAL_FILES \
	_IOWR('B', 7, struct bscfs_query_internal_files)

#define BSCFS_IOC_INSTALL_INTERNAL_FILES \
	_IOWR('B', 8, struct bscfs_install_internal_files)

#define BSCFS_IOC_GET_PARAMETER \
	_IOWR('B', 9, struct bscfs_get_parameter)

#ifdef __cplusplus
}
#endif

#endif /* BSCFS_IOCTL_H */
