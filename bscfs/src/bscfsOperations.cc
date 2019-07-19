/*******************************************************************************
 |    bscfsOperations.cc
 |
 |  � Copyright IBM Corporation 2015,2016. All Rights Reserved
 |
 |    This program is licensed under the terms of the Eclipse Public License
 |    v1.0 as published by the Eclipse Foundation and available at
 |    http://www.eclipse.org/legal/epl-v10.html
 |
 |    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
 |    restricted by GSA ADP Schedule Contract with IBM Corp.
*******************************************************************************/

/*
 *  All file systems operations needed to be implemented for a full Fuse FS
 */

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/xattr.h>
#include <pthread.h>
#include <assert.h>
#include <linux/fuse.h>
#include <linux/falloc.h>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <aio.h>
#include "logging.h"
#include "bbfileflags.h"
#include "bscfsInternals.h"
#include "bscfsAgent_flightlog.h"

static inline void FL_sf_new(shared_file_t *sf)
{
    FL_Write(FLAgent, BSCFS_sf_new, "new shared_file_t, sf %p", (uint64_t) sf, 0,0,0);
}

static inline void FL_sf_delete(shared_file_t *sf)
{
    FL_Write(FLAgent, BSCFS_sf_delete, "delete shared_file_t, sf %p", (uint64_t) sf, 0,0,0);
}

static inline void FL_sf_lookup(shared_file_t *sf)
{
    FL_Write(FLAgent, BSCFS_sf_lookup, "shared_file_t lookup, sf %p", (uint64_t) sf, 0,0,0);
}

static inline void FL_sf_state(shared_file_t *sf)
{
    FL_Write(FLAgent, BSCFS_sf_state, "shared_file_t state change, sf %p, new state %d", (uint64_t) sf, sf->state, 0,0);
}

static void log_bb_error(int res, const char *bb_op)
{
    size_t needed = 0;
    size_t strsize = 4096;
    char *errstr = (char *) malloc(strsize);
    int rc = BB_GetLastErrorDetails(BBERRORJSON, &needed, strsize, errstr);
    if ((rc == 0) && (needed > strsize)) {
	errstr = (char *) realloc(errstr, needed);
	rc = BB_GetLastErrorDetails(BBERRORJSON, NULL, needed, errstr);
    }
    if (rc != 0) strcpy(errstr, "<not available>");
    LOG(bscfsagent,info)
	<< bb_op << " returned " << res << " ("
	<< ((res > 0) ? strerror(res) : "unknown error")
	<< "), failure details: " << errstr;
    free(errstr);
}

/* when provided a path to access a file from the bscfs mount point,
 * convert the path to the mirrored file in the PFS
 */
static char *bscfs_pfs_path(const char *path)
{
    uint64_t path_len = strnlen(path, BSCFS_PATH_MAX);
    if (path_len > bscfs_data.max_path_len) return NULL;
    uint64_t len = bscfs_data.pfs_path_len + path_len;
    char *rpath = (char *) malloc(len + 1); // leave room for null
    memcpy(rpath, bscfs_data.pfs_path, bscfs_data.pfs_path_len);
    memcpy(rpath + bscfs_data.pfs_path_len, path, path_len);
    rpath[len] = '\0';
    return rpath;
}

#define USE_INODE true

#if USE_INODE
static char* bscfs_data_path(shared_file_t* sf)
{
    size_t len = bscfs_data.bb_path_len + 32;
    char* rpath = (char*)malloc(len);
    snprintf(rpath, len, "%s/%016lx.data", bscfs_data.bb_path, sf->pfsstatcache.st_ino);
    return rpath;
}

static char* bscfs_index_path(shared_file_t* sf)
{
    size_t len = bscfs_data.bb_path_len + 32;
    char* rpath = (char*)malloc(len);
    snprintf(rpath, len, "%s/%016lx.index", bscfs_data.bb_path, sf->pfsstatcache.st_ino);
    return rpath;
}
#else
/* return the data file name to be associated with <path>
 *
 */
static char *bscfs_data_path(const char *path)
{
    uint64_t path_len = strnlen(path, BSCFS_PATH_MAX);
    if (path_len > bscfs_data.max_path_len) return NULL; // should never happen
    uint64_t len = bscfs_data.bb_path_len + path_len;
    char *rpath = (char *) malloc(len + 6); // leave room for suffix and null
    memcpy(rpath, bscfs_data.bb_path, bscfs_data.bb_path_len);
    memcpy(rpath + bscfs_data.bb_path_len, path, path_len);
    memcpy(rpath + len, ".data", 6);
    // change forward-slashes in path to commas
    for (char *p = (rpath+bscfs_data.bb_path_len+1); p < (rpath+len); p++) {
	if ((*p) == '/') (*p) = ',';
    }
    return rpath;
}

/* return the index file name to be associated with <path>
 *
 */
static char *bscfs_index_path(const char *path)
{
    uint64_t path_len = strnlen(path, BSCFS_PATH_MAX);
    if (path_len > bscfs_data.max_path_len) return NULL; // should never happen
    uint64_t len = bscfs_data.bb_path_len + path_len;
    char *rpath = (char *) malloc(len + 7); // leave room for suffix and null
    memcpy(rpath, bscfs_data.bb_path, bscfs_data.bb_path_len);
    memcpy(rpath + bscfs_data.bb_path_len, path, path_len);
    memcpy(rpath + len, ".index", 7);
    // change forward-slashes in path to commas
    for (char *p = (rpath+bscfs_data.bb_path_len+1); p < (rpath+len); p++) {
	if ((*p) == '/') (*p) = ',';
    }
    return rpath;
}
#endif

/* Return the shared_file_t that corresponds to <path> if it exists.
 * Otherwise return NULL. The shared_files_lock is assumed to be held.
 */
static shared_file_t *shared_file_lookup(const char *path)
{
    shared_file_t *sf = NULL;
    std::map<std::string, shared_file_t*>::iterator item;
    item = bscfs_data.shared_files.find(path);
    if (item != bscfs_data.shared_files.end()) {
	sf = item->second;
    }
    FL_sf_lookup(sf);
    return sf;
}

/* Return the shared_file_t whose transfer_handle matches <handle> if it
 * exists. Otherwise return NULL. The shared_files_lock is assumed to be held.
 */
static shared_file_t *shared_file_lookup_handle(BBTransferHandle_t handle)
{
    shared_file_t *sf = NULL;
    std::map<std::string, shared_file_t*>::iterator item;
    for (item = bscfs_data.shared_files.begin();
	 item != bscfs_data.shared_files.end();
	 item++)
    {
	if (item->second->transfer_handle == handle) {
	    // found it!
	    sf = item->second;
	    break;
	}
    }
    FL_sf_lookup(sf);
    return sf;
}

static int cmp_sf_offset(const void *m1, const void *m2)
{
    bscfs_mapping_t *map1 = (bscfs_mapping_t *) m1;
    bscfs_mapping_t *map2 = (bscfs_mapping_t *) m2;

    return (map1->sf_offset < map2->sf_offset) ? -1 :
	    ((map1->sf_offset > map2->sf_offset) ? 1 : 0);
}

static int cmp_df_offset(const void *m1, const void *m2)
{
    bscfs_mapping_t *map1 = (bscfs_mapping_t *) m1;
    bscfs_mapping_t *map2 = (bscfs_mapping_t *) m2;

    return (map1->df_offset < map2->df_offset) ? -1 :
	    ((map1->df_offset > map2->df_offset) ? 1 : 0);
}

static void bscfs_index_normalize(bscfs_index_t *index, uint64_t max_index_size)
{
    bscfs_mapping_t *M, *overflow, *s, t;
    uint64_t mapping_count_max, end, trim;
    int64_t overflow_count, next, keep, i, j, k;

    // Start by sorting mappings into sf_offset order.
    qsort(index->mapping, index->mapping_count,
	  sizeof(bscfs_mapping_t), cmp_sf_offset);
    index->finalized = 0; // mappings may not be in df_offset order now

    M = index->mapping; // convenience variable
    mapping_count_max = BSCFS_INDEX_MAPPING_COUNT_MAX(max_index_size);

    // Place a sentinel mapping at the end of the array so we do not have to
    // check for the end in various places below. Just fail if we are so
    // close to the limit that there is no room for the sentinel.
    if (index->mapping_count >= mapping_count_max) goto index_too_large;
    M[index->mapping_count].sf_offset = UINT64_MAX; // sentinel

    // In general, we plan to copy mappings down in the mappings array as we
    // eliminate overlaps, but it's possible that we generate MORE mappings
    // than we've looked at so far. We put them aside in an overflow array
    // and merge them back in at the end. In all likelihood we'll use very
    // little of the overflow space, but in a pathological case we could need
    // as many entries as there are currently in the mapping array.
    overflow = (bscfs_mapping_t *)
	malloc(index->mapping_count * sizeof(bscfs_mapping_t));
    if (overflow == NULL) goto index_too_large;
    overflow_count = 0;

    next = 0; // next established mapping goes here
    i = 0; // current mapping being considered
    while (i < (int64_t) index->mapping_count) {
	// Look ahead through all mappings that start at the same sf_offset
	// as the current mapping. Find the one with the largest df_offset,
	// At least an initial segment of that mapping will be established.
	j = i;
	for (k = i+1; M[k].sf_offset == M[i].sf_offset; k++) {
	    if (M[k].df_offset > M[j].df_offset) j = k;
	}
	// j now identifies the target mapping.
	// k now identifies the first mapping with a strictly larger sf_offset.

	// Look further ahead for a mapping that overlaps the target mapping
	// AND has a larger df_offset, i.e. a mapping that supplants part
	// of the target mapping.
	t = M[j]; // preserve the target mapping.
	end = t.sf_offset + t.length;
	while ((M[k].sf_offset < end) && (M[k].df_offset < t.df_offset)) k++;
	if (M[k].sf_offset < end) {
	    // We found a supplanting mapping. Truncate the target mapping.
	    end = M[k].sf_offset;
	    t.length = end - t.sf_offset;
	}

	// Walk backwards through the mappings that are at least partially
	// covered by the target mapping, and trim them forward to the end
	// of the target. Any that extend beyond the end (possibly including
	// the original target itself) are kept for consideration in the next
	// iteration. All others are dropped.
	keep = k;
	for (j = k-1; j >= i; j--) {
	    if ((M[j].sf_offset + M[j].length) > end) {
		keep--;
		trim = end - M[j].sf_offset;
		M[keep].sf_offset = M[j].sf_offset + trim;
		M[keep].df_offset = M[j].df_offset + trim;
		M[keep].length = M[j].length - trim;
	    }
	}
	i = keep;

	// Copy the established mapping down in the array (if there's room)
	// or into the overflow array.
	s = (next < i) ? &M[next++] : &overflow[overflow_count++];
	*s = t;
    }

    // Merge the overflow mappings into the mapping array.
    index->mapping_count = next + overflow_count;
    if (index->mapping_count > mapping_count_max) {
	free(overflow); // to keep clang happy
	goto index_too_large;
    }
    i = index->mapping_count;
    while ((overflow_count > 0) && (next > 0)) {
	s = (M[next-1].sf_offset > overflow[overflow_count-1].sf_offset) ?
		&M[--next] : &overflow[--overflow_count];
	M[--i] = *s;
    }
    while (overflow_count > 0) M[--i] = overflow[--overflow_count];

    free(overflow);
    return;

index_too_large:
    LOG(bscfsagent,error)
	<< "max_index_size of " << max_index_size
	<< " bytes exceeded; BSCFS exiting!";
    exit(-1);
}

static void bscfs_index_finalize(bscfs_index_t *index, uint64_t max_index_size)
{
    // First, normalize the index to remove overlaps.
    bscfs_index_normalize(index, max_index_size);

    // Then sort the mappings back into df_offset order.
    qsort(index->mapping, index->mapping_count,
	  sizeof(bscfs_mapping_t), cmp_df_offset);
    index->normalized = 0; // mappings may not be in sf_offset order now
}

/*
 * Return the entry number of the mapping that contains offset, or, if there
 * is no such mapping, the mapping that precedes offset. Return -1 if
 * offset is smaller than any existing mapping.
 */
static int64_t bscfs_index_lookup(bscfs_index_t *index, uint64_t offset)
{
    int64_t base = index->mapping_count;
    int64_t delta = base + 1;
    while (delta > 1) {
	delta = (delta + 1) / 2;
	int64_t next = base - delta;
	if ((next >= 0) &&
	    (offset < index->mapping[next].sf_offset)) base = next;
    }
    return base - 1;
}

static void bscfs_index_convert_host_to_little_endian(bscfs_index_t *index)
{
    if (htole64(0x0123456789abcdefull) == 0x0123456789abcdefull) return;

    uint64_t i;
    for (i = 0; i < index->mapping_count; i++) {
	bscfs_mapping_t *m = &index->mapping[i];
	m->sf_offset = htole64(m->sf_offset);
	m->df_offset = htole64(m->df_offset);
	m->length = htole64(m->length);
    }

    index->node = htole32(index->node);
    index->node_count = htole32(index->node_count);
    index->normalized = htole32(index->normalized);
    index->finalized = htole32(index->finalized);
    index->mapping_count = htole64(index->mapping_count);
}

static void bscfs_index_convert_little_endian_to_host(bscfs_index_t *index)
{
    if (le64toh(0x0123456789abcdefull) == 0x0123456789abcdefull) return;

    index->node = le32toh(index->node);
    index->node_count = le32toh(index->node_count);
    index->normalized = le32toh(index->normalized);
    index->finalized = le32toh(index->finalized);
    index->mapping_count = le64toh(index->mapping_count);

    uint64_t i;
    for (i = 0; i < index->mapping_count; i++) {
	bscfs_mapping_t *m = &index->mapping[i];
	m->sf_offset = le64toh(m->sf_offset);
	m->df_offset = le64toh(m->df_offset);
	m->length = le64toh(m->length);
    }
}

static ssize_t await_aio_completion(data_buffer_t *buf)
{
    int aio_errno;
    struct aiocb *req = &buf->io_req;
    while ((aio_errno = aio_error(req)) == EINPROGRESS) {
	const struct aiocb *const aiocb_list[1] = {req};
	FL_Write(FLAgent, BSCFS_aio_suspend, "calling aio_suspend, req %p", (uint64_t) req, 0,0,0);
	int res = aio_suspend(aiocb_list, 1, NULL);
	if (res != 0) {
	    LOG(bscfsagent,debug)
		<< "aio_suspend() returned error: " << strerror(errno);
	}
    }
    ssize_t nbytes = aio_return(req);
    if (nbytes < 0) nbytes = -aio_errno;
    FL_Write(FLAgent, BSCFS_aio_return, "aio_return returned 0x%lx, req %p", nbytes, (uint64_t) req, 0,0);
    return nbytes;
}

static void data_write_error(shared_file_t *sf, off_t offset, ssize_t nbytes)
{
    if (nbytes >= 0) {
	LOG(bscfsagent,info)
	    << "async write returned " << nbytes
	    << " (" << bscfs_data.write_buffer_size << " expected).";
	offset += nbytes;
	nbytes = -ENOSPC;
    } else {
	LOG(bscfsagent,error)
	    << "async write failed (" << strerror(-nbytes) << ").";
    }
    if (sf->data_file_write_error == 0) {
	sf->data_file_write_error = nbytes;
    }
    if (offset < (ssize_t) sf->data_file_size) {
	sf->data_file_size = offset;
    }
}

static void await_write_completion(shared_file_t *sf, int buf_idx)
{
    data_buffer_t *buf = &sf->write_buffer[buf_idx];
    if (buf->busy) {
	ssize_t nbytes = await_aio_completion(buf);
	buf->busy = 0;
	if (nbytes != ((ssize_t) bscfs_data.write_buffer_size)) {
	    data_write_error(sf, buf->io_req.aio_offset, nbytes);
	}
    }
}

static void await_all_write_completions(shared_file_t *sf)
{
    if (sf->state == BSCFS_MODIFIED) {
	await_write_completion(sf, sf->current_write_buffer);
	await_write_completion(sf, 1 - sf->current_write_buffer);
	(void) fsync(sf->data_fd_write);
    }
}

static void ensure_index_normalized(shared_file_t *sf)
{
    if (!sf->index->normalized) {
	bscfs_index_normalize(sf->index, bscfs_data.max_index_size);

	// Adjust the space allocated for the index file if necessary.
	uint64_t index_size = BSCFS_INDEX_SIZE(sf->index->mapping_count);
	index_size = ((index_size + (BSCFS_INDEX_BLOCK_SIZE - 1))
			/ BSCFS_INDEX_BLOCK_SIZE)
			    * BSCFS_INDEX_BLOCK_SIZE;
	if (index_size != sf->index_file_space) {
	    int res;
	    if (index_size < sf->index_file_space) {
		res = fallocate(sf->index_fd, FALLOC_FL_PUNCH_HOLE,
				index_size,
				sf->index_file_space - index_size);
	    } else {
		res = fallocate(sf->index_fd, FALLOC_FL_KEEP_SIZE,
				sf->index_file_space,
				index_size - sf->index_file_space);
	    }
	    if (res != 0) {
		int errno_save = errno;
		LOG(bscfsagent,info)
		    << "Attempt to change storage allocated for"
		    << " index file from " << sf->index_file_space
		    << " to " << index_size
		    << " failed: " << strerror(errno_save);
		// Ignore the error for now. We'll report the
		// problem if writing the index out fails.
	    }
	    sf->index_file_space = index_size;
	}

	sf->index->normalized = 1;
    }
}

static int finalize_to_bb(shared_file_t *sf)
{
    await_all_write_completions(sf);

    if (sf->write_buffer_offset > 0) {
	// write out partial buffer (rounded to page boundary for O_DIRECT)
	uint64_t pgsize = getpagesize();
	ssize_t bytes_to_write =
	    (sf->write_buffer_offset + (pgsize-1)) & ~(pgsize-1);
	data_buffer_t *buf = &sf->write_buffer[sf->current_write_buffer];
	ssize_t bytes = pwrite(sf->data_fd_write, buf->buffer,
			       bytes_to_write, sf->data_file_size);
	if (bytes != bytes_to_write) {
	    int errno_save = (bytes < 0) ? errno : ENOSPC;
	    LOG(bscfsagent,info)
		<< "finalize_to_bb: failed writing"
		<< " partial data buffer: " << strerror(errno_save);
	    return -errno_save;
	}
	// now fix the file size; O_DIRECT doesn't constrain ftruncate
	sf->data_file_size += sf->write_buffer_offset;
	sf->write_buffer_offset = 0;
	(void) ftruncate(sf->data_fd_write, sf->data_file_size);
	// before writing again, we'll need to restore the partial buffer
	sf->current_write_buffer = -1;
    }

    if (!sf->index->finalized) {
	ensure_index_normalized(sf);
	bscfs_index_finalize(sf->index, bscfs_data.max_index_size);
	sf->index->finalized = 1;
    }

    uint64_t index_size = BSCFS_INDEX_SIZE(sf->index->mapping_count);

    bscfs_index_convert_host_to_little_endian(sf->index);

    uint64_t bytes_written = 0;
    while (bytes_written < index_size) {
	ssize_t bytes = pwrite(sf->index_fd,
			       ((char *) sf->index) + bytes_written,
			       index_size - bytes_written, bytes_written);
	if (bytes < 0) {
	    int errno_save = errno;
	    LOG(bscfsagent,info)
		<< "finalize_to_bb: failed writing index: "
		<< strerror(errno_save);
	    return -errno_save;
	}
	bytes_written += bytes;
    }

    bscfs_index_convert_little_endian_to_host(sf->index);

    return 0;
}



int bscfs_getattr(const char *path, struct stat *stbuf)
{
    FL_Write(FLAgent, BSCFS_getattr, "getattr called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_getattr() called on " << path;

    // check for artificial control file
    if (0 == strcmp(path, "/" BSCFS_CONTROL_FILE_NAME)) {
	// fake a read-only regular file
	stbuf->st_dev = 0;
	stbuf->st_ino = 0;
	stbuf->st_mode = S_IFREG | S_IRUSR;
	stbuf->st_nlink = 1;
	stbuf->st_uid = geteuid();
	stbuf->st_gid = getegid();
	stbuf->st_rdev = 0;
	stbuf->st_size = 0;
	stbuf->st_blksize = 0;
	stbuf->st_blocks = 0;
	stbuf->st_atime = 0;
	stbuf->st_mtime = 0;
	stbuf->st_ctime = 0;
	return 0;
    }

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(path);

    int res, errno_save;

    if (sf != NULL) {
	pthread_mutex_lock(&(sf->lock));
	res = lstat(sf->pfs_file_name, stbuf);
	errno_save = errno;
	if (res == 0) {
	    if ((int64_t) sf->pfs_file_size > stbuf->st_size) {
		stbuf->st_size = sf->pfs_file_size;
		stbuf->st_blocks = (sf->pfs_file_size + 511) / 512;
	    } else {
		sf->pfs_file_size = stbuf->st_size;
	    }
	}
	pthread_mutex_unlock(&(sf->lock));
    } else {
	char *rpath = bscfs_pfs_path(path);
	if (rpath == NULL) {
	    LOG(bscfsagent,info)
		<< "bscfs_getattr() path too long: " << path;
	    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	    return -ENAMETOOLONG;
	}
	res = lstat(rpath, stbuf);
	errno_save = errno;
	free(rpath);
    }

    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_getattr(): lstat on file " << path
	    << " failed: " << strerror(errno_save);
	res = -errno_save;
    }

    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));

    return res;
}

int bscfs_readdir(const char *path, void *buffer, fuse_fill_dir_t file_filler,
		  off_t offset, struct fuse_file_info *file_info)
{
    FL_Write(FLAgent, BSCFS_readdir, "readdir called", 0,0,0,0);
    DIR *dp = (DIR *) file_info->fh;

    LOG(bscfsagent,trace) << "bscfs_readdir() called on DIR " << dp;

    char de_space[offsetof(struct dirent, d_name) + PATH_MAX];

    int res;
    struct dirent *de;
    while (((res = readdir_r(dp, (struct dirent *) de_space, &de)) == 0) &&
		(de != NULL))
    {
	if (file_filler(buffer, de->d_name, NULL, 0) != 0) {
	    res = ENOMEM;
	    break;
	}
    }

    if (res != 0) {
	LOG(bscfsagent,info) << "bscfs_readdir() failed: " << strerror(res);
	return -res;
    }
    return 0;
}


//called when user creates a new file
int bscfs_mknod(const char *name, mode_t mode, dev_t dev)
{
    FL_Write(FLAgent, BSCFS_mknod, "mknod called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_mknod() called on " << name;
    char *rpath = bscfs_pfs_path(name);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_mknod() path too long: " << name;
	return -ENAMETOOLONG;
    }
    int res = mknod(rpath, mode, dev);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info) << "bscfs_mknod() failed: " << strerror(errno_save) ;
	return -errno_save;
    }
    return 0;
}


int bscfs_link (const char *source, const char *dest)
{
    FL_Write(FLAgent, BSCFS_link, "link called", 0,0,0,0);
    LOG(bscfsagent,info)
	<< "bscfs_link() called on " << source << "; NOT IMPLEMENTED!";
    return -ENOSYS;
}


int bscfs_unlink(const char *name)
{
    FL_Write(FLAgent, BSCFS_unlink, "unlink called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_unlink() called on " << name;

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(name);

    int res = 0;
    if (sf == NULL) {
	char *rpath = bscfs_pfs_path(name);
	if (rpath == NULL) {
	    LOG(bscfsagent,info)
		<< "bscfs_unlink() path too long: " << name;
	    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	    return -ENAMETOOLONG;
	}
	if (unlink(rpath) < 0) {
	    int errno_save = errno;
	    LOG(bscfsagent,info)
		<< "bscfs_unlink() failed: " << strerror(errno_save);
	    res = -errno_save;
	}
	free(rpath);
    } else {
	pthread_mutex_lock(&(sf->lock));
	if (sf->state == BSCFS_INACTIVE) {
	    if (unlink(sf->pfs_file_name) < 0) {
		int errno_save = errno;
		LOG(bscfsagent,info)
		    << "bscfs_unlink() failed: "
		    << strerror(errno_save);
		res = -errno_save;
	    } else {
		free(sf->file_name);
		free(sf->pfs_file_name);
		sf->file_name = strdup("<removed>");
		sf->pfs_file_name = strdup("<removed>");
	    }
	} else {
	    LOG(bscfsagent,info)
		<< "bscfs_unlink() file " << name << " is active";
	    res = -EPERM;
	}
	pthread_mutex_unlock(&(sf->lock));
    }
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
    return res;
}

int bscfs_chmod(const char *name, mode_t mode)
{
    FL_Write(FLAgent, BSCFS_chmod, "chmod called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_chmod() called on " << name;
    char *rpath = bscfs_pfs_path(name);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_chmod() path too long: " << name;
	return -ENAMETOOLONG;
    }
    int res = chmod(rpath, mode);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_chmod() failed: " << strerror(errno_save);
	return -errno_save;
    }
    return 0;
}

int bscfs_chown(const char *name, uid_t uid, gid_t gid)
{
    FL_Write(FLAgent, BSCFS_chown, "chown called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_chown() called on " << name;
    char *rpath = bscfs_pfs_path(name);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_chown() path too long: " << name;
	return -ENAMETOOLONG;
    }
    int res = chown(rpath, uid, gid);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_chown() failed: " << strerror(errno_save);
	return -errno_save;
    }
    return 0;
}

int bscfs_utime(const char *name, struct utimbuf *ubuf)
{
    FL_Write(FLAgent, BSCFS_utime, "utime called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_utime() called on " << name;
    char *rpath = bscfs_pfs_path(name);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_utime() path too long: " << name;
	return -ENAMETOOLONG;
    }
    int res = utime(rpath, ubuf);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_utime() failed: " << strerror(errno_save);
	return -errno_save;
    }
    return 0;
}

static void shared_file_init(shared_file_t *sf)
{
    pthread_mutex_init(&sf->lock, NULL);
    sf->open_count = 0;
    sf->state = BSCFS_INACTIVE;
    sf->accmode = 0;
    sf->file_name = NULL;
    sf->pfs_file_name = NULL;
    sf->map_file_name = NULL;
    sf->cleanup_file_name = NULL;
    sf->data_file_name = NULL;
    sf->index_file_name = NULL;
    sf->pfs_fd = -1;
    sf->data_fd_write = -1;
    sf->data_fd_read = -1;
    sf->index_fd = -1;
    sf->pfs_file_size = 0;
    sf->data_file_size = 0;
    sf->data_file_space = 0;
    sf->data_file_offset = 0;
    sf->index_file_space = 0;
    sf->index = NULL;
    sf->data_file_write_error = 0;
    sf->current_write_buffer = -1; // not ready for writing
    sf->write_buffer_offset = 0;
    memset(&(sf->write_buffer[0].io_req), 0, sizeof(struct aiocb));
    sf->write_buffer[0].busy = 0;
    sf->write_buffer[0].buffer = NULL;
    memset(&(sf->write_buffer[1].io_req), 0, sizeof(struct aiocb));
    sf->write_buffer[1].busy = 0;
    sf->write_buffer[1].buffer = NULL;
    memset(&(sf->read_buffer[0].io_req), 0, sizeof(struct aiocb));
    sf->read_buffer[0].busy = 0;
    sf->read_buffer[0].buffer = NULL;
    sf->read_buffer[0].io_req.aio_offset = -1; // no valid data currently
    memset(&(sf->read_buffer[1].io_req), 0, sizeof(struct aiocb));
    sf->read_buffer[1].busy = 0;
    sf->read_buffer[1].buffer = NULL;
    sf->read_buffer[1].io_req.aio_offset = -1; // no valid data currently
    sf->transfer_handle = 0;
}

static shared_file_t *shared_file_create(int *resp, const char *path,
					 int flags, mode_t mode)
{
    shared_file_t *sf = new shared_file_t();
    FL_sf_new(sf);
    shared_file_init(sf);
    sf->file_name = strdup(path);
    sf->accmode = flags & O_ACCMODE;
    sf->open_count = 0;
    sf->pfs_file_name = bscfs_pfs_path(path);
    if (sf->pfs_file_name == NULL) {
	LOG(bscfsagent,info)
	    << "shared_file_create() path too long: " << path;
	free(sf->file_name);
	FL_sf_delete(sf);
	delete sf;
	(*resp) = -ENAMETOOLONG;
	return NULL;
    }
    LOG(bscfsagent,info)
	<< "shared_file_create:"
	<< "  path=" << path
	<< "  flags=" << flags
	<< "  mode=" << mode
	<< "  pfsname=" << sf->pfs_file_name;
    sf->pfs_fd = open(sf->pfs_file_name, flags, mode);
    if (sf->pfs_fd < 0) {
	int errno_save = errno;
	LOG(bscfsagent,info)
	    << "shared_file_create() failed to open pfs file "
	    << sf->pfs_file_name
	    << ": " << strerror(errno_save);
	free(sf->pfs_file_name);
	free(sf->file_name);
	FL_sf_delete(sf);
	delete sf;
	(*resp) = -errno_save;
	return NULL;
    }
#if USE_INODE
    int rc = fstat(sf->pfs_fd, &sf->pfsstatcache);
    if (rc < 0) {
	int errno_save = errno;
	LOG(bscfsagent,info)
	    << "shared_file_create() failed to fstat pfs file "
	    << sf->pfs_file_name
	    << ": " << strerror(errno_save);
    }
#endif
    
    // add file to the shared_files map
    bscfs_data.shared_files[path] = sf;

    (*resp) = 0;
    return sf;
}

static int activate_index(shared_file_t *sf, int create)
{
    // use index_fd as a flag indicating prior activation
    if (sf->index_fd >= 0) return 0;

    int create_flags = create ? (O_CREAT|O_TRUNC) : 0;
    sf->index_fd = open(sf->index_file_name, O_RDWR | create_flags,
		       S_IWUSR|S_IRUSR);
    if (sf->index_fd < 0) {
	int errno_save = errno;
	LOG(bscfsagent,info)
	    << "activate_index(): failed to open index file "
	    << sf->index_file_name << ": " << strerror(errno_save);
	return -errno_save;
    }
    // SSD space will be allocated when the first mapping is instantiated
    sf->index_file_space = 0;

    int res = posix_memalign((void**) &(sf->index), getpagesize(),
			     bscfs_data.max_index_size);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "activate_index(): index allocation failed: "
	    << strerror(res);
	close(sf->index_fd);
	sf->index_fd = -1;
	return -res;
    }

    sf->index->node = bscfs_data.node_number;
    sf->index->node_count = bscfs_data.node_count;

    // an empty index is both normalized and finalized
    sf->index->normalized = 1;
    sf->index->finalized = 1;

    sf->index->mapping_count = 0;

    // pfs file is empty, as far as we know locally
    sf->pfs_file_size = 0;

    return 0;
}

static int activate_for_reading(shared_file_t *sf, int create)
{
    int res;

    res = activate_index(sf, create);
    if (res != 0) return res;

    // A MODIFIED file may have asychronous writes outstanding.
    await_all_write_completions(sf);

    // The index may not be normalized.
    ensure_index_normalized(sf);

    // use data_fd_read as a flag indicating prior activation
    if (sf->data_fd_read >= 0) return 0;

    int create_flags = create ? (O_CREAT|O_TRUNC) : 0;
    sf->data_fd_read = open(sf->data_file_name, O_RDWR | create_flags,
			    S_IWUSR|S_IRUSR);
    if (sf->data_fd_read < 0) {
	int errno_save = errno;
	LOG(bscfsagent,info)
	    << "activate_for_reading(): failed to open data file "
	    << sf->data_file_name << ": " << strerror(errno_save);
	return -errno_save;
    }

    res = posix_memalign((void**) &(sf->read_buffer[0].buffer),
			 getpagesize(), bscfs_data.read_buffer_size);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "activate_for_reading(): buffer allocation failed: "
	    << strerror(res);
	return -res;
    }
    sf->read_buffer[0].busy = 0;
    sf->read_buffer[0].io_req.aio_offset = -1; // no valid data currently

    res = posix_memalign((void**) &(sf->read_buffer[1].buffer),
			 getpagesize(), bscfs_data.read_buffer_size);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "activate_for_reading(): buffer allocation failed: "
	    << strerror(res);
	free(sf->read_buffer[0].buffer);
	sf->read_buffer[0].buffer = NULL;
	return -res;
    }
    sf->read_buffer[1].busy = 0;
    sf->read_buffer[1].io_req.aio_offset = -1; // no valid data currently

    return 0;
}

static int activate_for_writing(shared_file_t *sf, int create)
{
    int res;

    res = activate_index(sf, create);
    if (res != 0) return res;

    // use data_fd_write as a flag indicating prior activation
    if (sf->data_fd_write >= 0) return 0;

    int create_flags = create ? (O_CREAT|O_TRUNC) : 0;
    sf->data_fd_write = open(sf->data_file_name, O_RDWR|O_DIRECT | create_flags,
			     S_IWUSR|S_IRUSR);
    if (sf->data_fd_write < 0) {
	int errno_save = errno;
	LOG(bscfsagent,info)
	    << "activate_for_writing(): failed to open data file "
	    << sf->data_file_name << ": " << strerror(errno_save);
	return -errno_save;
    }
    sf->data_file_space = 0;
    sf->data_file_size = 0;

    res = posix_memalign((void**) &(sf->write_buffer[0].buffer), getpagesize(),
			 bscfs_data.write_buffer_size);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "activate_for_writing(): buffer allocation failed: "
	    << strerror(res);
	close(sf->data_fd_write);
	sf->data_fd_write = -1;
	return -res;
    }
    sf->write_buffer[0].busy = 0;

    res = posix_memalign((void**) &(sf->write_buffer[1].buffer), getpagesize(),
			 bscfs_data.write_buffer_size);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "activate_for_writing(): buffer allocation failed: "
	    << strerror(res);
	free(sf->write_buffer[0].buffer);
	sf->write_buffer[0].buffer = NULL;
	close(sf->data_fd_write);
	sf->data_fd_write = -1;
	return -res;
    }
    sf->write_buffer[1].busy = 0;

    sf->current_write_buffer = 0;
    sf->write_buffer_offset = 0;
    sf->data_file_write_error = 0;

    return 0;
}

static int ensure_ready_for_writing(shared_file_t *sf)
{
    if (sf->current_write_buffer >= 0) return 0;

    // The data file has been finalized to the SSD. That is, the last partial
    // buffer has been written to the file and any extra fallocated space has
    // been released. We now need to get the file back into its
    // pre-finalization state in order to continue writing to it. That means
    // reading any partial buffer back into memory, truncating the partial
    // buffer from the end of the file, and fallocating space beyond the end
    // of the file to ensure the next buffer write(s) will succeed.
    uint64_t cur_size = lseek(sf->data_fd_write, 0, SEEK_END);
    uint64_t new_size = (cur_size
			    / bscfs_data.write_buffer_size)
				* bscfs_data.write_buffer_size;
    uint64_t new_space = 0;
    if (bscfs_data.data_falloc_size > 0) {
	new_space = ((cur_size + bscfs_data.data_falloc_size - 1)
			    / bscfs_data.data_falloc_size)
				* bscfs_data.data_falloc_size;
    }
    sf->data_file_size = new_size;
    sf->data_file_space = new_space;
    data_buffer_t *buf = &sf->write_buffer[0];
    sf->current_write_buffer = 0;
    sf->write_buffer_offset = cur_size - new_size;
    sf->data_file_write_error = 0;
    if (sf->write_buffer_offset > 0) {
	ssize_t bytes = pread(sf->data_fd_write, buf->buffer,
			      sf->write_buffer_offset, new_size);
	if (bytes != (ssize_t) sf->write_buffer_offset) {
	    int errno_save = errno;
	    LOG(bscfsagent,error)
		<< "ensure_ready_for_writing: failed reading"
		<< " partial data buffer: "
		<< ((bytes < 0) ? strerror(errno_save) :
				    "truncated read")
		<< "; BSCFS exiting!";
	    exit(-1);
	}
	(void) ftruncate(sf->data_fd_write, new_size);
    }
    if (new_space > new_size) {
	int res = fallocate(sf->data_fd_write, FALLOC_FL_KEEP_SIZE,
			    new_size, new_space - new_size);
	if (res != 0) {
	    int errno_save = errno;
	    LOG(bscfsagent,info)
		<< "ensure_ready_for_writing: failed to allocate SSD"
		<< " space for next buffer: "
		<< strerror(errno_save);
	    sf->data_file_space = new_size;
	    return -errno_save;
	}
    }
    return 0;
}

static int ingest_internal_files(shared_file_t *sf)
{
    uint64_t index_size = lseek(sf->index_fd, 0, SEEK_END);
    if (index_size > bscfs_data.max_index_size) {
	LOG(bscfsagent,info)
	    << "ingest_internal_files: index file " << sf->index_file_name
	    << " too big";
	return -ENOMEM;
    }

    uint64_t bytes_read = 0;
    while (bytes_read < index_size) {
	ssize_t bytes = pread(sf->index_fd,
			      ((char *) sf->index) + bytes_read,
			      index_size - bytes_read, bytes_read);
	if (bytes < 0) {
	    int errno_save = errno;
	    LOG(bscfsagent,info)
		<< "ingest_internal_files: failed reading index: "
		<< strerror(errno_save);
	    return -errno_save;
	}
	bytes_read += bytes;
    }

    bscfs_index_convert_little_endian_to_host(sf->index);

    sf->index_file_space = ((index_size + (BSCFS_INDEX_BLOCK_SIZE - 1))
				/ BSCFS_INDEX_BLOCK_SIZE)
				    * BSCFS_INDEX_BLOCK_SIZE;

    sf->data_file_size = lseek(sf->data_fd_read, 0, SEEK_END);
    sf->current_write_buffer = -1; // not ready for writing

    return 0;
}

static int activate(const char *path, bscfs_file_state_t state,
		    const char *index_path, const char *data_path)
{
    int res = 0;

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(path);

    if (sf != NULL) {
	LOG(bscfsagent,info) << "activate() called on active file: " << path;
	res = -EPERM;
	goto done;
    }

    sf = shared_file_create(&res, path,
			    ((state == BSCFS_STABLE) ? O_RDONLY : O_WRONLY),
			    0);
    if (sf == NULL) goto done;

    sf->state = state;
    FL_sf_state(sf);
    sf->data_file_name = strdup(data_path);
    sf->index_file_name = strdup(index_path);

    if (state == BSCFS_STABLE) {
	res = activate_for_reading(sf, 0);
    } else {
	res = activate_for_writing(sf, 0);
    }
    if (res != 0) goto done;

    res = ingest_internal_files(sf);

done:
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
    return res;
}

int bscfs_open(const char *name, struct fuse_file_info *file_info)
{
    FL_Write(FLAgent, BSCFS_open, "open called", 0,0,0,0);
    LOG(bscfsagent,trace)
	<< "bscfs_open() called with flags " << file_info->flags
	<< ", name " << name;
    return bscfs_create(name, 0, file_info);
}

static ssize_t data_file_pread(shared_file_t *sf, char *buffer,
			       size_t size, uint64_t offset)
{
    uint64_t remainder = size;

    while (remainder > 0) {
	uint64_t buf_num = offset / bscfs_data.read_buffer_size;

	// first kick off an async read of the *next* buffer if possible
	data_buffer_t *next_buf = &sf->read_buffer[(buf_num + 1) % 2];
	uint64_t next_buf_offset =
	    (buf_num + 1) * bscfs_data.read_buffer_size;
	if ((!next_buf->busy) &&
	    (next_buf->io_req.aio_offset != (off_t) next_buf_offset) &&
	    (next_buf_offset < sf->data_file_size))
	{
	    struct aiocb *req = &next_buf->io_req;
	    req->aio_fildes = sf->data_fd_read;
	    req->aio_offset = (off_t) next_buf_offset;
	    req->aio_buf = next_buf->buffer;
	    req->aio_nbytes = sf->data_file_size - next_buf_offset;
	    if (req->aio_nbytes > bscfs_data.read_buffer_size) {
		req->aio_nbytes = bscfs_data.read_buffer_size;
	    }
	    req->aio_reqprio = 0;
	    req->aio_sigevent.sigev_notify = SIGEV_NONE;
	    next_buf->busy = 1;
	    (void) aio_read(req);
	    FL_Write(FLAgent, BSCFS_aio_read, "aio_read called, req %p, size 0x%lx, offset 0x%lx", (uint64_t) req, req->aio_nbytes, req->aio_offset, 0);
	}

	// wait for any async read into the current buffer to complete
	data_buffer_t *cur_buf = &sf->read_buffer[buf_num % 2];
	uint64_t cur_buf_offset = buf_num * bscfs_data.read_buffer_size;
	struct aiocb *req = &cur_buf->io_req;
	if (cur_buf->busy) {
	    ssize_t res = await_aio_completion(cur_buf);
	    if (res != (ssize_t) req->aio_nbytes) {
		LOG(bscfsagent,debug)
		    << "async read returned " << res
		    << " (" << req->aio_nbytes
		    << " expected). Ignoring.";
		req->aio_offset = -1;
	    }
	    cur_buf->busy = 0;
	}

	// if the current buffer doesn't hold the block we need, reset it
	if (req->aio_offset != (off_t) cur_buf_offset) {
	    req->aio_offset = (off_t) cur_buf_offset;
	    req->aio_nbytes = 0;
	}
	uint64_t cur_buf_end = req->aio_offset + req->aio_nbytes;
	// if the buffer doesn't yet hold the offset we need, extend it with
	// a blocking pread call
	if (offset >= cur_buf_end) {
	    size_t needed = bscfs_data.read_buffer_size - req->aio_nbytes;
	    if (needed > (sf->data_file_size - cur_buf_end)) {
		needed = sf->data_file_size - cur_buf_end;
	    }
	    FL_Write(FLAgent, BSCFS_pread, "calling pread, sf %p, size 0x%lx, offset 0x%lx", (uint64_t) sf, needed, cur_buf_end, 0);
	    ssize_t res = pread(sf->data_fd_read,
				cur_buf->buffer + req->aio_nbytes,
				needed, cur_buf_end);
	    if (res != (ssize_t) needed) {
		// We don't try to handle real read failures.
		if (res < 0) {
		    LOG(bscfsagent,error)
			<< "pread() failed (" << strerror(errno)
			<< "). BSCFS exiting!";
		} else {
		    LOG(bscfsagent,error)
			<< "pread() returned " << res
			<< " (" << needed
			<< " expected). BSCFS exiting!";
		}
		exit(-1);
	    }
	    req->aio_nbytes += needed;
	}

	// copy as much as we can from the read buffer
	uint64_t chunk = bscfs_data.read_buffer_size -
					(offset - cur_buf_offset);
	if (chunk > remainder) chunk = remainder;
	memcpy(buffer, cur_buf->buffer + (offset - cur_buf_offset), chunk);

	buffer += chunk;
	offset += chunk;
	remainder -= chunk;
    }

    return size;
}

static ssize_t do_pfs_read(shared_file_t *sf, char *buffer,
			   size_t size, uint64_t offset)
{
    ssize_t res = pread(sf->pfs_fd, buffer, size, offset);
    if (res != (ssize_t) size) {
	if (res < 0) {
	    int errno_save = errno;
	    LOG(bscfsagent,info)
		<< "do_pfs_read(): pread failed: "
		<< strerror(errno_save);
	    res = -errno_save;
	} else {
	    LOG(bscfsagent,debug)
		<< "do_pfs_read(): pread returned " << res
		<< "; expected " << size;
	}
	return res;
    }
    return size;
}

static ssize_t do_local_read(shared_file_t *sf, char *buffer,
			     size_t size, uint64_t offset)
{
    int64_t idx = bscfs_index_lookup(sf->index, offset);
    if ((idx < 0) ||
	(offset >= (sf->index->mapping[idx].sf_offset +
			sf->index->mapping[idx].length)))
    {
	if (offset == sf->pfs_file_size) {
	    // EOF, as near as we can tell
	    return 0;
	}
	// no local data available at this offset
	LOG(bscfsagent,info) << "do_local_read(): no local data available";
	return -ENODATA;
    }

    uint64_t remainder = size;

    while (remainder > 0) {
	bscfs_mapping_t *m = &sf->index->mapping[idx];

	uint64_t df_offset = m->df_offset + (offset - m->sf_offset);
	uint64_t chunk = m->length - (offset - m->sf_offset);
	if (chunk > remainder) chunk = remainder;

	// For a MODIFIED file, part of the requested data may have been
	// written to the data file, while the rest remains in the data
	// buffer.
	uint64_t file_offset, file_size, buf_offset, buf_size;
	if ((df_offset + chunk) <= sf->data_file_size) {
	    // data is all in the data file
	    file_offset = df_offset;
	    file_size = chunk;
	    buf_offset = 0;
	    buf_size = 0;
	} else if (df_offset >= sf->data_file_size) {
	    // data is all in the current data buffer
	    file_offset = 0;
	    file_size = 0;
	    buf_offset = df_offset - sf->data_file_size;
	    buf_size = chunk;
	} else {
	    // data is split
	    file_offset = df_offset;
	    file_size = sf->data_file_size - df_offset;
	    buf_offset = 0;
	    buf_size = (df_offset + chunk) - sf->data_file_size;
	}

	if (file_size > 0) {
	    ssize_t res = data_file_pread(sf, buffer, file_size,
					  file_offset);
	    if (res < 0) {
		int errno_save = errno;
		LOG(bscfsagent,info)
		    << "do_local_read(): pread failed: "
		    << strerror(errno_save);
		if (size > remainder) return (size - remainder);
		return -errno_save;
	    }
	    if (res < (int64_t) file_size) {
		LOG(bscfsagent,debug)
		    << "do_local_read(): pread returned " << res
		    << "; expected " << file_size;
		return (size - (remainder - res));
	    }
	}

	if (buf_size > 0) {
	    char *buf = sf->write_buffer[sf->current_write_buffer].buffer;
	    memcpy(buffer + file_size, buf + buf_offset, buf_size);
	}

	offset += chunk;
	buffer += chunk;
	remainder -= chunk;

	if (remainder > 0) {
	    // move to the next mapping
	    idx++;
	    if ((idx >= (int64_t) sf->index->mapping_count) ||
		(offset < sf->index->mapping[idx].sf_offset))
	    {
		// next mapping not adjacent; return what we have
		LOG(bscfsagent,debug)
		    << "do_local_read(): "
		    << (size - remainder) << " bytes available, "
		    << size << " bytes requested";
		return (size - remainder);
	    }
	}
    }

    return size;
}

static ssize_t do_mixed_read(shared_file_t *sf, char *buffer,
			     size_t size, uint64_t offset)
{
    int64_t idx = bscfs_index_lookup(sf->index, offset);
    bscfs_mapping_t *m = NULL;
    uint64_t end = 0;
    if (idx >= 0) {
	m = &sf->index->mapping[idx];
	end = m->sf_offset + m->length;
    }

    uint64_t remainder = size;

    while (remainder > 0) {
	ssize_t res;
	uint64_t chunk;
	if (offset < end) {
	    // offset is inside the current mapping; read as much
	    // as we can from the data file (a STABLE file will not
	    // have data remaining in the data buffers)
	    chunk = end - offset;
	    if (chunk > remainder) chunk = remainder;
	    res = data_file_pread(sf, buffer, chunk,
				  m->df_offset +
				      (offset - m->sf_offset));
	} else {
	    // offset is beyond the current mapping; move to the next
	    // mapping and read any missing data from the pfs file
	    chunk = remainder;
	    idx++;
	    if (idx < (int64_t) sf->index->mapping_count) {
		m = &sf->index->mapping[idx];
		end = m->sf_offset + m->length;
		if (chunk > (m->sf_offset - offset)) {
		    chunk = m->sf_offset - offset;
		}
	    }
	    // chunk will be 0 if there is no gap between adjacent
	    // mappings (which will happen if the mappings are
	    // contiguous in sf_offsets but not in df_offsets)
	    res = 0;
	    if (chunk > 0) {
		res = pread(sf->pfs_fd, buffer, chunk, offset);
		if (res < 0) res = -errno;
	    }
	}
	if (res != (int64_t) chunk) {
	    if (res < 0) {
		LOG(bscfsagent,info)
		    << "do_mixed_read(): pread failed: "
		    << strerror(-res);
		if (size > remainder) return (size - remainder);
		return res;
	    }
	    LOG(bscfsagent,debug)
		<< "do_mixed_read(): pread returned "
		<< res << "; expected " << chunk;
	    return (size - (remainder - res));
	}
	offset += chunk;
	buffer += chunk;
	remainder -= chunk;
    }

    return size;
}

int bscfs_read(const char *name , char *buffer, size_t size, off_t offset,
               struct fuse_file_info *file_info)
{
    ssize_t res;
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_read, "read called, sf %p, size 0x%lx, offset 0x%lx", (uint64_t) sf, size, offset, 0);

    LOG(bscfsagent,trace)
	<< "bscfs_read() called on " << sf->file_name
	<< ", size " << size
	<< ", offset " << offset;

    pthread_mutex_lock(&(sf->lock));

    if ((sf->state == BSCFS_INACTIVE) || (sf->state == BSCFS_PREFETCHING)) {
	res = do_pfs_read(sf, buffer, size, offset);
    } else if (sf->state == BSCFS_STABLE) {
	res = activate_for_reading(sf, 0);
	if (res == 0) {
	    res = do_mixed_read(sf, buffer, size, offset);
	}
    } else if (sf->state == BSCFS_CONTROL) {
	LOG(bscfsagent,info)
	    << "bscfs_read: file " << sf->file_name
	    << " in disallowed state " << sf->state;
	res = -EPERM;
    } else {
	// state == MODIFIED, FLUSH_PENDING, FLUSHING, or FLUSH_COMPLETED
	res = activate_for_reading(sf, 0);
	if (res == 0) {
	    res = do_local_read(sf, buffer, size, offset);
	}
    }

    pthread_mutex_unlock(&(sf->lock));
    return res;
}

static int do_write(shared_file_t *sf, const char *buffer,
		    uint64_t size, uint64_t offset)
{
    if (size == 0) return 0; // nothing to write, fake call -_-
    if (sf->data_file_write_error != 0) return sf->data_file_write_error;

    /****************************************
     * update index
     ****************************************/

    /* check if this write continues the previous one */
    bscfs_mapping_t *map = NULL;
    if (sf->index->mapping_count > 0) {
	bscfs_mapping_t *m =
	    &sf->index->mapping[sf->index->mapping_count - 1];
	if ((offset == (m->sf_offset + m->length)) &&
	    (sf->data_file_offset == (m->df_offset + m->length)))
	{
	    // new write is contiguous; simply bump the mapping size
	    map = m;
	    map->length += size;
	}
	if (offset < (m->sf_offset + m->length)) {
	    // index is no longer normalized, whether or not it
	    // was normalized to begin with
	    sf->index->normalized = 0;
	}
    }
    if (map == NULL) {
	// allocate a new mapping
	uint64_t index_size = BSCFS_INDEX_SIZE(sf->index->mapping_count+1);
	if (index_size > bscfs_data.max_index_size) {
	    // no more room in index
	    LOG(bscfsagent,error)
		<< "do_write(): max_index_size of "
		<< bscfs_data.max_index_size
		<< " bytes exceeded; BSCFS exiting!";
	    exit(-1);
	}
	if (index_size > sf->index_file_space) {
	    // allocate another block of SSD space for the index file
	    int res = fallocate(sf->index_fd, FALLOC_FL_KEEP_SIZE,
				sf->index_file_space,
				BSCFS_INDEX_BLOCK_SIZE);
	    int errno_save = errno;
	    if (res != 0) {
		LOG(bscfsagent,info)
		    << "do_write(): failed to allocate SSD space"
		    << " for next index block: "
		    << strerror(errno_save);
		return -errno_save;
	    }
	    sf->index_file_space += BSCFS_INDEX_BLOCK_SIZE;
	}
	map = &sf->index->mapping[sf->index->mapping_count];
	sf->index->mapping_count++;
	map->sf_offset = offset;
	map->df_offset = sf->data_file_offset;
	map->length = size;
    }
    sf->data_file_offset += size;

    /****************************************
     * copy and possibly write out data
     ****************************************/

    uint64_t remainder = size;
    while (remainder > 0) {
	data_buffer_t *cur_buf = &sf->write_buffer[sf->current_write_buffer];

	// If we're preallocating data-file SSD space, we're starting a new
	// buffer, and we haven't allocated SSD space for it yet, allocate a
	// new chunk of SSD space now to ensure we'll be able to write the
	// buffer out when it's full.
	if ((bscfs_data.data_falloc_size > 0) &&
	    (sf->write_buffer_offset == 0) &&
	    (sf->data_file_size >= sf->data_file_space))
	{
	    int res = fallocate(sf->data_fd_write, FALLOC_FL_KEEP_SIZE,
				sf->data_file_size,
				bscfs_data.data_falloc_size);
	    int errno_save = errno;
	    if (res != 0) {
		LOG(bscfsagent,info)
		    << "do_write(): failed to allocate SSD space"
		    << " for next buffer: "
		    << strerror(errno_save);
		// roll back data_file_offset
		sf->data_file_offset -= remainder;
		// trim mapping to what we were able to absorb
		map->length -= remainder;
		// remove the mapping if it's empty
		if (map->length == 0) sf->index->mapping_count--;
		if (size > remainder) return (size - remainder);
		return -errno_save;
	    }
	    sf->data_file_space += bscfs_data.data_falloc_size;
	}

	// check if current buffer is busy with a previous write, and
	// wait for the write to complete if it is
	if (cur_buf->busy) {
	    ssize_t nbytes = await_aio_completion(cur_buf);
	    cur_buf->busy = 0;
	    if (nbytes != ((ssize_t) bscfs_data.write_buffer_size)) {
		// Real write failures shouldn't happen if we're pre-allocating
		// data-file SSD space, but they might happen if we're not.
		data_write_error(sf, cur_buf->io_req.aio_offset, nbytes);
		return sf->data_file_write_error;
	    }
	}

	// copy as much data as will fit into the current buffer
	uint64_t chunk =
	    bscfs_data.write_buffer_size - sf->write_buffer_offset;
	if (chunk > remainder) chunk = remainder;
	memcpy(cur_buf->buffer + sf->write_buffer_offset, buffer, chunk);
	sf->write_buffer_offset += chunk;
	buffer += chunk;
	remainder -= chunk;


	// check if the current buffer is now filled, and initiate an
	// asynchronous write if it is
	if (sf->write_buffer_offset == bscfs_data.write_buffer_size) {
	    struct aiocb *req = &cur_buf->io_req;
	    req->aio_fildes = sf->data_fd_write;
	    req->aio_offset = (off_t) sf->data_file_size;
	    req->aio_buf = cur_buf->buffer;
	    req->aio_nbytes = bscfs_data.write_buffer_size;
	    req->aio_reqprio = 0;
	    req->aio_sigevent.sigev_notify = SIGEV_NONE;

	    cur_buf->busy = 1;

	    aio_write(req);
	    FL_Write(FLAgent, BSCFS_aio_write, "aio_write called, req %p, size 0x%lx, offset 0x%lx", (uint64_t) req, req->aio_nbytes, req->aio_offset, 0);

	    sf->data_file_size += bscfs_data.write_buffer_size;

	    // switch buffers
	    sf->current_write_buffer = 1 - sf->current_write_buffer;
	    sf->write_buffer_offset = 0;
	}
    }

    return size;
}


int bscfs_write(const char *name, const char *buffer, size_t size,
		off_t offset, struct fuse_file_info *file_info)
{
    int res;
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_write, "write called, sf %p, size 0x%lx, offset 0x%lx", (uint64_t) sf, size, offset, 0);

    LOG(bscfsagent,trace)
	<< "bscfs_write() called on " << sf->file_name
	<< ", of size " << size
	<< ", at offset " << offset;

    pthread_mutex_lock(&(sf->lock));

    if (sf->state == BSCFS_MODIFIED) {
	res = ensure_ready_for_writing(sf);
	if (res == 0) {
	    res = do_write(sf, buffer, size, offset);
	}
    } else if (sf->state == BSCFS_INACTIVE) {
	res = activate_for_writing(sf, 1);
	if (res == 0) {
	    sf->state = BSCFS_MODIFIED;
	    FL_sf_state(sf);
	    res = do_write(sf, buffer, size, offset);
	}
    } else {
	// state == FLUSH_PENDING, FLUSHING, FLUSH_COMPLETED,
	//          PREFETCHING, STABLE, or CONTROL
	LOG(bscfsagent,info)
	    << "bscfs_write: file " << sf->file_name
	    << " in disallowed state " << sf->state;
	res = -EPERM;
    }

    if ((res > 0) && ((offset + res) > (int64_t) sf->pfs_file_size)) {
	sf->pfs_file_size = offset + res;
    }

    pthread_mutex_unlock(&(sf->lock));
    return res;
}


int bscfs_statfs(const char *path, struct statvfs *stat_fs)
{
    FL_Write(FLAgent, BSCFS_statfs, "statfs called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_statfs() called on " << path;
    char *rpath = bscfs_pfs_path(path);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_statfs() path too long: " << path;
	return -ENAMETOOLONG;
    }
    int res = statvfs(rpath, stat_fs);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_statfs() failed: " << strerror(errno_save);
	return -errno_save;
    }
    return 0;
}

int bscfs_flush(const char *path, struct fuse_file_info *file_info)
{
    // We'll wait for any outstanding writes to complete, but that
    // doesn't really count for much. The final data buffer and the
    // index may not have been written out yet (although SSD space for
    // them will have been fallocated.
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_flush, "flush called, sf %p", (uint64_t) sf, 0,0,0);
    LOG(bscfsagent,trace) << "bscfs_flush() called on " << sf->file_name;
    pthread_mutex_lock(&(sf->lock));
    await_all_write_completions(sf);
    int res = sf->data_file_write_error;
    pthread_mutex_unlock(&(sf->lock));
    return res;
}

int bscfs_release(const char *path, struct fuse_file_info *file_info)
{
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_release, "release called, sf %p", (uint64_t) sf, 0,0,0);

    LOG(bscfsagent,trace) << "bscfs_release() called on " << sf->file_name;

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    pthread_mutex_lock(&(sf->lock));

    sf->open_count--;

    if ((sf->open_count == 0) && (sf->state == BSCFS_INACTIVE)) {
	// remove sf from the shared-files list
	if (bscfs_data.shared_files.erase(sf->file_name) == 0) {
	    LOG(bscfsagent,error)
		<< "bscfs_release(): file " << sf->file_name
		<< " not found in shared-files list. BSCFS exiting!";
	    exit(-1);
	}

	if (close(sf->pfs_fd) < 0) {
	    int errno_save = errno;
	    LOG(bscfsagent,info)
		<< "bscfs_release(): close failed: "
		<< strerror(errno_save)
		<< ". Failure ignored.";
	}
	free(sf->data_file_name);
	free(sf->index_file_name);
	free(sf->pfs_file_name);
	free(sf->file_name);
	FL_sf_delete(sf);
	pthread_mutex_unlock(&(sf->lock)); // for coverity
	delete sf;
    } else {
	pthread_mutex_unlock(&(sf->lock));
    }

    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));

    return 0; // ignored
}

static int start_flush(shared_file_t *sf)
{
    int res;

    // tag allocation is serialized by shared_files_lock, which must be held
    // by the caller
    BBTAG tag;
    tag = bscfs_data.next_transfer_tag++;

    BBTransferHandle_t handle;
    res = BB_GetTransferHandle(tag, 1, NULL, &handle);
    if (res != 0) {
	log_bb_error(res, "BB_GetTransferHandle");
	return res;
    }

    BBTransferDef_t *tdef;
    res = BB_CreateTransferDef(&tdef);
    if (res != 0) {
	log_bb_error(res, "BB_CreateTransferDef");
	return res;
    }

    char *mapfile = sf->map_file_name;
    if (mapfile[0] == '\0') {
	// The application is not required to specify a mapfile for a flush,
	// but bbProxy requires a pathname that appears to be a PFS file.
    	mapfile = bscfs_pfs_path("/<no_mapfile>");
    }
    res = BB_AddFiles(tdef, sf->index_file_name, mapfile,
		      BBFILEFLAGS(BBFileBundleID(1) |
				  BBTransferOrder(0) |
				  BBTransferType(BBTransferTypeBSCFS)));
    if (mapfile != sf->map_file_name) free(mapfile);
    if (res != 0) {
	log_bb_error(res, "BB_AddFiles(<index_file>)");
	(void) BB_FreeTransferDef(tdef);
	return res;
    }

    res = BB_AddFiles(tdef, sf->data_file_name, sf->pfs_file_name,
		      BBFILEFLAGS(BBFileBundleID(1) |
				  BBTransferOrder(1) |
				  BBTransferType(BBTransferTypeBSCFS)));
    if (res != 0) {
	log_bb_error(res, "BB_AddFiles(<data_file>)");
	(void) BB_FreeTransferDef(tdef);
	return res;
    }

    res = BB_StartTransfer(tdef, handle);
    if (res != 0) {
	log_bb_error(res, "BB_StartTransfer");
	(void) BB_FreeTransferDef(tdef);
	return res;
    }
    (void) BB_FreeTransferDef(tdef);

    sf->transfer_handle = handle;

    return 0;
}

void *bscfs_init(struct fuse_conn_info *conn)
{
    FL_Write(FLAgent, BSCFS_init, "init called", 0,0,0,0);

    LOG(bscfsagent,trace) << "bscfs_init() called";
    LOG(bscfsagent,info)
	<< "bscfs_init(): max write is " << conn->max_write
	<< ", pagesize: "<< getpagesize();

    //chdir into the mount point
    if (bscfs_data.mount_fd < 0) {
	LOG(bscfsagent,error)
	    << "Mount point has not been opened properly. BSCFS exiting!";
	exit(-1);
    }
    fchdir(bscfs_data.mount_fd);
    close(bscfs_data.mount_fd);
    bscfs_data.mount_fd = -1;

    // We use this bscfsAgent's node number as the agent's contribId.
    // For a prefetch, BBIO_BSCFS in bbServer uses the contribId to determine
    // what content to send to the requester.
    int res = BB_InitLibrary(bscfs_data.node_number, BBAPI_CLIENTVERSIONSTR);
    if (res != 0) {
	log_bb_error(res, "BB_InitLibrary");
	LOG(bscfsagent,error) << "BB_InitLibrary failed. BSCFS exiting!";
	exit(-1);
    }

    if (bscfs_data.pre_install_list[0] != '\0') {
	FILE *pil = fopen(bscfs_data.pre_install_list, "r");
	if (pil == NULL) {
	    LOG(bscfsagent,error)
		<< "bscfsAgent could not open pre-install list "
		<< bscfs_data.pre_install_list
		<< ": " << strerror(errno) << "; BSCFS exiting!";
	    exit(-1);
	}
	char *line = NULL;
	size_t line_size = 0;
	int line_count = 0;
	while (getline(&line, &line_size, pil) != -1) {
	    // Lines are expected to be of the form:
	    //     "<bscfs_path>"  "<index_path>"  "<data_path>"
	    // with arbitrary white space preceding each string.
	    line_count++;
	    char *bscfs_path, *index_path, *data_path, *path;
	    int len = strlen(line);
	    if (line[len-1] == '\n') line[len-1] = '\0';
	    char *p = &line[0];
	    while ((*p == ' ') || (*p == '\t')) p++;
	    if (*p != '"') goto parse_error;
	    bscfs_path = ++p;
	    while ((*p != '\0') && (*p != '"')) p++;
	    if (*p != '"') goto parse_error;
	    *p++ = '\0'; // terminate bscfs_path
	    while ((*p == ' ') || (*p == '\t')) p++;
	    if (*p != '"') goto parse_error;
	    index_path = ++p;
	    while ((*p != '\0') && (*p != '"')) p++;
	    if (*p != '"') goto parse_error;
	    *p++ = '\0'; // terminate index_path
	    while ((*p == ' ') || (*p == '\t')) p++;
	    if (*p != '"') goto parse_error;
	    data_path = ++p;
	    while ((*p != '\0') && (*p != '"')) p++;
	    if (*p != '"') goto parse_error;
	    *p++ = '\0'; // terminate data_path

	    path = bscfs_path + strlen(bscfs_data.mount_path);

	    if ((bscfs_path[0] == '\0') ||
		(strnlen(bscfs_path, BSCFS_PATH_MAX) == BSCFS_PATH_MAX) ||
		(strncmp(bscfs_path, bscfs_data.mount_path,
			 strlen(bscfs_data.mount_path)) != 0) ||
		(path[0] != '/'))
	    {
		LOG(bscfsagent,info)
		    << "bscfsAgent: bad <bscfs_file> name in "
		    << "pre-install file line " << line_count
		    << "; skipping";
		continue;
	    }

	    if ((index_path[0] == '\0') ||
		(strnlen(index_path, BSCFS_PATH_MAX) == BSCFS_PATH_MAX))
	    {
		LOG(bscfsagent,info)
		    << "bscfsAgent: bad <index_file> name in "
		    << "pre-install file line " << line_count
		    << "; skipping";
		continue;
	    }

	    if ((data_path[0] == '\0') ||
		(strnlen(data_path, BSCFS_PATH_MAX) == BSCFS_PATH_MAX))
	    {
		LOG(bscfsagent,info)
		    << "bscfsAgent: bad <data_file> name in "
		    << "pre-install file line " << line_count
		    << "; skipping";
		continue;
	    }

	    LOG(bscfsagent,info)
		<< "\nbscfsAgent pre-installing " << bscfs_path
		<< "\n    using index file " << index_path
		<< "\n    and data file " << data_path;
	    (void) activate(path, BSCFS_STABLE, index_path, data_path);
	    continue;

	parse_error:
	    LOG(bscfsagent,info)
		<< "bscfsAgent: parsing error in "
		<< "pre-install file line " << line_count
		<< "; skipping";
	    continue;
	}

	if (line != NULL) free(line);
	(void) fclose(pil);
    }

    return NULL;
}

void bscfs_destroy(void *user_data)
{
    FL_Write(FLAgent, BSCFS_destroy, "destroy called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_destroy() called";

    FILE *cul;
    std::map<std::string, shared_file_t*>::iterator item;

    if (bscfs_data.cleanup_list[0] == '\0') goto done;

    cul = fopen(bscfs_data.cleanup_list, "w");
    if (cul == NULL) {
	LOG(bscfsagent,error)
	    << "bscfsAgent could not open cleanup list "
	    << bscfs_data.cleanup_list
	    << ": " << strerror(errno)
	    << "; No post-job cleanup will occur.";
	goto done;
    }

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));

    for (item = bscfs_data.shared_files.begin();
	 item != bscfs_data.shared_files.end();
	 item++)
    {
	shared_file_t *sf = item->second;

	if (sf->state == BSCFS_FLUSH_PENDING) {
	    int res = start_flush(sf);
	    if (res == 0) {
		sf->state = BSCFS_FLUSHING;
		FL_sf_state(sf);
	    }
	}

	if (sf->state == BSCFS_FLUSHING) {
	    fprintf(cul, "%ld \"%s\" \"%s\" \"%s\"\n",
		    sf->transfer_handle, sf->cleanup_file_name,
		    sf->pfs_file_name, sf->map_file_name);
	}
    }

    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));

    fclose(cul);

done:
    int res = BB_TerminateLibrary();
    if (res != 0) {
	log_bb_error(res, "BB_TerminateLibrary");
	LOG(bscfsagent,error) << "BB_TerminateLibrary failed.";
	// No need to exit. We're shutting down anyway.
    }
    return;
}

int bscfs_create(const char *name, mode_t mode,
		 struct fuse_file_info *file_info)
{
    FL_Write(FLAgent, BSCFS_create, "create called", 0,0,0,0);
    LOG(bscfsagent,trace)
	<< "bscfs_create() called with flags " << file_info->flags
	<< ", name " << name;

    //check if this file has been already created in the BB
    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(name);

    if (sf != NULL) {
	// file found
	pthread_mutex_lock(&(sf->lock));
	// upgrade pfs_fd to O_RDWR if necessary
	int accmode = file_info->flags & O_ACCMODE;
	if (((sf->accmode == O_RDONLY) && (accmode != O_RDONLY)) ||
	    ((sf->accmode == O_WRONLY) && (accmode != O_WRONLY)))
	{
	    int newfd = open(sf->pfs_file_name, O_RDWR, 0);
	    if (newfd < 0) {
		int errno_save = errno;
		LOG(bscfsagent,info)
		    << "bscfs_create() failed to upgrade open"
		    << " pfs file " << sf->pfs_file_name
		    << " to O_RDWR: " << strerror(errno_save);
		pthread_mutex_unlock(&(sf->lock));
		pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
		return -errno_save;
	    }
	    close(sf->pfs_fd);
	    sf->pfs_fd = newfd;
	    sf->accmode = O_RDWR;
	}
	sf->open_count++;
	pthread_mutex_unlock(&(sf->lock));
    } else {
	// file not found
	if (strcmp(name, "/" BSCFS_CONTROL_FILE_NAME) == 0) {
	    LOG(bscfsagent,trace) << "control file opened";
	    sf = new shared_file_t();
	    FL_sf_new(sf);
	    shared_file_init(sf);
	    sf->file_name = strdup(name);
	    sf->accmode = file_info->flags & O_ACCMODE;
	    sf->state = BSCFS_CONTROL;
	    FL_sf_state(sf);
	} else {
	    int res;
	    sf = shared_file_create(&res, name, file_info->flags, mode);
	    if (sf == NULL) {
		pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
		return res;
	    }

	    sf->state = BSCFS_INACTIVE;
	    FL_sf_state(sf);
#if USE_INODE
	    sf->data_file_name = bscfs_data_path(sf);
	    sf->index_file_name = bscfs_index_path(sf);
#else
	    sf->data_file_name = bscfs_data_path(name);
	    sf->index_file_name = bscfs_index_path(name);
#endif
	    // Neither name can be NULL. File name length was checked
	    // in shared_file_create().
	}
	sf->open_count = 1;
    }
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
    file_info->direct_io = 1;
    file_info->fh = (uintptr_t) sf;
    return 0;
}

int bscfs_fsync(const char *name, int datasync,
		struct fuse_file_info *file_info)
{
    // For BSCFS, fsync() is a (successful) no-op. Implementing its full POSIX
    // semantics would undermine BSCFS's role in buffering shared-file content
    // and transferring it asynchronously to the PFS, while making it fail
    // could break applications unnecessarily.
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_fsync, "fsync called, sf %p", (uint64_t) sf, 0,0,0);
    LOG(bscfsagent,trace) << "bscfs_fsync() called on " << sf->file_name;
    if (sf->data_file_write_error != 0) return sf->data_file_write_error;
    return 0;
}

int bscfs_setxattr(const char *path, const char *name, const char *value,
		   size_t size, int flags)
{
    FL_Write(FLAgent, BSCFS_setxattr, "setxattr called", 0,0,0,0);
    LOG(bscfsagent,trace)
	<< "bscfs_setxattr() called on path: " << path << ", name: " << name;
    char *rpath = bscfs_pfs_path(path);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_setxattr() path too long: " << path;
	return -ENAMETOOLONG;
    }
    int res = setxattr(rpath, name, value, size, flags);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_setxattr() failed: " << strerror(errno_save);
	return -errno_save;
    }
    return 0;
}

int bscfs_getxattr(const char *path, const char *name, char *value,
		   size_t size)
{
    FL_Write(FLAgent, BSCFS_getxattr, "getxattr called", 0,0,0,0);
    LOG(bscfsagent,trace)
	<< "bscfs_getxattr() called on path: " << path << ", name: " << name;
    char *rpath = bscfs_pfs_path(path);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_getxattr() path too long: " << path;
	return -ENAMETOOLONG;
    }
    ssize_t res = getxattr(rpath, name, value, size);
    int errno_save = errno;
    free(rpath);
    if (res < 0) {
	LOG(bscfsagent,info)
	    << "bscfs_getxattr(): failed for path " << path
	    << ", attribute " << name
	    << ", with error: " << strerror(errno_save);
	return -errno_save;
    }
    return res;
}

int bscfs_listxattr(const char *name, char *list, size_t size)
{
    FL_Write(FLAgent, BSCFS_listxattr, "listxattr called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_listxattr() called on " << name;
    char *rpath = bscfs_pfs_path(name);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_listxattr() path too long: " << name;
	return -ENAMETOOLONG;
    }

    ssize_t res = listxattr(rpath, list, size);
    int errno_save = errno;
    free(rpath);
    if (res < 0) {
	LOG(bscfsagent,info)
	    << "bscfs_listxattr() failed: " << strerror(errno_save);
	return -errno_save;
    }
    return res;
}

int bscfs_removexattr(const char *path, const char *name)
{
    FL_Write(FLAgent, BSCFS_removexattr, "removexattr called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_removexattr() called on " << path;
    char *rpath = bscfs_pfs_path(path);
    if (rpath == NULL) {
	LOG(bscfsagent,info)
	    << "bscfs_removexattr() path too long: " << path;
	return -ENAMETOOLONG;
    }
    int res = removexattr(rpath, name);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_removexattr() failed: " << strerror(errno_save);
	return -errno_save;
    }
    return 0;
}

int bscfs_opendir(const char *name, struct fuse_file_info *file_info)
{
    FL_Write(FLAgent, BSCFS_opendir, "opendir called", 0,0,0,0);
    DIR *dp;
    LOG(bscfsagent,trace) << "bscfs_opendir() called on " << name;
    char *rpath = bscfs_pfs_path(name);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_opendir() path too long: " << name;
	return -ENAMETOOLONG;
    }
    dp = opendir(rpath);
    int errno_save = errno;
    free(rpath);
    if (dp == NULL) {
	LOG(bscfsagent,info)
	    << "bscfs_opendir() failed: " << strerror(errno_save);
	return -errno_save;
    }
    LOG(bscfsagent,trace) << "opendir() returned DIR " << dp;
    file_info->fh = (uintptr_t) dp;
    return 0;
}

int bscfs_releasedir(const char *path, struct fuse_file_info *file_info)
{
    FL_Write(FLAgent, BSCFS_releasedir, "releasedir called", 0,0,0,0);
    DIR *dp = (DIR *) file_info->fh;
    LOG(bscfsagent,trace) << "bscfs_releasedir() called on DIR " << dp;
    closedir(dp);
    return 0;
}

int bscfs_access(const char *name, int mask)
{
    FL_Write(FLAgent, BSCFS_access, "access called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_access() called on " << name;
    char *rpath = bscfs_pfs_path(name);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_access() path too long: " << name;
	return -ENAMETOOLONG;
    }

    int res = access(rpath, mask);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_access() failed: " << strerror(errno_save) ;
	return -errno_save;
    }
    return 0;
}

int bscfs_fgetattr(const char *name, struct stat *statbuf,
		   struct fuse_file_info *file_info)
{
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_fgetattr, "fgetattr called, sf %p", (uint64_t) sf, 0,0,0);
    LOG(bscfsagent,trace) << "bscfs_fgetattr() called on " << sf->file_name;
    if (sf->state == BSCFS_CONTROL) return -EPERM;
    pthread_mutex_lock(&(sf->lock));
    int res = fstat(sf->pfs_fd, statbuf);
    int errno_save = errno;
    if (res == 0) {
	if ((int64_t) sf->pfs_file_size > statbuf->st_size) {
	    statbuf->st_size = sf->pfs_file_size;
	    statbuf->st_blocks = (sf->pfs_file_size + 511) / 512;
	} else {
	    sf->pfs_file_size = statbuf->st_size;
	}
    } else {
	LOG(bscfsagent,info)
	    << "bscfs_fgetattr() failed: " << strerror(errno_save) ;
	res = -errno_save;
    }
    pthread_mutex_unlock(&(sf->lock));
    return res;
}

int bscfs_rename(const char *oldname, const char *newname)
{
    FL_Write(FLAgent, BSCFS_rename, "rename called", 0,0,0,0);
    LOG(bscfsagent,trace)
	<< "bscfs_rename() called, oldname " << oldname
	<< ", newname " << newname;

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(oldname);

    int res = 0;
    if (sf == NULL) {
	char *rpath_old = bscfs_pfs_path(oldname);
	if (rpath_old == NULL) {
	    LOG(bscfsagent,info)
		<< "bscfs_rename() path too long: " << oldname;
	    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	    return -ENAMETOOLONG;
	}
	char *rpath_new = bscfs_pfs_path(newname);
	if (rpath_new == NULL) {
	    LOG(bscfsagent,info)
		<< "bscfs_rename() path too long: " << newname;
	    free(rpath_old);
	    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	    return -ENAMETOOLONG;
	}
	if (rename(rpath_old, rpath_new) < 0) {
	    int errno_save = errno;
	    LOG(bscfsagent,info)
		<< "bscfs_rename() failed: " << strerror(errno_save);
	    res = -errno_save;
	}
	free(rpath_old);
	free(rpath_new);
    } else {
	pthread_mutex_lock(&(sf->lock));
	if (sf->state == BSCFS_INACTIVE) {
	    char *rpath_new = bscfs_pfs_path(newname);
	    if (rpath_new == NULL) {
		LOG(bscfsagent,info)
		    << "bscfs_rename() path too long: " << newname;
		pthread_mutex_unlock(&(sf->lock));
		pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
		return -ENAMETOOLONG;
	    }
	    if (rename(sf->pfs_file_name, rpath_new) < 0) {
		int errno_save = errno;
		LOG(bscfsagent,info)
		    << "bscfs_rename() failed: "
		    << strerror(errno_save);
		res = -errno_save;
		free(rpath_new);
	    } else {
		free(sf->file_name);
		free(sf->pfs_file_name);
		sf->file_name = strdup(newname);
		sf->pfs_file_name = rpath_new;
	    }
	} else {
	    LOG(bscfsagent,info)
		<< "bscfs_rename() file " << oldname << " is active";
	    res = -EPERM;
	}
	pthread_mutex_unlock(&(sf->lock));
    }
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
    return res;
}

int bscfs_truncate(const char *name, off_t offset)
{
    FL_Write(FLAgent, BSCFS_truncate, "truncate called", 0,0,0,0);
    LOG(bscfsagent,trace)
	<< "bscfs_truncate() called on " << name
	<< " with size: " << offset;

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(name);

    int res = 0;
    if (sf == NULL) {
	char *rpath = bscfs_pfs_path(name);
	if (rpath == NULL) {
	    LOG(bscfsagent,info)
		<< "bscfs_truncate: path too long: " << name;
	    res = -ENAMETOOLONG;
	} else {
	    res = truncate(rpath, offset);
	    if (res != 0) {
		int errno_save = errno;
		LOG(bscfsagent,info)
		    << "bscfs_truncate: truncate failed: "
		    << strerror(errno_save) ;
		res = -errno_save;
	    }
	    free(rpath);
	}
    } else {
	pthread_mutex_lock(&(sf->lock));
	if (sf->state == BSCFS_INACTIVE) {
	    res = truncate(sf->pfs_file_name, offset);
	    if (res != 0) {
		int errno_save = errno;
		LOG(bscfsagent,info)
		    << "bscfs_truncate: truncate failed: "
		    << strerror(errno_save) ;
		res = -errno_save;
	    }
	} else {
	    LOG(bscfsagent,info)
		<< "bscfs_truncate: file " << name << " is active";
	    res = -EPERM;
	}
	pthread_mutex_unlock(&(sf->lock));
    }
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
    return res;
}

int bscfs_ftruncate(const char *name, off_t offset,
		    struct fuse_file_info *file_info)
{
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_ftruncate, "ftruncate called, sf %p", (uint64_t) sf, 0,0,0);
    LOG(bscfsagent,trace)
	<< "bscfs_ftruncate() called on " << sf->file_name
	<< " with size: " << offset;

    int res;

    pthread_mutex_lock(&(sf->lock));
    if (sf->state == BSCFS_INACTIVE) {
	res = ftruncate(sf->pfs_fd, offset);
	if (res != 0) {
	    int errno_save = errno;
	    LOG(bscfsagent,info)
		<< "bscfs_ftruncate: ftruncate failed: "
		<< strerror(errno_save) ;
	    res = -errno_save;
	}
    } else {
	LOG(bscfsagent,info)
	    << "bscfs_ftruncate: file " << sf->file_name << " is active";
	res = -EPERM;
    }
    pthread_mutex_unlock(&(sf->lock));
    return res;
}

int bscfs_readlink(const char *name, char *buffer, size_t size)
{
    FL_Write(FLAgent, BSCFS_readlink, "readlink called", 0,0,0,0);
    LOG(bscfsagent,info)
	<< "bscfs_readlink() called on " << name << "; NOT IMPLEMENTED!";
    return -ENOSYS;
}


int bscfs_mkdir(const char *name, mode_t mode)
{
    FL_Write(FLAgent, BSCFS_mkdir, "mkdir called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_mkdir() called on " << name;
    char *rpath = bscfs_pfs_path(name);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_mkdir() path too long: " << name;
	return -ENAMETOOLONG;
    }
    mode_t rmode = mode|S_IFDIR;
    int res = mkdir(rpath,rmode);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_mkdir() failed: " << strerror(errno_save) ;
	return -errno_save;
    }
    return 0;
}


int bscfs_rmdir(const char *name)
{
    FL_Write(FLAgent, BSCFS_rmdir, "rmdir called", 0,0,0,0);
    LOG(bscfsagent,trace) << "bscfs_rmdir() called on " << name;
    char *rpath = bscfs_pfs_path(name);
    if (rpath == NULL) {
	LOG(bscfsagent,info) << "bscfs_rmdir() path too long: " << name;
	return -ENAMETOOLONG;
    }
    int res = rmdir(rpath);
    int errno_save = errno;
    free(rpath);
    if (res != 0) {
	LOG(bscfsagent,info)
	    << "bscfs_rmdir() failed: " << strerror(errno_save) ;
	return -errno_save;
    }
    return 0;
}

int bscfs_symlink(const char *dest_name, const char *src_name)
{
    FL_Write(FLAgent, BSCFS_symlink, "symlink called", 0,0,0,0);
    LOG(bscfsagent,info)
	<< "bscfs_symlink() called on " << dest_name << "; NOT IMPLEMENTED!";
    return -ENOSYS;
}

int bscfs_fsyncdir(const char *name, int sync,
		   struct fuse_file_info *file_info)
{
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_fsyncdir, "fsyncdir called, sf %p", (uint64_t) sf, 0,0,0);
    LOG(bscfsagent,info)
	<< "bscfs_fsyncdir() called on " << sf->file_name
	<< "; NOT IMPLEMENTED!";
    return -ENOSYS;
}

/*
int bscfs_lock(const char *name, struct fuse_file_info *file_info, int cmd,
               struct flock *lock)
{
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_lock, "lock called, sf %p", (uint64_t) sf, 0,0,0);
    LOG(bscfsagent,info)
	<< "bscfs_lock() called on " << sf->file_name << "; NOT IMPLEMENTED!";
    return -ENOSYS;

}
*/

int bscfs_utimens(const char *name , const struct timespec tv[2])
{
    FL_Write(FLAgent, BSCFS_utimens, "utimens called", 0,0,0,0);
    LOG(bscfsagent,info)
	<< "bscfs_utimens() called on " << name << "; NOT IMPLEMENTED!";
    return -ENOSYS;
}

int bscfs_bmap(const char *name, size_t blocksize, uint64_t *idx)
{
    FL_Write(FLAgent, BSCFS_bmap, "bmap called", 0,0,0,0);
    LOG(bscfsagent,info)
	<< "bscfs_bmap() called on " << name << "; NOT IMPLEMENTED!";
    return -ENOSYS;

}

int bscfs_ioctl(const char *name , int cmd, void *arg,
                struct fuse_file_info *file_info,
		unsigned int flags, void *data)
{
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_ioctl, "ioctl called, sf %p", (uint64_t) sf, 0,0,0);
    if (sf->state == BSCFS_CONTROL) {
	// ioctl on the control file
	switch ((uint32_t) cmd) {
	case BSCFS_IOC_START_LOCAL_FLUSH:
	    return bscfs_start_local_flush(data);
	case BSCFS_IOC_PREPARE_LOCAL_FLUSH:
	    return bscfs_prepare_local_flush(data);
	case BSCFS_IOC_START_LOCAL_PREFETCH:
	    return bscfs_start_local_prefetch(data);
	case BSCFS_IOC_CHECK_LOCAL_TRANSFER:
	    return bscfs_check_local_transfer(data);
	case BSCFS_IOC_GLOBAL_FLUSH_COMPLETED:
	    return bscfs_global_flush_completed(data);
	case BSCFS_IOC_FORGET:
	    return bscfs_forget(data);
	case BSCFS_IOC_QUERY_INTERNAL_FILES:
	    return bscfs_query_internal_files(data);
	case BSCFS_IOC_INSTALL_INTERNAL_FILES:
	    return bscfs_install_internal_files(data);
	case BSCFS_IOC_GET_PARAMETER:
		return bscfs_get_parameter(data);
	default:
	    // not a BSCFS operation
	    LOG(bscfsagent,info)
		<< "bscfs_ioctl(): unknown operation on control file";
	    return -EINVAL;
	}
    }

    LOG(bscfsagent,info)
	<< "bscfs_ioctl() called on " << sf->file_name << "; NOT IMPLEMENTED!";
    return -ENOSYS;
}

int bscfs_poll(const char *name, struct fuse_file_info *file_info,
               struct fuse_pollhandle *ph, unsigned *reventsp)
{
    shared_file_t *sf = (shared_file_t *) file_info->fh;
    FL_Write(FLAgent, BSCFS_poll, "poll called, sf %p", (uint64_t) sf, 0,0,0);
    LOG(bscfsagent,info)
	<< "bscfs_poll() called on " << sf->file_name << "; NOT IMPLEMENTED!";
    return -ENOSYS;
}

/*
 * Explicit BSCFS operations
 */

static void setup_local_flush(int start_transfer,
			      struct bscfs_setup_local_flush *request)
{
    int res;

    int pathname_len = strnlen(request->pathname, BSCFS_PATH_MAX);
    if (pathname_len == BSCFS_PATH_MAX) {
	LOG(bscfsagent,info)
	    << "setup_local_flush(): pathname too long";
	request->return_code = ENAMETOOLONG;
	return;
    }

    int mapfile_len = strnlen(request->mapfile, BSCFS_PATH_MAX);
    if (mapfile_len == BSCFS_PATH_MAX) {
	LOG(bscfsagent,info)
	    << "setup_local_flush(): mapfile name too long";
	request->return_code = ENAMETOOLONG;
	return;
    }

    int cleanup_script_len = strnlen(request->cleanup_script, BSCFS_PATH_MAX);
    if (cleanup_script_len == BSCFS_PATH_MAX) {
	LOG(bscfsagent,info)
	    << "setup_local_flush(): cleanup_script name too long";
	request->return_code = ENAMETOOLONG;
	return;
    }

    int mpath_len = strlen(bscfs_data.mount_path);

	if(strncmp(request->mapfile, bscfs_data.mount_path, mpath_len) == 0)
	{
		LOG(bscfsagent,info) << "setup_local_flush() called with bscfs-based mapfile: " << request->mapfile;
		request->return_code = ENOENT;
		return;
	}
	if(strncmp(request->cleanup_script, bscfs_data.mount_path, mpath_len) == 0)
	{
		LOG(bscfsagent,info) << "setup_local_flush() called with bscfs-based cleanup script: " << request->cleanup_script;
		request->return_code = ENOENT;
		return;
	}

    char *path = request->pathname + mpath_len;
    if(strncmp(path, "/./", 3)==0)
    {
        path += 2;
    }
    
    if ((strncmp(request->pathname, bscfs_data.mount_path, mpath_len) != 0) ||
	(path[0] != '/'))
    {
	LOG(bscfsagent,info)
	    << "setup_local_flush() called on non-BSCFS file: "
	    << request->pathname;
	request->return_code = ENOENT;
	return;
    }

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(path);

    if (sf == NULL) {
	sf = shared_file_create(&res, path, O_WRONLY, 0);
	if (sf == NULL) {
	    request->return_code = -res;
	    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	    return;
	}

	sf->state = BSCFS_INACTIVE;
	FL_sf_state(sf);
#if USE_INODE
	sf->data_file_name = bscfs_data_path(sf);
	sf->index_file_name = bscfs_index_path(sf);
#else
        sf->data_file_name = bscfs_data_path(name);
        sf->index_file_name = bscfs_index_path(name);
#endif
	sf->open_count = 0;
    }

    pthread_mutex_lock(&(sf->lock));

    if ((sf->state == BSCFS_PREFETCHING) || (sf->state == BSCFS_STABLE)) {
	LOG(bscfsagent,info)
	    << "setup_local_flush: file " << sf->file_name
	    << " in disallowed state " << sf->state;
	request->return_code = EPERM;
	goto done;
    }

    if (sf->state == BSCFS_INACTIVE) {
	res = activate_for_writing(sf, 1);
	if (res != 0) {
	    request->return_code = -res;
	    goto done;
	}
	sf->state = BSCFS_MODIFIED;
	FL_sf_state(sf);
    }

    if (sf->state == BSCFS_MODIFIED) {
	res = finalize_to_bb(sf);
	if (res != 0) {
	    request->return_code = -res;
	    goto done;
	}
	sf->map_file_name = strdup(request->mapfile);
	sf->cleanup_file_name = strdup(request->cleanup_script);
	sf->state = BSCFS_FLUSH_PENDING;
	FL_sf_state(sf);
    }

    if (!start_transfer) {
	request->handle = 0;
	request->return_code = 0;
	goto done;
    }

    if (sf->state == BSCFS_FLUSH_PENDING) {
	res = start_flush(sf);
	if (res != 0) {
	    request->return_code = res;
	    goto done;
	}
	sf->state = BSCFS_FLUSHING;
	FL_sf_state(sf);
    }

    request->handle = sf->transfer_handle;
    request->return_code = 0;

done:
    pthread_mutex_unlock(&(sf->lock));
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
}

int bscfs_start_local_flush(void *data)
{
    FL_Write(FLAgent, BSCFS_start_local_flush, "start_local_flush called", 0,0,0,0);
    struct bscfs_setup_local_flush *request =
	(struct bscfs_setup_local_flush *) data;

    LOG(bscfsagent,trace) << "bscfs_start_local_flush() called on "
			  << request->pathname;

    setup_local_flush(1, request);

    return 0;
}

int bscfs_prepare_local_flush(void *data)
{
    FL_Write(FLAgent, BSCFS_prepare_local_flush, "prepare_local_flush called", 0,0,0,0);
    struct bscfs_setup_local_flush *request =
	(struct bscfs_setup_local_flush *) data;

    LOG(bscfsagent,trace) << "bscfs_prepare_local_flush() called on "
			  << request->pathname;

    setup_local_flush(0, request);

    return 0;
}

int bscfs_start_local_prefetch(void *data)
{
    FL_Write(FLAgent, BSCFS_start_local_prefetch, "start_local_prefetch called", 0,0,0,0);
    struct bscfs_start_local_prefetch *request =
	(struct bscfs_start_local_prefetch *) data;

    LOG(bscfsagent,trace) << "bscfs_start_local_prefetch() called on "
			  << request->pathname;

    int res;

    int pathname_len = strnlen(request->pathname, BSCFS_PATH_MAX);
    if (pathname_len == BSCFS_PATH_MAX) {
	LOG(bscfsagent,info)
	    << "bscfs_start_local_prefetch(): pathname too long";
	request->return_code = ENAMETOOLONG;
	return 0;
    }

    int mapfile_len = strnlen(request->mapfile, BSCFS_PATH_MAX);
    if (mapfile_len == BSCFS_PATH_MAX) {
	LOG(bscfsagent,info)
	    << "bscfs_start_local_prefetch(): mapfile name too long";
	request->return_code = ENAMETOOLONG;
	return 0;
    }

    int mpath_len = strlen(bscfs_data.mount_path);
    char *path = request->pathname + mpath_len;
    if(strncmp(path, "/./", 3)==0)
    {
        path += 2;
    }

    if ((strncmp(request->pathname, bscfs_data.mount_path, mpath_len) != 0) ||
	(path[0] != '/'))
    {
	LOG(bscfsagent,info)
	    << "bscfs_start_local_prefetch() called on non-BSCFS file: "
	    << request->pathname;
	request->return_code = ENOENT;
	return 0;
    }

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(path);

    if (sf == NULL) {
	sf = shared_file_create(&res, path, O_RDONLY, 0);
	if (sf == NULL) {
	    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	    request->return_code = -res;
	    return 0;
	}

	sf->state = BSCFS_INACTIVE;
	FL_sf_state(sf);
#if USE_INODE
	sf->data_file_name = bscfs_data_path(sf);
	sf->index_file_name = bscfs_index_path(sf);
#else
        sf->data_file_name = bscfs_data_path(name);
        sf->index_file_name = bscfs_index_path(name);
#endif
	sf->open_count = 0;
    }

    pthread_mutex_lock(&(sf->lock));

    if ((sf->state == BSCFS_PREFETCHING) ||
	((sf->state == BSCFS_STABLE) && (sf->transfer_handle != 0))) {
	// Normally, start_local_prefetch is disallowed in STABLE state,
	// but it's possible this file has already been prefetched AND
	// awaited by another process. In that case we simply return the
	// handle from the completed prefetch.
	request->handle = sf->transfer_handle;
	request->return_code = 0;
	goto done;
    } else if (sf->state != BSCFS_INACTIVE) {
	LOG(bscfsagent,info)
	    << "bscfs_start_local_prefetch: file " << sf->file_name
	    << " in disallowed state " << sf->state;
	request->return_code = EPERM;
	goto done;
    }

    // tag allocation is serialized by shared_files_lock;
    BBTAG tag;
    tag = bscfs_data.next_transfer_tag++;

    BBTransferHandle_t handle;
    res = BB_GetTransferHandle(tag, 1, NULL, &handle);
    if (res != 0) {
	log_bb_error(res, "BB_GetTransferHandle");
	request->return_code = res;
	goto done;
    }

    BBTransferDef_t *tdef;
    res = BB_CreateTransferDef(&tdef);
    if (res != 0) {
	log_bb_error(res, "BB_CreateTransferDef");
	request->return_code = res;
	goto done;
    }

    res = BB_AddFiles(tdef, request->mapfile, sf->index_file_name,
		      BBFILEFLAGS(BBFileBundleID(1) |
				  BBTransferOrder(0) |
				  BBTransferType(BBTransferTypeBSCFS)));
    if (res != 0) {
	log_bb_error(res, "BB_AddFiles(<index_file>)");
	(void) BB_FreeTransferDef(tdef);
	request->return_code = res;
	goto done;
    }
    res = BB_AddFiles(tdef, sf->pfs_file_name, sf->data_file_name,
		      BBFILEFLAGS(BBFileBundleID(1) |
				  BBTransferOrder(1) |
				  BBTransferType(BBTransferTypeBSCFS)));
    if (res != 0) {
	log_bb_error(res, "BB_AddFiles(<data_file>)");
	(void) BB_FreeTransferDef(tdef);
	request->return_code = res;
	goto done;
    }

    res = BB_StartTransfer(tdef, handle);
    if (res != 0) {
	log_bb_error(res, "BB_StartTransfer");
	(void) BB_FreeTransferDef(tdef);
	request->return_code = res;
	goto done;
    }
    (void) BB_FreeTransferDef(tdef);

    sf->state = BSCFS_PREFETCHING;
    FL_sf_state(sf);

    sf->transfer_handle = handle;
    request->handle = handle;
    request->return_code = 0;

done:
    pthread_mutex_unlock(&(sf->lock));
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));

    return 0;
}

int bscfs_check_local_transfer(void *data)
{
    FL_Write(FLAgent, BSCFS_check_local_transfer, "check_local_transfer called", 0,0,0,0);
    struct bscfs_check_local_transfer *request =
	(struct bscfs_check_local_transfer *) data;

    LOG(bscfsagent,trace) << "bscfs_check_local_transfer() called on handle "
			  << request->handle;

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup_handle(request->handle);

    if (sf == NULL) {
	LOG(bscfsagent,info)
	    << "bscfs_check_local_transfer: handle not found";
	request->return_code = EBADR;
	pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	return 0;
    }

    pthread_mutex_lock(&(sf->lock));

    if ((sf->state == BSCFS_FLUSHING) || (sf->state == BSCFS_PREFETCHING)) {
	BBTransferInfo_t info;
	int res = BB_GetTransferInfo(sf->transfer_handle, &info);
	if (res != 0) {
	    log_bb_error(res, "BB_GetTransferInfo");
	    request->return_code = res;
	} else if ((info.status == BBNOTSTARTED) ||
		       (info.status == BBINPROGRESS)) {
	    request->return_code = EAGAIN;
	} else if (info.status == BBFULLSUCCESS) {
	    if (sf->state == BSCFS_FLUSHING) {
		free(sf->map_file_name);
		sf->map_file_name = NULL;
		free(sf->cleanup_file_name);
		sf->cleanup_file_name = NULL;
		sf->state = BSCFS_FLUSH_COMPLETED;
		FL_sf_state(sf);
		request->return_code = 0;
	    }
	    else { // (sf->state == BSCFS_PREFETCHING)
		res = activate_for_reading(sf, 0);
		if (res == 0) {
		    res = ingest_internal_files(sf);
		}
		if (res == 0) {
		    sf->state = BSCFS_STABLE;
		    FL_sf_state(sf);
		    request->return_code = 0;
		} else {
		    LOG(bscfsagent,info)
			<< "bscfs_check_local_transfer: "
			<< "activation failed for file "
			<< sf->file_name
			<< " after prefetch";
		    request->return_code = -res;
		}
	    }
	} else {
	    LOG(bscfsagent,info)
		<< "bscfs_check_local_transfer: transfer failed"
		<< " (status " << info.status << ")";
	    request->return_code = EIO;
	}
    }
    else if ((sf->state == BSCFS_FLUSH_COMPLETED) ||
	     (sf->state == BSCFS_STABLE))
    {
	// transfer already completed
	request->return_code = 0;
    } else {
	// state == INACTIVE, MODIFIED, FLUSH_PENDING, or CONTROL
	LOG(bscfsagent,info)
	    << "bscfs_check_local_transfer: file " << sf->file_name
	    << " in disallowed state " << sf->state;
	request->return_code = EPERM;
    }

    pthread_mutex_unlock(&(sf->lock));
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));

    return 0;
}

int bscfs_global_flush_completed(void *data)
{
    FL_Write(FLAgent, BSCFS_global_flush_completed, "global_flush_completed called", 0,0,0,0);
    struct bscfs_global_flush_completed *request =
	(struct bscfs_global_flush_completed *) data;

    LOG(bscfsagent,trace) << "bscfs_global_flush_completed() called on "
			  << request->pathname;

    int pathname_len = strnlen(request->pathname, BSCFS_PATH_MAX);
    if (pathname_len == BSCFS_PATH_MAX) {
	LOG(bscfsagent,info)
	    << "bscfs_global_flush_completed(): pathname too long";
	request->return_code = ENAMETOOLONG;
	return 0;
    }

    int mpath_len = strlen(bscfs_data.mount_path);
    char *path = request->pathname + mpath_len;
    if(strncmp(path, "/./", 3)==0)
    {
        path += 2;
    }

    if ((strncmp(request->pathname, bscfs_data.mount_path, mpath_len) != 0) ||
	(path[0] != '/'))
    {
	LOG(bscfsagent,info)
	    << "bscfs_global_flush_completed() called on non-BSCFS file: "
	    << request->pathname;
	request->return_code = ENOENT;
	return 0;
    }

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(path);

    if (sf == NULL) {
	LOG(bscfsagent,info)
	    << "bscfs_global_flush_completed() called on non-active file: "
	    << path;
	pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	request->return_code = ENOENT;
	return 0;
    }

    pthread_mutex_lock(&(sf->lock));

    if ((sf->state == BSCFS_FLUSH_COMPLETED) || (sf->state == BSCFS_STABLE)) {
	// No process should be using the transfer handle going forward,
	// and furthermore, we need to distinguish this case from a
	// STABLE prefetched file, which will still have a non-zero handle.
	sf->transfer_handle = 0;
	sf->state = BSCFS_STABLE;
	FL_sf_state(sf);
	request->return_code = 0;
    } else {
	// all other states
	LOG(bscfsagent,info)
	    << "bscfs_global_flush_completed: file " << path
	    << " in disallowed state " << sf->state;
	request->return_code = EPERM;
    }

    pthread_mutex_unlock(&(sf->lock));
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));

    return 0;
}

int bscfs_forget(void *data)
{
    FL_Write(FLAgent, BSCFS_forget, "forget called", 0,0,0,0);
    struct bscfs_forget *request =
	(struct bscfs_forget *) data;

    LOG(bscfsagent,trace) << "bscfs_forget() called on "
			  << request->pathname;

    int res;

    int pathname_len = strnlen(request->pathname, BSCFS_PATH_MAX);
    if (pathname_len == BSCFS_PATH_MAX) {
	LOG(bscfsagent,info)
	    << "bscfs_forget(): pathname too long";
	request->return_code = ENAMETOOLONG;
	return 0;
    }

    int mpath_len = strlen(bscfs_data.mount_path);
    char *path = request->pathname + mpath_len;
    if(strncmp(path, "/./", 3)==0)
    {
        path += 2;
    }

    if ((strncmp(request->pathname, bscfs_data.mount_path, mpath_len) != 0) ||
	(path[0] != '/'))
    {
	LOG(bscfsagent,info)
	    << "bscfs_forget() called on non-BSCFS file: "
	    << request->pathname;
	request->return_code = ENOENT;
	return 0;
    }

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(path);

    if (sf == NULL) {
	// file already inactive
	pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	request->return_code = 0;
	return 0;
    }

    pthread_mutex_lock(&(sf->lock));

    if ((sf->state == BSCFS_FLUSHING) || (sf->state == BSCFS_PREFETCHING)) {
	res = BB_CancelTransfer(sf->transfer_handle, BBSCOPETAG);
	if (res != 0) {
	    LOG(bscfsagent,debug)
		<< "bscfs_forget: ignoring BB_CancelTransfer failure";
	    log_bb_error(res, "BB_CancelTransfer");
	}
	sf->transfer_handle = 0;
    }

    if ((sf->state == BSCFS_FLUSH_PENDING) || (sf->state == BSCFS_FLUSHING)) {
	free(sf->map_file_name);
	sf->map_file_name = NULL;
	free(sf->cleanup_file_name);
	sf->cleanup_file_name = NULL;
    }

    if (sf->data_fd_read >= 0) {
	if (sf->read_buffer[0].busy) {
	    (void) await_aio_completion(&sf->read_buffer[0]);
	    sf->read_buffer[0].busy = 0;
	}
	sf->read_buffer[0].io_req.aio_offset = -1; // no valid data
	free(sf->read_buffer[0].buffer);
	sf->read_buffer[0].buffer = NULL;
	if (sf->read_buffer[1].busy) {
	    (void) await_aio_completion(&sf->read_buffer[1]);
	    sf->read_buffer[1].busy = 0;
	}
	sf->read_buffer[1].io_req.aio_offset = -1; // no valid data
	free(sf->read_buffer[1].buffer);
	sf->read_buffer[1].buffer = NULL;
	res = close(sf->data_fd_read);
	if (res != 0) {
	    LOG(bscfsagent,debug)
		<< "bscfs_forget: ignoring close(data_fd_read) "
		<< "failure: " << strerror(errno);
	}
    }

    if (sf->data_fd_write >= 0) {
	if (sf->write_buffer[0].busy) {
	    (void) await_aio_completion(&sf->write_buffer[0]);
	    sf->write_buffer[0].busy = 0;
	}
	free(sf->write_buffer[0].buffer);
	sf->write_buffer[0].buffer = NULL;
	if (sf->write_buffer[1].busy) {
	    (void) await_aio_completion(&sf->write_buffer[1]);
	    sf->write_buffer[1].busy = 0;
	}
	free(sf->write_buffer[1].buffer);
	sf->write_buffer[1].buffer = NULL;
	sf->current_write_buffer = -1;
	sf->write_buffer_offset = 0;
	sf->data_file_write_error = 0;
	sf->data_file_size = 0;
	sf->data_file_space = 0;
	res = close(sf->data_fd_write);
	if (res != 0) {
	    LOG(bscfsagent,debug)
		<< "bscfs_forget: ignoring close(data_fd_write) "
		<< "failure: " << strerror(errno);
	}
    }

    if ((sf->data_fd_read >= 0) || (sf->data_fd_write >= 0)) {
	res = unlink(sf->data_file_name);
	if (res != 0) {
	    LOG(bscfsagent,debug)
		<< "bscfs_forget: ignoring unlink(data_file_name) "
		<< "failure: " << strerror(errno);
	}
	sf->data_fd_read = -1;
	sf->data_fd_write = -1;
    }

    if (sf->index_fd >= 0) {
	free(sf->index);
	sf->index = NULL;
	res = close(sf->index_fd);
	if (res != 0) {
	    LOG(bscfsagent,debug)
		<< "bscfs_forget: ignoring close(index_fd) "
		<< "failure: " << strerror(errno);
	}
	res = unlink(sf->index_file_name);
	if (res != 0) {
	    LOG(bscfsagent,debug)
		<< "bscfs_forget: ignoring unlink(index_file_name) "
		<< "failure: " << strerror(errno);
	}
	sf->index_file_space = 0;
	sf->pfs_file_size = 0;
    }

    sf->state = BSCFS_INACTIVE;
    FL_sf_state(sf);

    if (sf->open_count == 0) {
	// remove sf from the shared-files list
	if (bscfs_data.shared_files.erase(sf->file_name) == 0) {
	    LOG(bscfsagent,error)
		<< "bscfs_forget(): file " << sf->file_name
		<< " not found in shared-files list. BSCFS exiting!";
	    exit(-1);
	}

	res = close(sf->pfs_fd);
	if (res != 0) {
	    LOG(bscfsagent,debug)
		<< "bscfs_forget: ignoring close(pfs_fd) "
		<< "failure: " << strerror(errno);
	}

	free(sf->data_file_name);
	free(sf->index_file_name);
	free(sf->pfs_file_name);
	free(sf->file_name);
	FL_sf_delete(sf);
	pthread_mutex_unlock(&(sf->lock)); // for coverity
	delete sf;
    } else {
	pthread_mutex_unlock(&(sf->lock));
    }

    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
    request->return_code = 0;
    return 0;
}

int bscfs_query_internal_files(void *data)
{
    FL_Write(FLAgent, BSCFS_query_internal_files, "query_internal_files called", 0,0,0,0);
    struct bscfs_query_internal_files *request =
	(struct bscfs_query_internal_files *) data;

    LOG(bscfsagent,trace) << "bscfs_query_internal_files() called on "
			  << request->pathname;

    if (request->files_length > BSCFS_INTERNAL_FILES_LENGTH_MAX) {
	LOG(bscfsagent,info)
	    << "bscfs_query_internal_files(): files_length "
	    << request->files_length << " exceeds maximum allowed";
	request->return_code = EINVAL;
	return 0;
    }

    int pathname_len = strnlen(request->pathname, BSCFS_PATH_MAX);
    if (pathname_len == BSCFS_PATH_MAX) {
	LOG(bscfsagent,info)
	    << "bscfs_query_internal_files(): pathname too long";
	request->return_code = ENAMETOOLONG;
	return 0;
    }

    int mpath_len = strlen(bscfs_data.mount_path);
    char *path = request->pathname + mpath_len;
    if(strncmp(path, "/./", 3)==0)
    {
        path += 2;
    }

    if ((strncmp(request->pathname, bscfs_data.mount_path, mpath_len) != 0) ||
	(path[0] != '/'))
    {
	LOG(bscfsagent,info)
	    << "bscfs_query_internal_files() called on non-BSCFS file: "
	    << request->pathname;
	request->return_code = ENOENT;
	return 0;
    }

    pthread_mutex_lock(&(bscfs_data.shared_files_lock));
    shared_file_t *sf = shared_file_lookup(path);

    if (sf == NULL) {
	LOG(bscfsagent,info)
	    << "bscfs_query_internal_files() called on non-active file: "
	    << path;
	pthread_mutex_unlock(&(bscfs_data.shared_files_lock));
	request->return_code = ENOENT;
	return 0;
    }

    pthread_mutex_lock(&(sf->lock));

    if ((sf->state == BSCFS_INACTIVE) ||
	(sf->state == BSCFS_PREFETCHING) ||
	(sf->state == BSCFS_CONTROL))
    {
	LOG(bscfsagent,info)
	    << "bscfs_query_internal_files: file " << sf->file_name
	    << " in disallowed state " << sf->state;
	request->return_code = EPERM;
	goto done;
    } else if (sf->state == BSCFS_MODIFIED) {
	int res = finalize_to_bb(sf);
	if (res != 0) {
	    request->return_code = -res;
	    goto done;
	}
    }

    uint64_t index_len, data_len;
    index_len = strnlen(sf->index_file_name, BSCFS_PATH_MAX);
    data_len = strnlen(sf->data_file_name, BSCFS_PATH_MAX);
    if (((index_len+1) + (data_len+1)) >= request->files_length) {
	LOG(bscfsagent,info)
	    << "bscfs_query_internal_files: space provided for "
	    << "file names insufficient";
	request->return_code = ENOMEM;
	goto done;
    }

    request->state = sf->state;

    memcpy(request->files, sf->index_file_name, (index_len+1));
    memcpy(request->files + (index_len+1), sf->data_file_name, (data_len+1));
    request->files[(index_len+1) + (data_len+1)] = '\0';

    request->return_code = 0;

done:
    pthread_mutex_unlock(&(sf->lock));
    pthread_mutex_unlock(&(bscfs_data.shared_files_lock));

    return 0;
}

int bscfs_install_internal_files(void *data)
{
    FL_Write(FLAgent, BSCFS_install_internal_files, "install_internal_files called", 0,0,0,0);
    struct bscfs_install_internal_files *request =
	(struct bscfs_install_internal_files *) data;

    LOG(bscfsagent,trace) << "bscfs_install_internal_files() called on "
			  << request->pathname;

    if ((request->state != BSCFS_STABLE) &&
	(request->state != BSCFS_MODIFIED))
    {
	LOG(bscfsagent,info)
	    << "bscfs_install_internal_files(): invalid state requested: "
	    << request->state;
	request->return_code = EINVAL;
	return 0;
    }

    if (request->files_length > BSCFS_INTERNAL_FILES_LENGTH_MAX) {
	LOG(bscfsagent,info)
	    << "bscfs_install_internal_files(): files_length "
	    << request->files_length << " exceeds maximum allowed";
	request->return_code = EINVAL;
	return 0;
    }

    char *index_path = request->files;
    uint64_t index_len = strnlen(index_path, request->files_length);
    if ((index_len == request->files_length) || (index_len >= BSCFS_PATH_MAX)) {
	LOG(bscfsagent,info)
	    << "bscfs_install_internal_files(): index path too long";
	request->return_code = ENAMETOOLONG;
	return 0;
    }

    char *data_path = index_path + index_len + 1;
    uint64_t data_len =
	strnlen(data_path, request->files_length - (index_len + 1));
    if ((data_len == (request->files_length - (index_len + 1))) ||
	(data_len >= BSCFS_PATH_MAX))
    {
	LOG(bscfsagent,info)
	    << "bscfs_install_internal_files(): data path too long";
	request->return_code = ENAMETOOLONG;
	return 0;
    }

    int pathname_len = strnlen(request->pathname, BSCFS_PATH_MAX);
    if (pathname_len == BSCFS_PATH_MAX) {
	LOG(bscfsagent,info)
	    << "bscfs_install_internal_files(): pathname too long";
	request->return_code = ENAMETOOLONG;
	return 0;
    }

    int mpath_len = strlen(bscfs_data.mount_path);
    char *path = request->pathname + mpath_len;
    if(strncmp(path, "/./", 3)==0)
    {
        path += 2;
    }

    if ((strncmp(request->pathname, bscfs_data.mount_path, mpath_len) != 0) ||
	(path[0] != '/'))
    {
	LOG(bscfsagent,info)
	    << "bscfs_install_internal_files() called on non-BSCFS file: "
	    << request->pathname;
	request->return_code = ENOENT;
	return 0;
    }

    int res = activate(path, (bscfs_file_state_t) request->state,
		       index_path, data_path);
    request->return_code = -res;
    return 0;
}

int bscfs_get_parameter(void *data)
{
    FL_Write(FLAgent, BSCFS_get_parameter, "get_parameter called", 0,0,0,0);
    struct bscfs_get_parameter* request = (struct bscfs_get_parameter *) data;

    LOG(bscfsagent,trace) << "BSCFS_get_parameter() called with " << request->parameter;
	if(strcmp(request->parameter, "BSCFS_MNT_PATH") == 0)
	{
		memcpy(request->value, bscfs_data.mount_path, sizeof(request->value));
		request->value[sizeof(request->value)-1] = 0;
		request->return_code = 0;
	}
	else if(strcmp(request->parameter, "BSCFS_PFS_PATH") == 0)
	{
		memcpy(request->value, bscfs_data.pfs_path, sizeof(request->value));
		request->value[sizeof(request->value)-1] = 0;
		request->return_code = 0;
	}
	else
	{
		request->return_code = EINVAL;
	}
	return 0;
}
