/*
 * The Buddy Page Allocator
 * SKELETON IMPLEMENTATION TO BE FILLED IN FOR TASK 2
 */

#include <infos/mm/page-allocator.h>
#include <infos/mm/mm.h>
#include <infos/kernel/kernel.h>
#include <infos/kernel/log.h>
#include <infos/util/math.h>
#include <infos/util/printf.h>

using namespace infos::kernel;
using namespace infos::mm;
using namespace infos::util;

#define MAX_ORDER 18
constexpr uint64_t MAX_ORDER_NUM = 1ULL << MAX_ORDER;

#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

/**
 * A buddy page allocation algorithm.
 */
class BuddyPageAllocator : public PageAllocatorAlgorithm
{
private:
	/** Given a page descriptor, and an order, returns the buddy PGD.  The buddy could either be
	 * to the left or the right of PGD, in the given order.
	 * @param pgd The page descriptor to find the buddy for.
	 * @param order The order in which the page descriptor lives.
	 * @return Returns the buddy of the given page descriptor, in the given order.
	 */
	PageDescriptor *buddy_of(PageDescriptor *pgd, int order)
	{
		uint64_t self_index = pgd - _page_descriptors;
		uint64_t buddy_index = self_index ^ (1ULL << order);
		return _page_descriptors + buddy_index;
	}

	// /**
	//  * Given a pointer to a block of free memory in the order "source_order", this function will
	//  * split the block in half, and insert it into the order below.
	//  * @param block_pointer A pointer to a pointer containing the beginning of a block of free memory.
	//  * @param source_order The order in which the block of free memory exists.  Naturally,
	//  * the split will insert the two new blocks into the order below.
	//  * @return Returns the left-hand-side of the new block.
	//  */
	// PageDescriptor *split_block(PageDescriptor **block_pointer, int source_order)
	// {
	// 	// TODO: Implement me!
	// 	PageDescriptor *block = *block_pointer;
	// 	*block_pointer = (*block_pointer)->next_free;
	//
	// 	PageDescriptor *block_buddy = buddy_of(block, source_order - 1);
	// 	block_buddy->next_free = _free_areas[source_order - 1];
	// 	block->next_free = block_buddy;
	// 	_free_areas[source_order - 1] = block;
	//
	// 	return block;
	// }

	// /**
	//  * Takes a block in the given source order, and merges it (and its buddy) into the next order.
	//  * @param block_pointer A pointer to a pointer containing a block in the pair to merge.
	//  * @param source_order The order in which the pair of blocks live.
	//  * @return Returns the new slot that points to the merged block.
	//  */
	// PageDescriptor **merge_block(PageDescriptor **block_pointer, int source_order)
	// {
	// 	// TODO: Implement me!
	// 	PageDescriptor *block = *block_pointer;
	// 	*block_pointer = (*block_pointer)->next_free;
	//
	// 	PageDescriptor *block_buddy = buddy_of(block, source_order);
	// 	PageDescriptor **current = &_free_areas[source_order];
	// 	while (*current && *current != block)
	// 	{
	// 		*current = (*current)->next_free;
	// 	}
	//
	// 	block->next_free = _free_areas[source_order - 1];
	// 	_free_areas[source_order - 1] = block;
	//
	// 	return &_free_areas[source_order + 1];
	// }

	uint8_t log2(uint64_t v)
	{
		unsigned int r = 0;
		while (v >>= 1)
		{
			++r;
		}
		return r;
	}

	uint64_t flp2(uint64_t x)
	{
		x |= x >> 1;
		x |= x >> 2;
		x |= x >> 4;
		x |= x >> 8;
		x |= x >> 16;
		x |= x >> 32;
		return x - (x >> 1);
	}

	void insert(PageDescriptor **head, PageDescriptor *node)
	{
		PageDescriptor **current;
		for (current = head; *current; current = &(*current)->next_free)
		{
			if (*current >= node)
			{
				break;
			}
		}
		node->next_free = *current;
		*current = node;
	}

	bool remove(PageDescriptor **head, PageDescriptor *node)
	{		
		PageDescriptor **current;
		for (current = head; *current; current = &(*current)->next_free)
		{
			if (*current == node)
			{
				*current = (*current)->next_free;
				return true;
			}
		}
		return false;
	}

public:
	/**
	 * Allocates 2^order number of contiguous pages
	 * @param order The power of two, of the number of contiguous pages to allocate.
	 * @return Returns a pointer to the first page descriptor for the newly allocated page range, or NULL if
	 * allocation failed.
	 */
	PageDescriptor *allocate_pages(int order) override
	{
		// find from order to MAX_ORDER
		for (int source_order = order; source_order <= MAX_ORDER; ++source_order)
		{
			if (_free_areas[source_order])
			{
				PageDescriptor *pages = _free_areas[source_order];
				// remove pages
				_free_areas[source_order] = pages->next_free;
				// split pages
				while (source_order-- != order)
				{
					// insert buddy pages
					insert(&_free_areas[source_order], buddy_of(pages, source_order));
				}
				return pages;
			}
		}
		// no free pages
		return nullptr;
	}

	/**
	 * Frees 2^order contiguous pages.
	 * @param pgd A pointer to an array of page descriptors to be freed.
	 * @param order The power of two number of contiguous pages to free.
	 */
	void free_pages(PageDescriptor *pgd, int order) override
	{
		// merge pages with buddy
		while (order < MAX_ORDER)
		{
			PageDescriptor *buddy = buddy_of(pgd, order);
			// try remove buddy
			if (!remove(&_free_areas[order], buddy))
			{
				break;
			}
			// merge into higher order
			++order;
			// update leftmost pages
			pgd = MIN(pgd, buddy);
		}

		insert(&_free_areas[order], pgd);
	}

	/**
	 * Marks a range of pages as available for allocation.
	 * @param start A pointer to the first page descriptors to be made available.
	 * @param count The number of page descriptors to make available.
	 */
	virtual void insert_page_range(PageDescriptor *start, uint64_t count) override
	{
		// do nothing if count == 0
		if (!count)
		{
			return;
		}

		uint64_t order_num = MIN(MAX_ORDER_NUM, flp2(count));
		int order = log2(order_num);

		uint64_t index = start - _page_descriptors;
		uint64_t offset = index + (-index & (order_num - 1));
		
		// [offset + order_num, index + count)
		insert_page_range(_page_descriptors + offset + order_num, index + count - offset - order_num);
		// [offset, offset + order_num)
		free_pages(_page_descriptors + offset, order);
		// [index, offset)
		insert_page_range(_page_descriptors + index, offset - index);
	}

	/**
	 * Marks a range of pages as unavailable for allocation.
	 * @param start A pointer to the first page descriptors to be made unavailable.
	 * @param count The number of page descriptors to make unavailable.
	 */
	virtual void remove_page_range(PageDescriptor *start, uint64_t count) override
	{
		// do nothing if count == 0
		if (!count)
		{
			return;
		}
		// [start, start_end)
		PageDescriptor *start_end = start + count;
		for (int order = 0; order <= MAX_ORDER; ++order)
		{
			for (PageDescriptor **current = &_free_areas[order]; *current;)
			{
				// [block, block_end)
				PageDescriptor *block = *current, *block_end = block + (1 << order);
				// check [start, start_end) and [block, block_end) overlap
				if (block < start_end && start < block_end)
				{
					// remove block
					*current = block->next_free;
					// insert [block, start)
					if (block < start)
					{
						insert_page_range(block, start - block);
					}
					// insert [start_end, block_end)
					if (start_end < block_end)
					{
						insert_page_range(start + count, block_end - start_end);
					}
				}
				// move to next
				else
				{
					current = &block->next_free;
				}
			}
		}
	}

	/**
	 * Initialises the allocation algorithm.
	 * @return Returns TRUE if the algorithm was successfully initialised, FALSE otherwise.
	 */
	bool init(PageDescriptor *page_descriptors, uint64_t nr_page_descriptors) override
	{
		// TODO: Implement me!
		this->_page_descriptors = page_descriptors;
		this->_nr_page_descriptors = nr_page_descriptors;
		return true;
	}

	/**
	 * Returns the friendly name of the allocation algorithm, for debugging and selection purposes.
	 */
	const char *name() const override { return "buddy"; }

	/**
	 * Dumps out the current state of the buddy system
	 */
	void dump_state() const override
	{
		// Print out a header, so we can find the output in the logs.
		mm_log.messagef(LogLevel::DEBUG, "BUDDY STATE:");

		// Iterate over each free area.
		for (unsigned int i = 0; i < ARRAY_SIZE(_free_areas); i++)
		{
			char buffer[256];
			snprintf(buffer, sizeof(buffer), "[%d] ", i);

			// Iterate over each block in the free area.
			PageDescriptor *pg = _free_areas[i];
			while (pg)
			{
				// Append the PFN of the free block to the output buffer.
				snprintf(buffer, sizeof(buffer), "%s%lx ", buffer, sys.mm().pgalloc().pgd_to_pfn(pg));
				pg = pg->next_free;
			}

			mm_log.messagef(LogLevel::DEBUG, "%s", buffer);
		}
	}

private:
	PageDescriptor *_free_areas[MAX_ORDER + 1];
	PageDescriptor *_page_descriptors;
	uint64_t _nr_page_descriptors;
};

/* --- DO NOT CHANGE ANYTHING BELOW THIS LINE --- */

/*
 * Allocation algorithm registration framework
 */
RegisterPageAllocator(BuddyPageAllocator);