/*******************************************************************************
 |    bbio_BSCFS.cc
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

#include <vector>
#include <sys/file.h>

#include "bbio.h"
#include "bbio_BSCFS.h"
#include "bbinternal.h"
#include "bbserver_flightlog.h"
#include "bscfs_mapfile.h"
#include "tracksyscall.h"

#define ERROR(text) { \
    stringstream errorText; \
    errorText << text; \
    bberror << errloc(-1) \
	<< err("error.text", errorText.str()) \
	<< RAS(bb.internal.bscfs); \
}

#define SC_ERROR(text) { \
    stringstream errorText; \
    errorText << text; \
    bberror << errloc(-1) \
	<< err("error.text", errorText.str()) \
	<< err("error.errno", errno) \
	<< err("error.strerror", strerror(errno)) \
	<< RAS(bb.sc.bscfs); \
}

static int CmpRegion(const void* r1, const void* r2)
{
    bscfs_mapfile_region_t* reg1 = (bscfs_mapfile_region_t*) r1;
    bscfs_mapfile_region_t* reg2 = (bscfs_mapfile_region_t*) r2;

    return (reg1->offset < reg2->offset) ? -1 :
	    ((reg1->offset > reg2->offset) ? 1 : 0);
}

int BBIO_BSCFS::close(uint32_t pFileIndex)
{
    FL_Write(FLXfer, BSCFS_close, "BBIO_BSCFS::close(%p, pFileIndex %ld)", (uint64_t) this, pFileIndex, 0,0);
    LOG(bb,info) << "BBIO_BSCFS::" << __func__ << "(" << this
	<< ", pFileIndex=" << pFileIndex
	<< ")";

    int rc = 0;

    if (pFileIndex == mapFileIndex)
    {
        // Index should be fully sent or received now.
        rc = closeFileIndexProcessing(pFileIndex);
    }
    else if (pFileIndex == sharedFileIndex)
    {
        rc = closeSharedFileProcessing(pFileIndex);

        // We're finished with both files and the index.
        closeCleanUp();
    }
    else
    {
        ERROR("Unexpected file index: " << pFileIndex);
        rc = -1;
    }

    return rc;
}

void BBIO_BSCFS::closeAllFileHandles()
{
    closeCleanUp();

    return;
}

void BBIO_BSCFS::closeCleanUp()
{
    if (index)
    {
        free(index);
        index = NULL;
    }

    if (mapFileHandle)
    {
        delete mapFileHandle;
        mapFileHandle = NULL;
    }

    if (sharedFileHandle)
    {
        delete sharedFileHandle;
        sharedFileHandle = NULL;
    }

    mapFileIndex = -1u;
    sharedFileIndex = -1u;

    return;
}

int BBIO_BSCFS::closeFileIndexProcessing(uint32_t pFileIndex)
{
    // Convert index to host-endian format, if necessary, so it can be
    // used in handling the data file.
    if (le64toh(0x0123456789abcdefull) != 0x0123456789abcdefull)
    {
	index->node = le32toh(index->node);
	index->node_count = le32toh(index->node_count);
	index->normalized = le32toh(index->normalized);
	index->finalized = le32toh(index->finalized);
	index->mapping_count = le64toh(index->mapping_count);

	for (uint64_t i = 0; i < index->mapping_count; i++)
	{
	    bscfs_mapping_t* m = &index->mapping[i];
	    m->sf_offset = le64toh(m->sf_offset);
	    m->df_offset = le64toh(m->df_offset);
	    m->length = le64toh(m->length);
	}
    }

    if (writing)
    {
	uint32_t sourceIndex = BBTransferDef::getSourceIndex(mapFileIndex);
	indexBytes = BBTransferDef::getTotalTransferSize(transferDef,
							 sourceIndex);
	if (indexBytes != BSCFS_INDEX_SIZE(index->mapping_count))
	{
	    ERROR("Size inconsistency for index file");
	    return -1;
	}

	if (!index->finalized)
	{
	    ERROR("Index file expected to be finalized");
	    return -1;
	}

	indexComplete = true;
    }

    return 0;
}

int BBIO_BSCFS::closeSharedFileProcessing(uint32_t pFileIndex)
{
    if (writing && (mapFileHandle != NULL))
    {
	// We're done writing to the shared file. Now update the mapfile
	// if one was specified.

	// Re-use the index mapping space to construct the mapfile region
	// array. Mapfile regions are smaller than index mappings, but we
	// still have to be careful: region[0].offset and
	// mapping[0].sf_offset are in the same location.
	bscfs_mapfile_region_t* region =
	    (bscfs_mapfile_region_t*) index->mapping;
	for (uint64_t i = 0; i < index->mapping_count; i++)
	{
	    region[i].offset = htole64(index->mapping[i].sf_offset);
	    region[i].length = htole64(index->mapping[i].length);
	}
	uint64_t region_count = index->mapping_count;
	uint64_t total_data_size = 0;
	if (region_count > 0)
	{
	    // sort the regions into file-offset order
	    qsort(region, region_count, sizeof(bscfs_mapfile_region_t),
		  CmpRegion);
	    // merge adjacent regions
	    total_data_size = region[0].length;
	    uint64_t current = 0;
	    for (uint64_t i = 1; i < region_count; i++)
	    {
		total_data_size += region[i].length;
		if (region[i].offset ==
			(region[current].offset + region[current].length))
		{
		    // region[i] is adjacent to current region; simply
		    // increment length
		    region[current].length += region[i].length;
		}
		else
		{
		    // region[i] is not adjacent; start a new current region
		    current++;
		    if (current != i) region[current] = region[i];
		}
	    }
	    region_count = current + 1; // update count
	}

	int map = mapFileHandle->getfd();
	ssize_t rc;
	struct flock map_lock;
	map_lock.l_whence = SEEK_SET;
	map_lock.l_start = 0;
	map_lock.l_len = 0; // Lock entire file

	map_lock.l_type = F_WRLCK;
	rc = ::fcntl(map, F_SETLKW, &map_lock);
	if (rc < 0)
	{
	    SC_ERROR("fcntl(F_WRLCK) failed");
	    return -1;
	}

	uint64_t region_offset = ::lseek(map, 0, SEEK_END);
	if (region_offset == ((uint64_t) -1))
	{
	    SC_ERROR("lseek(SEEK_END) failed");
	    return -1;
	}

	if (region_offset == 0)
	{
	    uint64_t node_count = index->node_count;
	    uint64_t header_size =
		sizeof(bscfs_mapfile_header_t) +
		    (node_count * sizeof(bscfs_mapfile_node_header_t));
	    bscfs_mapfile_header_t* header =
		(bscfs_mapfile_header_t*) malloc(header_size);
	    header->node_count = htole64(node_count);
	    for (uint64_t n = 0; n < node_count; n++)
	    {
		header->node[n].region_offset = 0;
		header->node[n].region_count = 0;
		header->node[n].total_data_size = 0;
	    }

	    rc = ::write(map, header, header_size);
	    if (rc != (ssize_t) header_size)
	    {
		SC_ERROR("write(header) failed");
		free(header);
		return -1;
	    }

	    region_offset = header_size;

	    free(header);
	}

	uint64_t region_size = region_count *
				    sizeof(bscfs_mapfile_region_t);
	rc = ::ftruncate(map, region_offset + region_size);
	if (rc < 0)
	{
	    SC_ERROR("ftruncate(region_end) failed");
	    return -1;
	}

	map_lock.l_type = F_UNLCK;
	rc = ::fcntl(map, F_SETLK, &map_lock);
	if (rc < 0)
	{
	    SC_ERROR("fcntl(F_UNLCK) failed");
	    return -1;
	}

	char* buffer = (char*) region;
	uint64_t remainder = region_size;
	while (remainder > 0)
	{
	    rc = ::write(map, buffer, remainder);
	    if (rc < 0)
	    {
		SC_ERROR("write(region) failed");
		return -1;
	    }
	    buffer += rc;
	    remainder -= rc;
	}

	bscfs_mapfile_node_header_t node_header;

	node_header.region_offset = htole64(region_offset);
	node_header.region_count = htole64(region_count);
	node_header.total_data_size = htole64(total_data_size);

	uint64_t node = (contribId & 0xffff);
	uint64_t node_offset = offsetof(bscfs_mapfile_header_t, node) +
			    (node * sizeof(bscfs_mapfile_node_header_t));


	TrackSyscall nowTrack(TrackSyscall::pwritesyscall, map,__LINE__,sizeof(node_header) ,node_offset);
	rc = ::pwrite(map, &node_header, sizeof(node_header), node_offset);
	nowTrack.clearTrack();

	if (rc != sizeof(node_header))
	{
	    SC_ERROR("pwrite(node_header) failed");
	    return -1;
	}
	TrackSyscall nowTrack2(TrackSyscall::fsyncsyscall, map, __LINE__);
	(void) ::fsync(map);
	nowTrack2.clearTrack();
    }

    return 0;
}

int BBIO_BSCFS::fstat(uint32_t pFileIndex, struct stat* pStats)
{
    FL_Write(FLXfer, BSCFS_fstat, "BBIO_BSCFS::fstat(%p, pFileIndex %ld)", (uint64_t) this, pFileIndex, 0,0);
    LOG(bb,info) << "BBIO_BSCFS::" << __func__ << "(" << this
	<< ", pFileIndex=" << pFileIndex
	<< ")";

    ssize_t bytes = 0;
    if (pFileIndex == mapFileIndex)
    {
	bytes = indexBytes;
    }
    else if (pFileIndex == sharedFileIndex)
    {
	if (mapFileIndex == -1u)
	{
	    ERROR("mapfile has not been opened");
	    return -1;
	}
	bytes = dataBytes;
    }
    else
    {
	ERROR("Unexpected file index: " << pFileIndex);
	return -1;
    }

    // fake a read-write regular file
    pStats->st_dev = 0;
    pStats->st_ino = 0;
    pStats->st_mode = S_IFREG | S_IRUSR | S_IWUSR;
    pStats->st_nlink = 1;
    pStats->st_uid = geteuid();
    pStats->st_gid = getegid();
    pStats->st_rdev = 0;
    pStats->st_size = bytes;
    pStats->st_blksize = 512;
    pStats->st_blocks = (bytes + 511) / 512;
    pStats->st_atime = 0;
    pStats->st_mtime = 0;
    pStats->st_ctime = 0;

    return 0;
}

int BBIO_BSCFS::open(uint32_t pFileIndex, uint64_t pBBFileFlags,
		     const string& pFileName, const mode_t pMode)
{
    FL_Write(FLXfer, BSCFS_open, "BBIO_BSCFS::open(%p, pFileIndex %ld, pBBFileFlags 0x%lx)", (uint64_t) this, pFileIndex, pBBFileFlags, 0);
    LOG(bb,info) << "BBIO_BSCFS::" << __func__ << "(" << this
	<< ", pFileIndex=" << pFileIndex
	<< ", pBBFileFlags=" << pBBFileFlags
	<< ", pFileName=" << pFileName
	<< ")";

    if (BBTransferTypeFromFlags(pBBFileFlags) != BBTransferTypeBSCFS)
    {
	ERROR("Bad transfer type: " << BBTransferTypeFromFlags(pBBFileFlags));
	return -1;
    }

    writing = ((pBBFileFlags & BBI_TargetSSD) == 0);

    if (BBTransferOrderFromFlags(pBBFileFlags) == 0)
    {
	if (mapFileIndex != -1u)
	{
	    ERROR("Mapfile already opened, index " << mapFileIndex);
	    return -1;
	}
	mapFileIndex = pFileIndex;
	 

	if (writing)
	{
	    // Allocate space for the incoming index
	    index = (bscfs_index_t*) malloc(MAX_INDEX_SIZE);
	    if (index == NULL)
	    {
		ERROR("malloc(MAX_INDEX_SIZE) failed");
		mapFileIndex = -1u;
		return -1;
	    }

	    // Applications are not required to specify a mapfile for a flush
	    // operation. bscfsAgents use "<no_mapfile>" as a flag to indicate
	    // that the application did not provide a mapfile name.
	    mapFileHandle = NULL;
	    if (strstr(pFileName.c_str(), "<no_mapfile>") == NULL)
	    {
		mapFileHandle = new filehandle(pFileName, O_CREAT|O_WRONLY,
					       S_IRUSR|S_IWUSR);
	    }
	}
	else
	{
	    // Instantiate an index for the client node from the just-opened
	    // mapfile.

	    // These limits are arbitrary.
	    const uint64_t MAX_NODE_COUNT = 16384;
	    const uint64_t MAX_REGION_COUNT = 1024 * 1024 * 1024;

	    mapFileHandle = new filehandle(pFileName, O_RDONLY, 0);
	    int map = mapFileHandle->getfd();
	    bscfs_mapfile_header_t header;
	    ssize_t rc;

	    TrackSyscall nowTrack(TrackSyscall::preadsyscall, map ,__LINE__,sizeof(header) , 0);
	    rc = ::pread(map, &header, sizeof(header), 0);
	    nowTrack.clearTrack();

	    if (rc != sizeof(header))
	    {
		SC_ERROR("pread(mapfile header) failed");
		delete mapFileHandle;
		mapFileHandle = NULL;
		mapFileIndex = -1u;
		return -1;
	    }

	    uint64_t node_count = le64toh(header.node_count);
	    if (node_count > MAX_NODE_COUNT)
	    {
		ERROR("bad node_count");
		delete mapFileHandle;
		mapFileHandle = NULL;
		mapFileIndex = -1u;
		return -1;
	    }

	    uint64_t node = (contribId & 0xffff);
	    uint64_t node_offset =
		offsetof(bscfs_mapfile_header_t, node) +
		    (node * sizeof(bscfs_mapfile_node_header_t));

	    bscfs_mapfile_node_header_t node_header = {0, 0, 0};
	    if (node < node_count) {

		TrackSyscall nowTrack(TrackSyscall::preadsyscall, map,__LINE__,sizeof(node_header) , node_offset);
		rc = ::pread(map, &node_header, sizeof(node_header),
			     node_offset);
		nowTrack.clearTrack();

		if (rc != sizeof(node_header))
		{
		    SC_ERROR("pread(node_header) failed");
		    delete mapFileHandle;
		    mapFileHandle = NULL;
		    mapFileIndex = -1u;
		    return -1;
		}
	    }

	    uint64_t region_offset = le64toh(node_header.region_offset);
	    uint64_t region_count = le64toh(node_header.region_count);
	    uint64_t total_data_size = le64toh(node_header.total_data_size);

	    if (region_count > MAX_REGION_COUNT) {
		ERROR("bad region_count");
		delete mapFileHandle;
		mapFileHandle = NULL;
		mapFileIndex = -1u;
		return -1;
	    }
	    uint64_t region_size =
		region_count * sizeof(bscfs_mapfile_region_t);

	    uint64_t mapfile_end = ::lseek(map, 0, SEEK_END);
	    if (mapfile_end == ((uint64_t) -1))
	    {
		SC_ERROR("lseek(SEEK_END) failed");
		delete mapFileHandle;
		mapFileHandle = NULL;
		mapFileIndex = -1u;
		return -1;
	    }

	    uint64_t node_headers_size =
		node_count * sizeof(bscfs_mapfile_node_header);
	    if ((region_offset < (sizeof(header) + node_headers_size)) ||
		((region_offset + region_size) > mapfile_end))
	    {
		ERROR("bad region_offset or region_count");
		delete mapFileHandle;
		mapFileHandle = NULL;
		mapFileIndex = -1u;
		return -1;
	    }

	    indexBytes = BSCFS_INDEX_SIZE(region_count);
	    index = (bscfs_index_t*) malloc(indexBytes);
	    if (index == NULL)
	    {
		ERROR("malloc(index) failed");
		delete mapFileHandle;
		mapFileHandle = NULL;
		mapFileIndex = -1u;
		return -1;
	    }

	    dataBytes = total_data_size;

	    // Read the region info into the end of the index space.
	    bscfs_mapfile_region_t* region = (bscfs_mapfile_region_t*)
		(((uint64_t) index) + indexBytes - region_size);

	    char* buffer = (char*) region;
	    uint64_t offset = region_offset;
	    uint64_t remainder = region_size;
	    while (remainder > 0)
	    {
		TrackSyscall nowTrack(TrackSyscall::preadsyscall, map ,__LINE__,remainder , offset);
		rc = ::pread(map, buffer, remainder, offset);
		nowTrack.clearTrack();

		if (rc < 0)
		{
		    SC_ERROR("pread(region) failed");
		    free(index);
		    index = NULL;
		    delete mapFileHandle;
		    mapFileHandle = NULL;
		    mapFileIndex = -1u;
		    return -1;
		}
		buffer += rc;
		offset += rc;
		remainder -= rc;
	    }

	    // Convert the region info to index mapping format, taking care
	    // because the last mapping overlaps the last two regions. We keep
	    // the index in little-endian form, ready for transmission.
	    uint64_t df_offset = 0;
	    for (uint64_t i = 0; i < region_count; i++)
	    {
		index->mapping[i].sf_offset = region[i].offset;
		index->mapping[i].df_offset = htole64(df_offset);
		index->mapping[i].length = region[i].length;
		df_offset += le64toh(region[i].length);
	    }

	    if (df_offset != dataBytes)
	    {
		ERROR("inconsistent data size for node " << node
			<< " in mapfile " << pFileName);
		free(index);
		index = NULL;
		delete mapFileHandle;
		mapFileHandle = NULL;
		mapFileIndex = -1u;
		return -1;
	    }

	    for (uint64_t i = 0; i < (region_count - 1); i++)
	    {
		if ((le64toh(index->mapping[i].sf_offset) +
				    le64toh(index->mapping[i].length)) >=
		    le64toh(index->mapping[i+1].sf_offset))
		{
		    ERROR("overlapping mappings for node " << node
			    << " in mapfile " << pFileName);
		    free(index);
		    index = NULL;
		    delete mapFileHandle;
		    mapFileHandle = NULL;
		    mapFileIndex = -1u;
		    return -1;
		}
	    }

	    index->node = htole32(node);
	    index->node_count = htole32(node_count);
	    index->normalized = htole32(1);
	    index->finalized = htole32(1);
	    index->mapping_count = htole64(region_count);
	}
    }
    else if (BBTransferOrderFromFlags(pBBFileFlags) == 1)
    {
	if (sharedFileIndex != -1u)
	{
	    ERROR("Shared file already opened, index " << sharedFileIndex);
	    return -1;
	}
	sharedFileIndex = pFileIndex;
	if (writing)
	{
	    sharedFileHandle = new filehandle(pFileName, O_CREAT|O_WRONLY,
					      S_IRUSR|S_IWUSR);
	}
	else
	{
	    sharedFileHandle = new filehandle(pFileName, O_RDONLY, 0);
	}
    }
    else
    {
	ERROR("Unexpected transfer order: "
		<< BBTransferOrderFromFlags(pBBFileFlags));
	return -1;
    }

    return 0;
}

ssize_t BBIO_BSCFS::pread(uint32_t pFileIndex, char* pBuffer,
			  size_t pMaxBytesToRead, off_t& pOffset)
{
    FL_Write(FLXfer, BSCFS_pread, "BBIO_BSCFS::pread(%p, pFileIndex %ld, pMaxBytesToRead 0x%lx, pOffset 0x%lx)", (uint64_t) this, pFileIndex, pMaxBytesToRead, pOffset);
    LOG(bb,trace) << "BBIO_BSCFS::" << __func__ << "(" << this
	<< ", pFileIndex=" << pFileIndex
	<< ", pBuffer=" << (void*) pBuffer
	<< ", pMaxBytesToRead=" << pMaxBytesToRead
	<< ", pOffset=" << pOffset
	<< ")";

    if (pFileIndex == mapFileIndex)
    {
	if (pMaxBytesToRead > (indexBytes - pOffset))
	{
	    pMaxBytesToRead = (indexBytes - pOffset);
	}
	memcpy(pBuffer, ((char *) index) + pOffset, pMaxBytesToRead);
    }
    else if (pFileIndex == sharedFileIndex)
    {
	if (mapFileIndex == -1u)
	{
	    ERROR("mapfile has not been opened");
	    return -1;
	}

	if (pOffset >= (off_t) dataBytes)
	{
	    ERROR("offset exceeds data file size");
	    return -1;
	}

	if (pMaxBytesToRead > (dataBytes - pOffset))
	{
	    pMaxBytesToRead = (dataBytes - pOffset);
	}

	char* buffer = pBuffer;
	uint64_t offset = pOffset;
	uint64_t remainder = pMaxBytesToRead;

	// Find the entry number of the mapping that contains offset.
	// By construction, there are no holes between mappings in terms of
	// data-file offsets, so we don't have to worry about offsets falling
	// between mappings.
	int64_t base = index->mapping_count;
	int64_t delta = base + 1;
	while (delta > 1)
	{
	    delta = (delta + 1) / 2;
	    int64_t next = base - delta;
	    if ((next >= 0) &&
		(offset < index->mapping[next].df_offset)) base = next;
	}
	int64_t idx = base - 1;
	

	while (remainder > 0)
	{
	    bscfs_mapping_t* m = &index->mapping[idx];
	    uint64_t end = m->df_offset + m->length;
	    // Read as much as we can from the shared file to the buffer,
	    // using the current mapping.
	    uint64_t chunk = end - offset;
	    if (chunk > remainder) chunk = remainder;

	    TrackSyscall nowTrack(TrackSyscall::preadsyscall, sharedFileHandle->getfd() ,__LINE__,chunk , m->sf_offset + (offset - m->df_offset));
	    ssize_t rc = ::pread(sharedFileHandle->getfd(), buffer, chunk,
				 m->sf_offset + (offset - m->df_offset));
	    nowTrack.clearTrack();

	    if (rc != (ssize_t) chunk)
	    {
		if (rc < 0)
		{
		    if (pMaxBytesToRead > remainder)
		    {
			rc = pMaxBytesToRead - remainder;
			LOG(bb,debug) << "BBIO_BSCFS::" << __func__ << ": "
			    << "pread failed (" << strerror(errno)
			    << "), but returning partial success: " << rc;
			return rc;
		    }
		    SC_ERROR("pread failed");
		    return -1;
		}
		LOG(bb,debug) << "BBIO_BSCFS::" << __func__ << ": "
		    << "pread returned " << rc << "; expected " << chunk;
		chunk = rc; // record what we actually read, and try again
	    }
	    else
	    {
		// move to the next mapping
		idx++;
	    }
	    offset += chunk;
	    buffer += chunk;
	    remainder -= chunk;
	}
    }
    else
    {
	ERROR("Unexpected file index: " << pFileIndex);
	return -1;
    }

    return pMaxBytesToRead;
}

int BBIO_BSCFS::fsync(uint32_t pFileIndex)
{
    FL_Write(FLXfer, BSCFS_fsync, "BBIO_BSCFS::fsync(%p, pFileIndex %ld)", (uint64_t) this, pFileIndex, 0,0);
    LOG(bb,info) << "BBIO_BSCFS::" << __func__ << "(" << this
    << ", pFileIndex=" << pFileIndex
    << ")";

    if (pFileIndex == mapFileIndex)
    {
        // Nothing to do for the mapfile. We won't actually write to
        // it until the shared file is closed.
    }
    else if (pFileIndex == sharedFileIndex)
    {
         
        TrackSyscall nowTrack(TrackSyscall::fsyncsyscall, sharedFileHandle->getfd(), __LINE__);
        (void) ::fsync(sharedFileHandle->getfd());
        nowTrack.clearTrack();
    }
    else
    {
        ERROR("Unexpected file index: " << pFileIndex);
        return -1;
    }

    return 0;
}

ssize_t BBIO_BSCFS::pwrite(uint32_t pFileIndex, const char* pBuffer,
			   size_t pMaxBytesToWrite, off_t& pOffset)
{
    FL_Write(FLXfer, BSCFS_pwrite, "BBIO_BSCFS::pwrite(%p, pFileIndex %ld, pMaxBytesToWrite 0x%lx, pOffset 0x%lx)", (uint64_t) this, pFileIndex, pMaxBytesToWrite, pOffset);
    LOG(bb,trace) << "BBIO_BSCFS::" << __func__ << "(" << this
	<< ", pFileIndex=" << pFileIndex
	<< ", pBuffer=" << (void*) pBuffer
	<< ", pMaxBytesToWrite=" << pMaxBytesToWrite
	<< ", pOffset=" << pOffset
	<< ")";

    if (pFileIndex == mapFileIndex)
    {
	if ((pOffset + pMaxBytesToWrite) > MAX_INDEX_SIZE) {
	    ERROR("Arriving index too large");
	    return -1;
	}
	memcpy(((char*) index) + pOffset, pBuffer, pMaxBytesToWrite);
    }
    else if (pFileIndex == sharedFileIndex)
    {
	if (!indexComplete)
	{
	    ERROR("Data arriving with index not yet complete");
	    return -1;
	}

	const char* buffer = pBuffer;
	uint64_t offset = pOffset;
	uint64_t remainder = pMaxBytesToWrite;

	// Find the entry number of the mapping that contains offset, or,
	// if there is no such mapping, the mapping that precedes offset.
	// Use -1 if offset is smaller than any existing mapping.
	int64_t base = index->mapping_count;
	int64_t delta = base + 1;
	while (delta > 1)
	{
	    delta = (delta + 1) / 2;
	    int64_t next = base - delta;
	    if ((next >= 0) &&
		(offset < index->mapping[next].df_offset)) base = next;
	}
	int64_t idx = base - 1;

	bscfs_mapping_t* m = NULL;
	uint64_t end = 0;
	if (idx >= 0)
	{
	    m = &index->mapping[idx];
	    end = m->df_offset + m->length;
	}

	 

	while (remainder > 0)
	{
	    uint64_t chunk;
	    if (offset < end)
	    {
		// offset is inside the current mapping; write as much
		// as we can from the buffer to the shared file
		chunk = end - offset;
		if (chunk > remainder) chunk = remainder;

		TrackSyscall nowTrack(TrackSyscall::pwritesyscall, sharedFileHandle->getfd(),__LINE__,chunk ,m->sf_offset + (offset - m->df_offset));
		ssize_t rc = ::pwrite(sharedFileHandle->getfd(), buffer, chunk,
				      m->sf_offset + (offset - m->df_offset));
		nowTrack.clearTrack();

		if (rc != (ssize_t) chunk)
		{
		    if (rc < 0)
		    {
			if (pMaxBytesToWrite > remainder)
			{
			    rc = pMaxBytesToWrite - remainder;
			    LOG(bb,debug) << "BBIO_BSCFS::" << __func__ << ": "
				<< "pwrite failed (" << strerror(errno)
				<< "), but returning partial success: " << rc;
			    return rc;
			}
			SC_ERROR("pwrite failed");
			return -1;
		    }
		    LOG(bb,debug) << "BBIO_BSCFS::" << __func__ << ": "
			<< "pwrite returned " << rc << "; expected " << chunk;
		    chunk = rc; // record what we actually wrote, and try again
		}
	    }
	    else
	    {
		// offset is beyond the current mapping; move to the next
		// mapping and skip any data that isn't indexed
		chunk = remainder;
		idx++;
		if (idx < (int64_t) index->mapping_count)
		{
		    m = &index->mapping[idx];
		    end = m->df_offset + m->length;
		    if (chunk > (m->df_offset - offset))
		    {
			chunk = m->df_offset - offset;
		    }
		}
	    }
	    offset += chunk;
	    buffer += chunk;
	    remainder -= chunk;
	}
    }
    else
    {
	ERROR("Unexpected file index: " << pFileIndex);
	return -1;
    }

    return pMaxBytesToWrite;
}
