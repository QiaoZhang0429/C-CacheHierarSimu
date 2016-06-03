//========================================================//
//  cache.c                                               //
//  Source file for the Cache Simulator                   //
//                                                        //
//  Implement the I-cache, D-Cache and L2-cache as        //
//  described in the README                               //
//========================================================//

#include "cache.h"
#include "limits.h"
#include <stdio.h>

//
// TODO:Student Information
//
const char *studentName = "Qiao Zhang";
const char *studentID   = "A53095965";
const char *email       = "qiz121@ucsd.edu";

//------------------------------------//
//        Cache Configuration         //
//------------------------------------//

uint32_t icacheSets;     // Number of sets in the I$
uint32_t icacheAssoc;    // Associativity of the I$
uint32_t icacheHitTime;  // Hit Time of the I$

uint32_t dcacheSets;     // Number of sets in the D$
uint32_t dcacheAssoc;    // Associativity of the D$
uint32_t dcacheHitTime;  // Hit Time of the D$

uint32_t l2cacheSets;    // Number of sets in the L2$
uint32_t l2cacheAssoc;   // Associativity of the L2$
uint32_t l2cacheHitTime; // Hit Time of the L2$
uint32_t inclusive;      // Indicates if the L2 is inclusive

uint32_t blocksize;      // Block/Line size
uint32_t memspeed;       // Latency of Main Memory

uint32_t blockoffset;
uint32_t icacheIndex;
uint32_t icacheTag;
uint32_t dcacheIndex;
uint32_t dcacheTag;
uint32_t l2cacheIndex;
uint32_t l2cacheTag;

//------------------------------------//
//          Cache Statistics          //
//------------------------------------//

uint64_t icacheRefs;       // I$ references
uint64_t icacheMisses;     // I$ misses
uint64_t icachePenalties;  // I$ penalties

uint64_t dcacheRefs;       // D$ references
uint64_t dcacheMisses;     // D$ misses
uint64_t dcachePenalties;  // D$ penalties

uint64_t l2cacheRefs;      // L2$ references
uint64_t l2cacheMisses;    // L2$ misses
uint64_t l2cachePenalties; // L2$ penalties

//------------------------------------//
//        Cache Data Structures       //
//------------------------------------//

//
//Auxiliary data structures
//
typedef int bool;
#define true 1
#define false 0

//
//Cache data structures
//
struct entry{
  uint32_t tag;
  uint64_t timestamp;
  bool valid;
};
struct entry** icache;
struct entry** dcache;
struct entry** l2cache;

//------------------------------------//
//          Cache Functions           //
//------------------------------------//

//logarithm base 2
//
uint32_t
logTwo(uint32_t x)
{
  uint32_t ans = 0;
  while(x >>= 1)
    ans++;
  return ans;
}

// Initialize the Cache Hierarchy
//
void
init_cache()
{
  // Initialize cache stats
  icacheRefs        = 0;
  icacheMisses      = 0;
  icachePenalties   = 0;
  dcacheRefs        = 0;
  dcacheMisses      = 0;
  dcachePenalties   = 0;
  l2cacheRefs       = 0;
  l2cacheMisses     = 0;
  l2cachePenalties  = 0;

  blockoffset = logTwo(blocksize);
  icacheIndex = logTwo(icacheSets);
  icacheTag = 32 - icacheIndex - blockoffset;
  dcacheIndex = logTwo(dcacheSets);
  dcacheTag = 32 - dcacheIndex - blockoffset;
  l2cacheIndex = logTwo(l2cacheSets);
  l2cacheTag = 32 - l2cacheIndex - blockoffset;
  
  //Initialize Cache Simulator Data Structures
  if(icacheSets != 0){
    icache = (struct entry **)malloc(icacheSets * sizeof(struct entry *));
    for(int i = 0; i < icacheSets; i++)
      icache[i] = (struct entry *)malloc(icacheAssoc * sizeof(struct entry));
    for(int i = 0; i < icacheSets; i++){
      for(int j = 0; j < icacheAssoc; j++){
        icache[i][j].valid = false;
        icache[i][j].timestamp = ULONG_MAX;
        icache[i][j].tag = UINT_MAX;
      }
    }
  }
  if(dcacheSets != 0){
    dcache = (struct entry **)malloc(dcacheSets * sizeof(struct entry *));
    for(int i = 0; i < dcacheSets; i++)
      dcache[i] = (struct entry *)malloc(dcacheAssoc * sizeof(struct entry));
    for(int i = 0; i < dcacheSets; i++){
      for(int j = 0; j < dcacheAssoc; j++){
        dcache[i][j].valid = false;
        dcache[i][j].timestamp = ULONG_MAX;
        dcache[i][j].tag = UINT_MAX;
      }
    }
  }
  if(l2cacheSets != 0){
    l2cache = (struct entry **)malloc(l2cacheSets * sizeof(struct entry *));
    for(int i = 0; i < l2cacheSets; i++)
      l2cache[i] = (struct entry *)malloc(l2cacheAssoc * sizeof(struct entry));
    for(int i = 0; i < l2cacheSets; i++){
      for(int j = 0; j < l2cacheAssoc; j++){
        l2cache[i][j].valid = false;
        l2cache[i][j].timestamp = ULONG_MAX;
        l2cache[i][j].tag = UINT_MAX;
      }
    }
  }
}

// Perform a memory access through the icache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
icache_access(uint32_t addr)
{
  // uninstantiated
  if(icacheSets == 0){
    return l2cache_access(addr);
  }
  // general
  icacheRefs++;
  uint32_t tag = addr >> (blockoffset + icacheIndex);
  uint32_t setIndex = (addr << icacheTag) >> (icacheTag + blockoffset);
  // hit
  for(int j = 0; j < icacheAssoc; j++){
    if(icache[setIndex][j].valid){              
      if(tag == icache[setIndex][j].tag){
        icache[setIndex][j].timestamp = icacheRefs;
        return icacheHitTime;
      }
    }
  }
  // penalty
  icacheMisses++;
  uint32_t penalty = l2cache_access(addr);
  icachePenalties += penalty;
  // miss
  uint64_t lowest_timestamp = ULONG_MAX;
  int evict_j = -1;
  for(int j = 0; j < icacheAssoc; j++){
    if(!icache[setIndex][j].valid){
      icache[setIndex][j].valid = true;
      icache[setIndex][j].timestamp = icacheRefs;
      icache[setIndex][j].tag = tag;
      return icacheHitTime + penalty;
    }
    else{
      if(icache[setIndex][j].timestamp < lowest_timestamp){
        lowest_timestamp = icache[setIndex][j].timestamp;
        evict_j = j;
      }
    }
  }
  icache[setIndex][evict_j].timestamp = icacheRefs;
  icache[setIndex][evict_j].tag = tag;
  return icacheHitTime + penalty;
}

// Perform a memory access through the dcache interface for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
dcache_access(uint32_t addr)
{
  // uninstantiated
  if(dcacheSets == 0){
    return l2cache_access(addr);
  }
  // general
  dcacheRefs++;
  uint32_t tag = addr >> (blockoffset + dcacheIndex);
  uint32_t setIndex = (addr << dcacheTag) >> (dcacheTag + blockoffset);
  // hit
  for(int j = 0; j < dcacheAssoc; j++){
    if(dcache[setIndex][j].valid){              
      if(tag == dcache[setIndex][j].tag){
        dcache[setIndex][j].timestamp = dcacheRefs;
        return dcacheHitTime;
      }
    }
  }
  // penalty
  dcacheMisses++;
  uint32_t penalty = l2cache_access(addr);
  dcachePenalties += penalty;
  // miss
  uint64_t lowest_timestamp = ULONG_MAX;
  int evict_j = -1;
  for(int j = 0; j < dcacheAssoc; j++){
    if(!dcache[setIndex][j].valid){
      dcache[setIndex][j].valid = true;
      dcache[setIndex][j].timestamp = dcacheRefs;
      dcache[setIndex][j].tag = tag;
      return dcacheHitTime + penalty;
    }
    else{
      if(dcache[setIndex][j].timestamp < lowest_timestamp){
        lowest_timestamp = dcache[setIndex][j].timestamp;
        evict_j = j;
      }
    }
  }
  dcache[setIndex][evict_j].timestamp = dcacheRefs;
  dcache[setIndex][evict_j].tag = tag;
  return dcacheHitTime + penalty;
}

// Perform a memory access to the l2cache for the address 'addr'
// Return the access time for the memory operation
//
uint32_t
l2cache_access(uint32_t addr)
{
  // uninstantiated
  if(l2cacheSets == 0){
    return memspeed;
  }
  // general
  l2cacheRefs++;
  uint32_t tag = addr >> (blockoffset + l2cacheIndex);
  uint32_t setIndex = (addr << l2cacheTag) >> (l2cacheTag + blockoffset);
  // hit
  for(int j = 0; j < l2cacheAssoc; j++){
    if(l2cache[setIndex][j].valid){              
      if(tag == l2cache[setIndex][j].tag){
        l2cache[setIndex][j].timestamp = l2cacheRefs;
        return l2cacheHitTime;
      }
    }
  }
  // penalty
  l2cacheMisses++;
  l2cachePenalties += memspeed;
  // miss
  uint64_t lowest_timestamp = ULONG_MAX;
  int evict_j = -1;
  for(int j = 0; j < l2cacheAssoc; j++){
    if(!l2cache[setIndex][j].valid){
      l2cache[setIndex][j].valid = true;
      l2cache[setIndex][j].timestamp = l2cacheRefs;
      l2cache[setIndex][j].tag = tag;
      return l2cacheHitTime + memspeed;
    }
    else{
      if(l2cache[setIndex][j].timestamp < lowest_timestamp){
        lowest_timestamp = l2cache[setIndex][j].timestamp;
        evict_j = j;
      }
    }
  }
  // inclusive
  if(inclusive == true){
    uint32_t evict_addr = (l2cache[setIndex][evict_j].tag << (blockoffset + l2cacheIndex)) | (setIndex << blockoffset);
    if(icacheSets != 0){
      uint32_t itag = evict_addr >> (blockoffset + icacheIndex);
      uint32_t isetIndex = (evict_addr << icacheTag) >> (icacheTag + blockoffset);
      for(int j = 0; j < icacheAssoc; j++){
        if(icache[isetIndex][j].valid){              
          if(itag == icache[isetIndex][j].tag){
            icache[isetIndex][j].valid = false;
          }
        }
      }
    }
    if(dcacheSets != 0){
      uint32_t dtag = evict_addr >> (blockoffset + dcacheIndex);
      uint32_t dsetIndex = (evict_addr << dcacheTag) >> (dcacheTag + blockoffset);
      for(int j = 0; j < dcacheAssoc; j++){
        if(dcache[dsetIndex][j].valid){              
          if(dtag == dcache[dsetIndex][j].tag){
            dcache[dsetIndex][j].valid = false;
          }
        }
      }
    }
  }
  l2cache[setIndex][evict_j].timestamp = l2cacheRefs;
  l2cache[setIndex][evict_j].tag = tag;
  return l2cacheHitTime + memspeed;
}
