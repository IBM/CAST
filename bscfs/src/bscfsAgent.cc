/*******************************************************************************
 |    bscfsAgent.cc
 |
 |  © Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/
#ifndef FUSE_USE_VERSION
#define FUSE_USE_VERSION 28
#endif

#include <pthread.h>
#include <fuse/fuse_opt.h>
#include <exception>
#include "logging.h"
#include "util.h"
#include "bscfsInternals.h"
#include "bscfsAgent_flightlog.h"

//user data initialized through command line arguments and used in operations
bscfs_data_t bscfs_data;

// fuse_opt structure for parsing command line arguments
static struct fuse_opt bscfs_opts[] =
{
    {"--pfs_path %s", offsetof(bscfs_data_t, pfs_path),
							FUSE_OPT_KEY_DISCARD },
    {"--bb_path %s", offsetof(bscfs_data_t, bb_path),
							FUSE_OPT_KEY_DISCARD },
    {"--pre_install_list %s", offsetof(bscfs_data_t, pre_install_list),
							FUSE_OPT_KEY_DISCARD },
    {"--cleanup_list %s", offsetof(bscfs_data_t, cleanup_list),
							FUSE_OPT_KEY_DISCARD },
    {"--max_index_size %lu", offsetof(bscfs_data_t, max_index_size),
							FUSE_OPT_KEY_DISCARD },
    {"--write_buffer_size %lu", offsetof(bscfs_data_t, write_buffer_size),
							FUSE_OPT_KEY_DISCARD },
    {"--read_buffer_size %lu", offsetof(bscfs_data_t, read_buffer_size),
							FUSE_OPT_KEY_DISCARD },
    {"--data_falloc_size %lu", offsetof(bscfs_data_t, data_falloc_size),
							FUSE_OPT_KEY_DISCARD },
    {"--config %s", offsetof(bscfs_data_t, config_file),
							FUSE_OPT_KEY_DISCARD},
    {"--node_number %lu", offsetof(bscfs_data_t, node_number),
							FUSE_OPT_KEY_DISCARD },
    {"--node_count %lu", offsetof(bscfs_data_t, node_count),
							FUSE_OPT_KEY_DISCARD },
    FUSE_OPT_END
};

// binding between fuse operations and bbfs operations
static struct fuse_operations bscfs_operations = {
    bscfs_getattr,    // int (*getattr) (const char *, struct stat *);
    bscfs_readlink,   // int (*readlink) (const char *, char *, size_t);
    NULL,             // int (*getdir) (const char *, fuse_dirh_t,
		      //                fuse_dirfil_t);
    bscfs_mknod,      // int (*mknod) (const char *, mode_t, dev_t);
    bscfs_mkdir,      // int (*mkdir) (const char *, mode_t);
    bscfs_unlink,     // int (*unlink) (const char *);
    bscfs_rmdir,      // int (*rmdir) (const char *);
    bscfs_symlink,    // int (*symlink) (const char *, const char *);
    bscfs_rename,     // int (*rename) (const char *, const char *);
    bscfs_link,       // int (*link) (const char *, const char *);
    bscfs_chmod,      // int (*chmod) (const char *, mode_t);
    bscfs_chown,      // int (*chown) (const char *, uid_t, gid_t);
    bscfs_truncate,   // int (*truncate) (const char *, off_t);
    NULL,             // int (*utime) (const char *, struct utimbuf *);
    bscfs_open,       // int (*open) (const char *, struct fuse_file_info *);
    bscfs_read,       // int (*read) (const char *, char *, size_t, off_t,
		      //              struct fuse_file_info *);
    bscfs_write,      // int (*write) (const char *, const char *, size_t,
		      //               off_t, struct fuse_file_info *);
    bscfs_statfs,     // int (*statfs) (const char *, struct statvfs *);
    bscfs_flush,      // int (*flush) (const char *, struct fuse_file_info *);
    bscfs_release,    // int (*release) (const char *, struct fuse_file_info *);
    bscfs_fsync,      // int (*fsync) (const char *, int,
		      //               struct fuse_file_info *);
    bscfs_setxattr,   // int (*setxattr) (const char *, const char *,
		      //                  const char *, size_t, int);
    bscfs_getxattr,   // int (*getxattr) (const char *, const char *, char *,
		      //                  size_t);
    bscfs_listxattr,  // int (*listxattr) (const char *, char *, size_t);
    bscfs_removexattr,// int (*removexattr) (const char *, const char *);
    bscfs_opendir,    // int (*opendir) (const char *, struct fuse_file_info *);
    bscfs_readdir,    // int (*readdir) (const char *, void *, fuse_fill_dir_t,
		      //                 off_t, struct fuse_file_info *);
    bscfs_releasedir, // int (*releasedir) (const char *,
		      //                    struct fuse_file_info *);
    bscfs_fsyncdir,   // int (*fsyncdir) (const char *, int,
		      //                  struct fuse_file_info *);
    bscfs_init,       // void *(*init) (struct fuse_conn_info *conn);
    bscfs_destroy,    // void (*destroy) (void *);
    bscfs_access,     // int (*access) (const char *, int);
    bscfs_create,     // int (*create) (const char *, mode_t,
		      //                struct fuse_file_info *);
    bscfs_ftruncate,  // int (*ftruncate) (const char *, off_t,
		      //                   struct fuse_file_info *);
    bscfs_fgetattr,   // int (*fgetattr) (const char *, struct stat *,
		      //                  struct fuse_file_info *);
    NULL,             // int (*lock) (const char *, struct fuse_file_info *,
		      //              int cmd, struct flock *);
    bscfs_utimens,    // int (*utimens) (const char *,
		      //                 const struct timespec tv[2]);
    bscfs_bmap,       // int (*bmap) (const char *, size_t blocksize,
		      //              uint64_t *idx);
    1,                // unsigned int flag_nullpath_ok:1;
    1,                // unsigned int flag_nopath:1;
    1,                // unsigned int flag_utime_omit_ok:1;
    29,               // unsigned int flag_reserved:29;
    bscfs_ioctl,      // int (*ioctl) (const char *, int cmd, void *arg,
		      //               struct fuse_file_info *,
		      //               unsigned int flags, void *data);
    bscfs_poll,       // int (*poll) (const char *, struct fuse_file_info *,
		      //              struct fuse_pollhandle *ph,
		      //              unsigned *reventsp);
    NULL,             // int (*write_buf) (const char *,
		      //                   struct fuse_bufvec *buf, off_t off,
		      //                   struct fuse_file_info *);
    NULL,             // int (*read_buf) (const char *,
		      //                  struct fuse_bufvec **bufp,
		      //                  size_t size, off_t off,
		      //                  struct fuse_file_info *);
    NULL,             // int (*flock) (const char *, struct fuse_file_info *,
		      //               int op);
    NULL,             // int (*fallocate) (const char *, int, off_t,
		      //                   off_t,struct fuse_file_info *);
};

int main (int argc, char **argv)
{
    int fuse_res;
    char *fs_path = NULL;
    struct fuse_args args = FUSE_ARGS_INIT(argc, argv);
    bscfs_data.bb_path = NULL;
    bscfs_data.pfs_path = NULL;
    bscfs_data.config_file = NULL;
    bscfs_data.pre_install_list = NULL;
    bscfs_data.cleanup_list = NULL;
    bscfs_data.write_buffer_size = 2ull * 1024ull * 1024ull;
    bscfs_data.read_buffer_size = 2ull * 1024ull * 1024ull;
    bscfs_data.data_falloc_size = 2ull * 1024ull * 1024ull;
    bscfs_data.max_index_size = 4ull * 1024ull * 1024ull * 1024ull;
    bscfs_data.node_number = 0;
    bscfs_data.node_count = 1;
    // parse command line arguments specific to bbfs
    if (fuse_opt_parse(&args, &bscfs_data, bscfs_opts, 0 ) == -1) {
	fprintf(stderr, "Could not parse command line options\n");
	return -1;
    }

    // if mount point, bb_path, pfs_path, or config_file not provided, abort
    if ((argc < 2) ||
	(bscfs_data.pfs_path == NULL) ||
	(bscfs_data.bb_path == NULL) ||
	(bscfs_data.config_file == NULL))
    {
	fprintf(stderr,
		"Must provide the mount point and paths to the parallel\n"
		"file system, to the Burst Buffer file system, and to\n"
		"the configuration file:\n"
		"\n"
		"%s\n"
		"    <mount_point>\n"
		"    --pfs_path <path>\n"
		"    --bb_path <path>\n"
		"    --config <file>\n"
		"    [ --pre_install_list <file> ]\n"
		"    [ --cleanup_list <file> ]\n"
		"    [ --write_buffer_size <bytes> ]\n"
		"    [ --read_buffer_size <bytes> ]\n"
		"    [ --data_falloc_size <bytes> ]\n"
		"    [ --max_index_size <bytes> ]\n"
		"    [ --node_number <number> ]\n"
		"    [ --node_count <number> ]\n",
		argv[0]);
	return -1;
    }

    // 1st argument is always the mount point when calling a fuse binary
    bscfs_data.mount_path  = realpath(argv[1], NULL);
    if (bscfs_data.mount_path == NULL) {
	fprintf(stderr, "Mount point \"%s\" not found: %s\n",
		argv[1], strerror(errno));
	return -1;
    }

    // get the real full path to the pfs
    fs_path = realpath(bscfs_data.pfs_path, NULL);
    if (fs_path == NULL) {
	fprintf(stderr, "pfs_path \"%s\" not found: %s\n",
		bscfs_data.pfs_path, strerror(errno));
	return -1;
    }
    free(bscfs_data.pfs_path);
    bscfs_data.pfs_path = fs_path;
    bscfs_data.pfs_path_len = strlen(fs_path);
    if (bscfs_data.pfs_path_len >= BSCFS_PATH_MAX) {
	fprintf(stderr, "pfs_path \"%s\" too long\n", fs_path);
	return -1;
    }

    // get the real full path to the BB
    fs_path = realpath(bscfs_data.bb_path, NULL);
    if (fs_path == NULL) {
	fprintf(stderr, "bb_path \"%s\" not found: %s\n",
		bscfs_data.bb_path, strerror(errno));
	return -1;
    }
    free(bscfs_data.bb_path);
    bscfs_data.bb_path = fs_path;
    bscfs_data.bb_path_len = strlen(fs_path);
    if (bscfs_data.bb_path_len >= BSCFS_PATH_MAX) {
	fprintf(stderr, "bb_path \"%s\" too long\n", fs_path);
	return -1;
    }

    // Determine the longest path (starting at the bscfs mount point) that we
    // can handle. We concatenate paths with pfs_path to form PFS path names,
    // and we concatenate them with bb_path (along with a ".data" or ".index"
    // suffix) to form data and index file names. Use whichever constraint is
    // more restrictive. Leave room for the null terminator.
    bscfs_data.max_path_len = BSCFS_PATH_MAX - bscfs_data.pfs_path_len - 1;
    if (bscfs_data.max_path_len > (BSCFS_PATH_MAX - bscfs_data.bb_path_len - 7))
	bscfs_data.max_path_len = BSCFS_PATH_MAX - bscfs_data.bb_path_len - 7;

    // get the real full path to the configuration file
    fs_path = realpath(bscfs_data.config_file, NULL);
    if (fs_path == NULL) {
	fprintf(stderr, "config_file \"%s\" not found: %s\n",
		bscfs_data.config_file, strerror(errno));
	return -1;
    }
    free(bscfs_data.config_file);
    bscfs_data.config_file = fs_path;

    // get the real full path to the pre_install_list file, if any
    if (bscfs_data.pre_install_list != NULL) {
	fs_path = realpath(bscfs_data.pre_install_list, NULL);
	if (fs_path == NULL) {
	    fprintf(stderr, "pre_install_list \"%s\" not found: %s\n",
		    bscfs_data.pre_install_list, strerror(errno));
	    return -1;
	}
	free(bscfs_data.pre_install_list);
	bscfs_data.pre_install_list = fs_path;
    } else {
	bscfs_data.pre_install_list = strdup("");
    }

    // create the cleanup_list file, if any, and get its real full path
    if (bscfs_data.cleanup_list != NULL) {
	int fd = creat(bscfs_data.cleanup_list, S_IRUSR|S_IWUSR);
	if (fd < 0) {
	    fprintf(stderr,
		    "could not create cleanup_list \"%s\": %s\n",
		    bscfs_data.cleanup_list, strerror(errno));
	    return -1;
	}
	close(fd);
	fs_path = realpath(bscfs_data.cleanup_list, NULL);
	if (fs_path == NULL) {
	    fprintf(stderr, "cleanup_list \"%s\" not found: %s\n",
		    bscfs_data.cleanup_list, strerror(errno));
	    return -1;
	}
	free(bscfs_data.cleanup_list);
	bscfs_data.cleanup_list = fs_path;
    } else {
	bscfs_data.cleanup_list = strdup("");
    }

    // round data_falloc_size up to be a multiple of write_buffer_size
    // (it can be zero)
    bscfs_data.data_falloc_size =
	((bscfs_data.data_falloc_size + bscfs_data.write_buffer_size - 1) /
	    bscfs_data.write_buffer_size) * bscfs_data.write_buffer_size;

    pthread_mutex_init(&bscfs_data.shared_files_lock,NULL);
    bscfs_data.shared_files.clear();
    // We embed the node number in the starting tag value to make tags
    // unique across the job. This is not strictly necessary, but it
    // facilitates testing of multiple bscfsAgents on a single node.
    // We also start at 100 rather than 0 to avoid conflicting with tags
    // generated by the stagein_user_bscfs script.
    bscfs_data.next_transfer_tag = (bscfs_data.node_number << 48) + 100;

    //setting the logging system
    if (!curConfig.load(bscfs_data.config_file)) {
	config = curConfig.getTree();
    } else {
	fprintf(stderr, "Error loading configuration\n");
	exit(-1);
    }
    std::string filelogpath = config.get("bb.bscfsagent.log.fileLog", "none");
    if(filelogpath == std::string("BBPATH"))
    {
        config.put("bb.bscfsagent.log.fileLog", std::string(bscfs_data.bb_path) + std::string("/bscfs.log"));
    }
    initializeLogging("bb.bscfsagent.log", config);
    LOG(bscfsagent,info)
	<< "\nbscfsAgent starting, with these parameters:"
	<< "\n    Mount point        --> " << bscfs_data.mount_path
	<< "\n    PFS path           --> " << bscfs_data.pfs_path
	<< "\n    BB path            --> " << bscfs_data.bb_path
	<< "\n    Configuration file --> " << bscfs_data.config_file
	<< "\n    Pre-install list   --> " << bscfs_data.pre_install_list
	<< "\n    Cleanup list       --> " << bscfs_data.cleanup_list
	<< "\n    Write buffer size  --> " << bscfs_data.write_buffer_size
	<< "\n    Read buffer size   --> " << bscfs_data.read_buffer_size
	<< "\n    Data falloc size   --> " << bscfs_data.data_falloc_size
	<< "\n    Max index size     --> " << bscfs_data.max_index_size
	<< "\n    Node number        --> " << bscfs_data.node_number
	<< "\n    Node count         --> " << bscfs_data.node_count
	<< "\n";

    std::string fl;
    try {
	fl = config.get("bb.bscfsagent.flightlog", "none");
    }
    catch(std::exception& e)
    {
	LOG(bscfsagent,error) << "Exception getting flightlog config: " << e.what();
	exit(-1);
    }
    int rc = FL_CreateAll(fl.c_str());
    if (rc != 0) {
	LOG(bscfsagent,error)
	    << "Unable to initialize flightlog (" << fl << ") rc = " << rc;
	exit(-1);
    }
    FL_Write(FLAgent, BSCFS_AgentStart, "bscfsAgent started", 0,0,0,0);

    // open the mount point for chdir inside once fuse is mounted (otherwise,
    // fuse believes path are root based)
    // preferable to open here rather than the fuse_init section to detect
    // potential error earlier
    bscfs_data.mount_fd = open(bscfs_data.mount_path, O_RDONLY, 0);
    if (bscfs_data.mount_fd < 0) {
	// could not open the mount point: abort
	LOG(bscfsagent,error)
	    << "bscfsAgent could not open mount point "
	    << bscfs_data.mount_path << ": " << strerror(errno)
	    << "; BSCFS exiting!";
	exit(-1);
    }

    // finally call the fuse mount functions
    fuse_res = fuse_main(args.argc, args.argv, &bscfs_operations, NULL);
    return fuse_res;
}
