/**
 * Defines a memory addressing scheme providing known addresses and address ranges for segments, pools, and their metadata.
 */
#pragma once

#include <stdint.h>
#include <type_traits>

namespace mem {

static constexpr uint32_t pointer_width = 8 * sizeof(void*);
static constexpr uint32_t address_width = 6 * sizeof(void*); // x86_64 only uses 48/64 bits. damn!

static constexpr uint64_t bits_to_size (uint32_t bits) {
	return 0x1L << bits;
}


static constexpr uint64_t bits_to_mask (uint32_t bitlength, uint32_t bitpos) {
	return ((uint64_t)(0x1L << bitlength) - 0x1L) << bitpos;
}


template<uint64_t B>
struct bit_storage
{
	typedef typename std::conditional<B <= 8, uint8_t,
	typename std::conditional<B <= 16, uint16_t,
  typename std::conditional<B <= 32, uint32_t, uint64_t >::type >::type >::type type;
};


template<typename I>
constexpr I bits(void *ptr64, uint64_t bitlength, uint64_t bitpos) {
	static_assert(std::is_unsigned<I>::value, "The integer type used to capture this field should be unsigned.");
	uint64_t mask = bits_to_mask(bitlength,bitpos);
	I result = ((uint64_t)(ptr64) & mask) >> bitpos;
	return result;
}


template<uint64_t RID, uint64_t RB>
struct region_addr_traits
{
	typedef typename bit_storage<RB>::type regionid_t;
	
	static constexpr uint32_t regionid_bits = RB;
	constexpr static uint64_t rid = RID;
	constexpr static uint64_t regionid_ofs = address_width - RB;
	constexpr static uint64_t region_address () { return RID << regionid_ofs; }
	
	static auto regionid (void* ptr) {
		return mem::bits<regionid_t>(ptr,RB,address_width-RB);
	}
	
};


template<uint64_t RID, uint32_t RB, uint32_t PB, uint32_t SB, uint32_t OB>
struct pool_addr_traits : public region_addr_traits<RID,RB>
{
	static_assert(RB + PB + SB + OB == address_width,
								"An addressing format must have bit field widths that add up to the pointer width.");

	static constexpr uint32_t regionid_bits = RB;
	static constexpr uint32_t poolid_bits = PB;
	static constexpr uint32_t segmentid_bits = SB;
	static constexpr uint32_t offset_bits = OB;
	
	static constexpr uint64_t regionid_space = bits_to_size(regionid_bits);
	static constexpr uint64_t poolid_space = bits_to_size(poolid_bits);
	static constexpr uint64_t segmentid_space = bits_to_size(segmentid_bits);
	static constexpr uint64_t offset_space = bits_to_size(offset_bits);
	
	static constexpr uint64_t segment_size = offset_space;
	static constexpr uint64_t page_size = offset_space;
	
	static constexpr uint64_t regionid_mask = bits_to_mask(regionid_bits, address_width - regionid_bits);
	static constexpr uint64_t poolid_mask = bits_to_mask(poolid_bits, segmentid_bits + offset_bits);
	static constexpr uint64_t segmentid_mask = bits_to_mask(segmentid_bits, offset_bits);
	static constexpr uint64_t offset_mask = bits_to_mask(offset_bits, 0);
	
	using regionid_t = typename region_addr_traits<RID,RB>::regionid_t;
	
	typedef typename bit_storage<PB>::type poolid_t;
	
	typedef typename bit_storage<SB>::type segmentid_t;
	
	typedef typename bit_storage<OB>::type offset_t;
	
	static auto poolid (void* ptr) {
		return mem::bits<poolid_t>(ptr,PB,SB+OB);
	}
	
	static auto segmentid (void* ptr) {
		return mem::bits<segmentid_t>(ptr,SB,OB);
	}
	
	static auto offset (void* ptr) {
		return mem::bits<uint16_t>(ptr,OB,0);
	}

	static void* base_address (poolid_t pid, segmentid_t segid = 0) {
		return (void*)((RID << (address_width - RB)) + (pid << (OB+SB)) + (segid << OB));
	}
	
};


const uint16_t progheap_region_id = 0;


} // namespace mem
