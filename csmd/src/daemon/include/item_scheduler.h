/*================================================================================

    csmd/src/daemon/include/item_scheduler.h

  © Copyright IBM Corporation 2015-2018. All Rights Reserved

    This program is licensed under the terms of the Eclipse Public License
    v1.0 as published by the Eclipse Foundation and available at
    http://www.eclipse.org/legal/epl-v10.html

    U.S. Government Users Restricted Rights:  Use, duplication or disclosure
    restricted by GSA ADP Schedule Contract with IBM Corp.

================================================================================*/

#ifndef CSMD_SRC_DAEMON_INCLUDE_ITEM_SCHEDULER_H_
#define CSMD_SRC_DAEMON_INCLUDE_ITEM_SCHEDULER_H_

#include <strings.h>

#include <algorithm>

#include "logging.h"
#include "include/csm_daemon_exception.h"


namespace csm {
namespace daemon {

struct BucketEntry {
  struct BucketEntry *_Next;
  uint64_t _Identifier;
  uint64_t _Interval;
  uint64_t _RemainingTicks;
};

class ItemScheduler
{
  typedef BucketEntry* BucketEntry_ptr;
  uint32_t _Size;         // last entry in the array

  uint32_t _Index;            // active index in the array
  BucketEntry *_ActiveQueue;  // head pointer of the active queue (nullptr if empty queue);

  BucketEntry_ptr *_Array;       // Array of entries
  uint32_t _ItemCount;  // keep track of the number of items

public:
  ItemScheduler( const uint32_t i_Size = 128 )
  : _Size( std::max( i_Size, 1u ) ), _Index( i_Size-1 ), _ItemCount( 0 )
  {
    _Array = new BucketEntry_ptr[ _Size ];
    bzero(_Array, sizeof(BucketEntry*) * _Size );
    _ActiveQueue = nullptr;
    LOG( csmd, debug ) << "Creating Itemscheduler: size=" << _Size;
  }
  ~ItemScheduler() noexcept(false)
  {
    BucketEntry *entry = nullptr;
    BucketEntry *to_delete = nullptr;
    for( uint32_t i=0; i<_Size; ++i )
    {
      if( _Array[i] == nullptr )
        continue;
      entry = _Array[i];
      to_delete = nullptr;
      while( entry != nullptr )
      {
        --_ItemCount;
        to_delete = entry;
        entry = Remove( entry, entry, &(_Array[i]) );
        delete to_delete;
      }
    }
    entry = _ActiveQueue;
    while( entry != nullptr )
    {
      --_ItemCount;
      to_delete = entry;
      entry = Remove( entry, entry, &_ActiveQueue );
      delete to_delete;
    }
    delete [] _Array;
    if( _ItemCount != 0 )
    {
      errno = _ItemCount;
      throw csm::daemon::Exception("Item underrun while Cleaning scheduled buckets.");
    }
  }

  BucketEntry* AddItem( uint64_t i_Identifier , uint64_t i_IntervalMySec, uint64_t i_Offset = 0 )
  {
    BucketEntry *entry = new BucketEntry();

    entry->_Identifier = i_Identifier;
    entry->_Interval = i_IntervalMySec;
    entry->_RemainingTicks = (entry->_Interval-1) / _Size;
    entry->_Next = nullptr;

    Queue( entry, i_Offset );

    LOG( csmd, debug ) << "Add Itemscheduler item:" << entry->_Identifier << " itv=" << entry->_Interval << " ticks=" << entry->_RemainingTicks;
    ++_ItemCount;
    return entry;
  }

  void DeleteItem( uint64_t i_Identifier )
  {
    BucketEntry *entry = nullptr;
    BucketEntry *prev  = nullptr;
    for( uint32_t i=0; i<_Size; ++i )
    {
      entry = _Array[ i ];
      prev = _Array[ i ];
      if( entry == nullptr )
        continue;
      while( entry != nullptr )
      {
        if( entry->_Identifier == i_Identifier )
        {
          Remove( prev, entry, &_Array[ i ] );
          delete entry;
          --_ItemCount;
          return;
        }
        else
        {
          prev = entry;
          entry = entry->_Next;
        }
      }
    }
    // check the current active queue since it's detached from the array
    entry = _ActiveQueue;
    prev = _ActiveQueue;
    if( entry->_Identifier == i_Identifier )
    {
      Remove( prev, entry, &_ActiveQueue );
      delete entry;
      --_ItemCount;
      return;
    }
    else
    {
      prev = entry;
      entry = entry->_Next;
    }
    LOG( csmd, error ) << "Error: ItemScheduler: Item " << i_Identifier << " not found.";
  }

  void RRForward()
  {
    _Index = ( _Index + 1 ) % _Size;
    _ActiveQueue = _Array[ _Index ];   // retrieve the current active queue for processing
    _Array[ _Index ] = nullptr;        // wipe array - it will get items for next iteration

    // pre-clean the active queue of any entries that have remaining ticks
    BucketEntry *entry = _ActiveQueue;
    BucketEntry *prev = _ActiveQueue;
    while( entry != nullptr )
    {
      BucketEntry *next = entry->_Next;

      // update counter, remove from active queue and queue into _Array
      if( entry->_RemainingTicks > 0 )
      {
        --( entry->_RemainingTicks );
        prev = Remove( prev, entry, &_ActiveQueue );
        Queue( entry, _Index );
        LOG( csmd, trace ) << "Delaying item=" << entry->_Identifier << " to " << entry->_RemainingTicks << ":" << entry->_Interval % _Size;
      }
      else  // or just skip to the next item
        if( entry != _ActiveQueue )
          prev = prev->_Next;
      entry = next;
    }
  }

  // get the head item of the currently active queue/index
  BucketEntry* GetNext()
  {
    BucketEntry *entry = _ActiveQueue;

    if( entry == nullptr )
      return nullptr;

    _ActiveQueue = _ActiveQueue->_Next;

    if( entry->_RemainingTicks == 0 )
    {
      Queue( entry, _Index + entry->_Interval );
      entry->_RemainingTicks = (entry->_Interval-1) / _Size;
    }
    else
      throw Exception("BUG: Scheduling protocol found item with remaining ticks after filtering.");

    return entry;
  }

  uint32_t GetItemCount() const
  {
    return _ItemCount;
  }

private:
  // queue the item to the front of the queue of index
  void Queue( BucketEntry *i_Item, const int i_Index )
  {
    BucketEntry *queue = _Array[ i_Index % _Size ];
    i_Item->_Next = queue;
    _Array[ i_Index % _Size ] = i_Item;
  }

  // removes an entry in the queue and returns a pointer to the predecessor (or the new head)
  BucketEntry* Remove( BucketEntry *i_Prev, BucketEntry *i_This, BucketEntry **io_Queue )
  {
    if( i_This == nullptr )
      return i_Prev;

    if( i_Prev == i_This )
    {
      i_Prev = i_This->_Next;
      // update the head of the queue if we had removed the first entry
      if( i_This == *io_Queue )
        *io_Queue = i_Prev;
    }
    else
      i_Prev->_Next = i_This->_Next;

    i_This->_Next = nullptr;
    return i_Prev;
  }

};

}
}

#endif /* CSMD_SRC_DAEMON_INCLUDE_ITEM_SCHEDULER_H_ */
