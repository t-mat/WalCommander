#pragma once

#include "../deps/tlsf/tlsf.h"

#if !defined(TLSF_HPP_LAMBDA_WALKER)
#  define TLSF_HPP_LAMBDA_WALKER 1
#endif

class Tlsf {
public:
	Tlsf() = delete;
	~Tlsf() {}

	static Tlsf* create(void* mem, size_t bytes) {
		auto* p = tlsf_create_with_pool(mem, bytes);
		return reinterpret_cast<Tlsf*>(p);
	}

	void destroy() {
		auto* ts = reinterpret_cast<tlsf_t>(this);
		tlsf_destroy(ts);
	}

	// malloc/free
	void* malloc(size_t bytes) {
		auto* ts = reinterpret_cast<tlsf_t>(this);
		return tlsf_malloc(ts, bytes);
	}

	void* memalign(size_t align, size_t bytes) {
		auto* ts = reinterpret_cast<tlsf_t>(this);
		return tlsf_memalign(ts, align, bytes);
	}

	void* realloc(void* ptr, size_t size) {
		auto* ts = reinterpret_cast<tlsf_t>(this);
		return tlsf_realloc(ts, ptr, size);
	}

	void free(void* ptr) {
		auto* ts = reinterpret_cast<tlsf_t>(this);
		return tlsf_free(ts, ptr);
	}

	// Constants
	static size_t getBlockSize(void* ptr) {
		return tlsf_block_size(ptr);
	}

	static size_t controlSize() {
		return tlsf_size();
	}

	static size_t alignSize() {
		return tlsf_align_size();
	}

	static size_t blockSizeMin() {
		return tlsf_block_size_min();
	}

	static size_t blockSizeMax() {
		return tlsf_block_size_max();
	}

	static size_t poolOverhead() {
		return tlsf_pool_overhead();
	}

	static size_t allocOverhead() {
		return tlsf_alloc_overhead();
	}

	// Debug
	bool check() const {
		const auto ts = (const tlsf_t) (this);
		return 0 != tlsf_check(ts);
	}

	void walk(const tlsf_walker walker, void* user) const {
		const auto ts = (const tlsf_t) (this);
		const auto pool = tlsf_get_pool(ts);
		tlsf_walk_pool(pool, walker, user);
	}

#if defined(TLSF_HPP_LAMBDA_WALKER) && TLSF_HPP_LAMBDA_WALKER
	using Walker = std::function<void(const void* ptr, size_t size, int used)>;

	struct WalkerHelper {
		const Walker& walker;

		static void walkerFunc(const void* ptr, size_t size, int used, void* user) {
			auto& walkerHelper = * reinterpret_cast<WalkerHelper*>(user);
			return walkerHelper.walker(ptr, size, used);
		}
	};

	void walk(const Walker& walker) const {
		WalkerHelper walkerHelper { walker };
		return walk((const tlsf_walker) WalkerHelper::walkerFunc, &walkerHelper);
	}
#endif
};
