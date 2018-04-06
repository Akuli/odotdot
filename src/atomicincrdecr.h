/*
atomic incrementing and decrementing for thread-safe reference counting

example:
	int var = 0;

	void threadsafe_func(void)
	{
		ATOMIC_INCR(var);
		ATOMIC_DECR(var);
	}

	// call threadsafe_func() many times in different threads
	// var will be 0 after all calls

these functions return the new value of var
*/

#ifndef ATOMICINCRDECR_H
#define ATOMICINCRDECR_H

// clang defines __GNUC__ for compatibility with gcc
#ifdef __GNUC__    // GCC or clang
	// this stuff seems to be new in GCC 4.7.0, try replacing 4.7 with 4.6 in this url and it 404s
	// https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/_005f_005fatomic-Builtins.html
	// this page also has a "GCC 4.7 status" section: https://gcc.gnu.org/wiki/Atomic/GCCMM
	//
	// clang also provides these, but it gives me __GNUC__ == 4 and __GNUC_MINOR__ == 2
	// https://clang.llvm.org/docs/LanguageExtensions.html#c11-atomic-builtins
	#if defined(__clang__) || (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 7))
		#define ATOMIC_INCR(var) __atomic_add_fetch(&(var), 1, __ATOMIC_SEQ_CST)
		#define ATOMIC_DECR(var) __atomic_sub_fetch(&(var), 1, __ATOMIC_SEQ_CST)
	#else
		// older gcc's have __sync builtins
		// https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/_005f_005fsync-Builtins.html#g_t_005f_005fsync-Builtins
		#define ATOMIC_INCR(var) __sync_add_and_fetch(&(var), 1);
		#define ATOMIC_DECR(var) __sync_sub_and_fetch(&(var), 1);
	#endif
// TODO: windows atomics and maybe a really dumb lock fallback
#else
	#error sorry, only GCC and clang are supported right now :(
#endif    // __GNUC__

#endif    // ATOMICINCRDECR_H
