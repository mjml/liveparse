/**
 * Defines a memory addressing scheme providing known addresses and address ranges for segments, pools, and their metadata.
 */
#pragma once

namespace util {

static constexpr int pointer_width = 8 * sizeof(void*);
static constexpr int addr_width = 6 * sizeof(void*);

template<int B>
struct bit_storage
{
	typedef typename std::conditional<B <= 8, uint8_t,
	typename std::conditional<B <= 16, uint16_t,
  typename std::conditional<B <= 32, uint32_t, uint64_t >::type >::type >::type type;
};


template<typename I>
constexpr I bits(void *ptr64, uint64_t bitlength, uint64_t bitpos) {
	static_assert(std::is_unsigned<I>::value, "The integer type used to capture this field should be unsigned.");
	return (I)
		(((uint64_t)(ptr64) & (((1L << bitlength) - 1L) << bitpos)) >> bitpos);
}


static constexpr uint64_t bits_to_size (int bits) {
	return (0x1L << bits);
}


template<int RB>
struct region_addr_traits
{
	typedef typename bit_storage<RB>::type regionid_t;

	constexpr static int regionid_ofs = pointer_width - RB;
	
	static auto regionid (void* ptr) {
		return util::bits<uint16_t>(ptr,RB,addr_width-RB);
	}
};


template<int RB=4, int PB=20, int SB=12, int OB=12>
struct pool_addr_traits : public region_addr_traits<RB>
{
	static_assert(RB + PB + SB + OB == addr_width,
								"An addressing format must have bit field widths that add up to the pointer width.");
	
	static constexpr int regionid_bits = RB;
	static constexpr int poolid_bits = PB;
	static constexpr int segmentid_bits = SB;
	static constexpr int offsetid_bits = OB;
	
	static constexpr uint64_t regionid_space = bits_to_size(regionid_bits);
	static constexpr uint64_t poolid_space = bits_to_size(poolid_bits);
	static constexpr uint64_t segmentid_space = bits_to_size(segmentid_bits);
	static constexpr uint64_t offsetid_space = bits_to_size(offsetid_bits);
	
	static constexpr uint64_t segment_size = offsetid_space;
	static constexpr uint64_t page_size = offsetid_space;
	
	using regionid_t = typename region_addr_traits<RB>::regionid_t;
	
	typedef typename bit_storage<PB>::type poolid_t;
	
	typedef typename bit_storage<SB>::type segmentid_t;
	
	typedef typename bit_storage<OB>::type offset_t;
	
	static auto poolid (void* ptr) {
		return util::bits<poolid_t>(ptr,PB,SB+OB);
	}
	
	static auto segmentid (void* ptr) {
		return util::bits<segmentid_t>(ptr,SB,OB);
	}
	
	static uint16_t offset (void* ptr) {
		return util::bits<uint16_t>(ptr,OB,0);
	}
	
};


const uint16_t progheap_region_id = 0;


}
