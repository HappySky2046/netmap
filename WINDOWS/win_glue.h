/*
 * Copyright (C) 2015 Universita` di Pisa. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
 
#ifndef _WIN_GLUE_H_
#define _WIN_GLUE_H_

#ifdef __CYGWIN__
/* we have __attribute__ */
#define _WIN32	/* we use _WIN32 throughout the code */
#else /* some MSC pragmas etc. */

// #define __attribute__(x) __declspec(align(64))
// #define __aligned__	align

//Disabling unuseful warnings
#pragma warning(disable:4018)	// expression: signed/unsigned mismatch
#pragma warning(disable:4047)	// operator: different levels of indirection
#pragma warning(disable:4098)	// void function returning a value - netmap_mem2.c
#pragma warning(disable:4100)	// unreferenced formal parameter
// #pragma warning(disable:4101)	// unreferenced local variable
#pragma warning(disable:4118)	//error in between signed and unsigned
//#pragma warning(disable:4115)	//definition of type between parenthesis
#pragma warning(disable:4127)	//constant conditional expression
#pragma warning(disable:4133)	//warning: uncompatible types: From <1> to <2>
#pragma warning(disable:4142)	//benign type redefinition
// #pragma warning(disable:4189)	//local variable initialized but without references
#pragma warning(disable:4200)	//non-standard extension: matrix of zero dimension in struct/union
#pragma warning(disable:4201)	//nameless structure
#pragma warning(disable:4229)	// zero-size arrays // XXX
#pragma warning(disable:4242)	//possible loss of data in conversion
#pragma warning(disable:4244)	//possible loss of data in conversion
#pragma warning(disable:4245)	//conversion from int to uint_32t: corrispondence error between signed and unsigned
#pragma warning(disable:4389)	//wrong corrispondence between signed and unsigned

#pragma warning(disable:4267)	//conversion from 'size_t' to <type>. possible loss of data

#endif //__CYGWIN__

/*#define FILTER_MAJOR_NDIS_VERSION   NDIS_SUPPORT_NDIS6
#define FILTER_MINOR_NDIS_VERSION   NDIS_SUPPORT_NDIS6*/
#define NDIS_SUPPORT_NDIS6			1	//gives support for NDIS NET_BUFFERs

#define WIN32_LEAN_AND_MEAN 1

#ifndef IS_USERSPACE
#include <ndis.h>
#include <string.h>
#include <WinDef.h>
//#include <Iptypes.h>		// definition of IP_ADAPTER_INFO
#include <netioapi.h>		// definition of IF_NAMESIZE
#include <ntddk.h>          // various NT definitions
#include <errno.h>
#include <intrin.h>			//machine specific code (for example le64toh)
#include <Ntstrsafe.h>
#endif //IS_USERSPACE

#define	M_DEVBUF		'nmDb'	/* netmap pool for memory allocation */
#define	M_NETMAP		'nmBm'	/* bitmap pool for netmap_mem2.c */
#define	M_NOWAIT		1	/* flags for malloc etc */
#define	M_ZERO			2	/* flags for malloc etc */


//Originally defined in LINUX\IF.H
#define	IFNAMSIZ 44//IF_NAMESIZE //defined in netioapi.h, is 256
//XXX_ale	must set the same here and in userspace somehow

/*********************************************************
*   REDEFINITION OF UNCOMMON STRUCTURES FOR WINDOWS		 *
**********************************************************/

// From inttypes.h
typedef __int8 int8_t;
typedef unsigned __int8 uint8_t;
typedef __int16 int16_t;
typedef unsigned __int16 uint16_t;
typedef __int32 int32_t;
typedef unsigned __int32 uint32_t;
typedef __int64 int64_t;
typedef unsigned __int64 uint64_t;
typedef uint32_t u_int;
typedef ULONG u_long;
typedef SSIZE_T ssize_t;


#ifndef IS_USERSPACE
typedef struct timeval {
  LONGLONG tv_sec;
  LONGLONG tv_usec;
} timeval;
typedef char* caddr_t;

typedef PHYSICAL_ADDRESS 	vm_paddr_t;
typedef uint32_t			vm_offset_t;
typedef ULONG 				vm_ooffset_t; 

#define thread PIO_STACK_LOCATION
#endif //IS_USERSPACE
//--------------------------------------------------------

/*********************************************************
*      		  	ERRNO->NTSTATUS TRANSLATION				 *  
**********************************************************/
#define ENOBUFS		STATUS_DEVICE_INSUFFICIENT_RESOURCES	
#define EOPNOTSUPP	STATUS_INVALID_DEVICE_REQUEST
/*********************************************************
*        		  	NO USE IN WINDOWS CODE         		 *
**********************************************************/
#define destroy_dev(a)
#define __user
#define nm_iommu_group_id(dev)	0
#define if_printf	DbgPrint
/*********************************************************
* TRANSLATION OF GCC COMPILER ATTRIBUTES TO MSVC COMPILER*
**********************************************************/

#ifdef _MSC_VER
#define inline __inline
#define __builtin_prefetch(x)	_mm_prefetch(x,_MM_HINT_T2)
#endif //_MSC_VER

#ifndef IS_USERSPACE
static void panic(const char *fmt, ...)
{
	DbgPrint(fmt);
	NT_ASSERT(1);
}
#define __assert	NT_ASSERT
#define assert	NT_ASSERT

/*********************************************************
*        			SPINLOCKS DEFINITION        		 *  
**********************************************************/
typedef struct {
        KSPIN_LOCK      sl;
        KIRQL          irql;
} win_spinlock_t;

static inline void spin_lock_init(win_spinlock_t *m)
{
	KeInitializeSpinLock(&(m->sl));
}

//Acquires the spinlock and saves the current IRQL level
static inline void mtx_lock(win_spinlock_t *m)
{
    KeAcquireSpinLock(&(m->sl), &(m->irql));
}

//Release the spinlock and restore the old IRQL level
static inline void mtx_unlock(win_spinlock_t *m)
{
    KeReleaseSpinLock(&(m->sl), (m->irql));
}

#define mtx_init(a, b, c, d)	spin_lock_init(a)
#define mtx_destroy(a)		(void)(a)	// XXX spin_lock_destroy(a)

#define mtx_lock_spin(a)	mtx_lock(a)
#define mtx_unlock_spin(a)	mtx_unlock(a)
//--------------------------------------------------------

/*********************************************************
*        		READ/WRITE LOCKS DEFINITION        		 *
**********************************************************/

#define BDG_RWLOCK_T			ERESOURCE
#define BDG_RWINIT(b)			ExInitializeResourceLite(b.bdg_lock)
#define BDG_RWDESTROY(b)		ExDeleteResourceLite(b.bdg_lock)
#define BDG_WLOCK(b)			ExAcquireResourceExclusiveLite(&b->bdg_lock, TRUE)
#define BDG_WUNLOCK(b)			ExReleaseResourceLite(&b->bdg_lock)
#define BDG_RLOCK(b)			ExAcquireResourceSharedLite(&b->bdg_lock,TRUE)
#define BDG_RUNLOCK(b)			ExReleaseResourceLite(&b->bdg_lock)
#define BDG_RTRYLOCK(b)			ExAcquireResourceExclusiveLite(&b->bdg_lock, FALSE)
#define BDG_SET_VAR(lval, p)	((lval) = (p))
#define BDG_GET_VAR(lval)		(lval)
#define BDG_FREE(p)				free(p)
//--------------------------------------------------------

/*********************************************************
*        			SLEEP/WAKEUP THREADS        		 *
**********************************************************/

#define PI_NET					16
#define init_waitqueue_head(x)			KeInitializeEvent(x, NotificationEvent, TRUE);
#define netmap_knlist_destroy(x)
#define tsleep(ident, priority, wmesg, time)	KeDelayExecutionThread(KernelMode, FALSE, (PLARGE_INTEGER)time)	
//--------------------------------------------------------

#define mb				KeMemoryBarrier
#define rmb				KeMemoryBarrier //XXX_ale: doesn't seems to exist just a read barrier

/*********************************************************
*        			TIME FUNCTIONS		        		 *
**********************************************************/
void do_gettimeofday(struct timeval *tv);
static int time_uptime_w32()
{
	int ret;
	LARGE_INTEGER tm;
	KeQuerySystemTime(&tm);
	ret = (int)(tm.QuadPart / (LONGLONG)1000000);
	return ret;
}

#define microtime		do_gettimeofday
#define time_second		time_uptime_w32
//--------------------------------------------------------

#define snprintf 							_snprintf
#define printf								DbgPrint

/* XXX copyin used in vale (indirect bufs) and netmap_offloadings.c
 * Is it ok to use RtlCopyMemory for user buffers ?
 */
#define copyin(src, dst, copy_len)		RtlCopyMemory(dst, src, copy_len)

#if 0
static NTSTATUS SafeAllocateString(OUT PUNICODE_STRING result, IN USHORT size)
{
	ASSERT(result != NULL);
	if (result == NULL || size == 0)
		return STATUS_INVALID_PARAMETER;

	result->Buffer = ExAllocatePoolWithTag(NonPagedPool, size, 'rtsM');
	result->Length = 0;
	result->MaximumLength = size;

	if (result->Buffer)
		RtlZeroMemory(result->Buffer, size);
	else
		return STATUS_NO_MEMORY;

	return STATUS_SUCCESS;
}
#endif /* UNUSED */

/*********************************************************
*        		GENERIC/HW SPECIFIC STRUCTURES     		 *
**********************************************************/


struct netmap_adapter;
#if 0
struct ifnet {
	char    if_xname[IFNAMSIZ];     		/* external name (name + unit) */
	//        struct ifaltq if_snd;         /* output queue (includes altq) */
	struct netmap_adapter* na;

	int* netmap_generic_rx_handler;
};
#endif

struct net_device {
	char	if_xname[IFNAMSIZ];			// external name (name + unit) 
	//        struct ifaltq if_snd;         /* output queue (includes altq) */
	struct netmap_adapter*	na;
	void *	pfilter;
	int *	intercept;	// bit 0: enable rx, bit 1 enable tx
#define NM_WIN_CATCH_RX	1
#define NM_WIN_CATCH_TX	2
	int		ifIndex;

	NPAGED_LOOKASIDE_LIST	mbuf_pool;
	NPAGED_LOOKASIDE_LIST	mbuf_packets_pool;
	BOOLEAN	lookasideListsAlreadyAllocated;
};

#define ifnet		net_device
struct mbuf {
	struct mbuf			*m_next;
	struct mbuf			*m_nextpkt;
	uint32_t			m_len;
	struct net_device	*dev;
	PVOID				pkt;
	void*(*netmap_default_mbuf_destructor)(struct mbuf *m);
};

/*
 * the following structure is used to hook ndis with netmap.
 */
typedef struct _FUNCTION_POINTER_XCHANGE {
	/* ndis -> netmap calls */
	struct NET_BUFFER* (*handle_rx)(struct net_device*, uint32_t length, const char* data);
	struct NET_BUFFER* (*handle_tx)(struct net_device*, uint32_t length, const char* data);

	/* netmap -> ndis calls */
	NTSTATUS (*ndis_regif)(struct net_device *ifp);
	NTSTATUS (*ndis_rele)(struct net_device *ifp);
	PVOID (*injectPacket)(PVOID _pfilter, PVOID data, uint32_t length, BOOLEAN sendToMiniport, PNET_BUFFER_LIST prev);
} FUNCTION_POINTER_XCHANGE; // , *PFUNCTION_POINTER_XCHANGE;

//XXX_ale To be correctly redefined
#define GET_MBUF_REFCNT(a)				1
#define	SET_MBUF_DESTRUCTOR(a,b)		// XXX must be set to enable tx notifications
#define MBUF_IFP(m)						m->dev	

void win32_init_lookaside_buffers(struct net_device *ifp);
void win32_clear_lookaside_buffers(struct net_device *ifp);

struct hrtimer;	// XXX unused, from generic
struct nm_generic_mit;	// XXX unused, from generic
struct device;	// XXX unused, in some place in netmap_mem2.c

static void
generic_timer_handler(struct hrtimer *t)
{
	DbgPrint("unimplemented %p\n", t);
#if 0
	struct nm_generic_mit *mit =
		container_of(t, struct nm_generic_mit, mit_timer);
	u_int work_done;

	if (!mit->mit_pending) {
		return HRTIMER_NORESTART;
	}

	/* Some work arrived while the timer was counting down:
	* Reset the pending work flag, restart the timer and send
	* a notification.
	*/
	mit->mit_pending = 0;
	/* below is a variation of netmap_generic_irq  XXX revise */
	if (nm_netmap_on(mit->mit_na)) {
		netmap_common_irq(mit->mit_na->ifp, mit->mit_ring_idx, &work_done);
		generic_rate(0, 0, 0, 0, 0, 1);
	}
	nm_os_mitigation_restart(mit);
	return HRTIMER_RESTART;
#endif
}

static void nm_os_mitigation_init(struct nm_generic_mit *mit, int idx,
	struct netmap_adapter *na)
{
	DbgPrint("unimplemented %p idx %d na %p\n", mit, idx, na);
#if 0
	hrtimer_init(&mit->mit_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	mit->mit_timer.function = &generic_timer_handler;
	mit->mit_pending = 0;
	mit->mit_ring_idx = idx;
	mit->mit_na = na;
#endif
}

static void nm_os_mitigation_start(struct nm_generic_mit *mit)
{
	DbgPrint("unimplemented %p\n", mit);
	//hrtimer_start(&mit->mit_timer, ktime_set(0, netmap_generic_mit), HRTIMER_MODE_REL);
}

static void nm_os_mitigation_restart(struct nm_generic_mit *mit)
{
	DbgPrint("unimplemented %p\n", mit);
	//hrtimer_forward_now(&mit->mit_timer, ktime_set(0, netmap_generic_mit));
}

static int nm_os_mitigation_active(struct nm_generic_mit *mit)
{
	DbgPrint("unimplemented %p\n", mit);
	return 1;
	//return hrtimer_active(&mit->mit_timer);
}

static void nm_os_mitigation_cleanup(struct nm_generic_mit *mit)
{
	DbgPrint("unimplemented %p\n", mit);
	//hrtimer_cancel(&mit->mit_timer);
}


static inline int ilog2(uint64_t n);
static inline int roundup_pow_of_two(int sz);

/*-------------------------------------------
 *      KERNEL MEMORY ALLOCATION and management
 */

char* win_contigMalloc(int sz, int page_size);
void win_ContigFree(void* virtualAddress);

#define bcopy(_s, _d, _l)			RtlCopyMemory(_d, _s, _l)
#define bzero(addr, size)			RtlZeroMemory(addr, size)
#define malloc(size, _ty, flags)		win_kernel_malloc(size, _ty, flags)
#define free(addr, _type)			ExFreePoolWithTag(addr, _type)
#define realloc(src, len, old_len)		win_reallocate(src, len, old_len)

/*
 * we default to always allocating and zeroing
 */
static void *
win_kernel_malloc(size_t size, int32_t ty, int flags)
{
	void* mem = ExAllocatePoolWithTag(NonPagedPool, size, ty);

	(void)flags;

	if (mem != NULL) { // && flags & M_ZERO XXX todo
		RtlZeroMemory(mem, size);
	}
	return mem;
}

static inline PVOID
win_reallocate(void* src, size_t size, size_t oldSize)
{
	//DbgPrint("Netmap.sys: win_reallocate(%p, %i, %i)", src, size, oldSize);
	PVOID newBuff = NULL; /* default return value */

	if (src == NULL) { /* if size > 0, this is a malloc */
		if (size > 0) {
			newBuff = malloc(size, M_DEVBUF, M_NOWAIT | M_ZERO);
		}
	} else if (size == 0) {
		free(src, M_DEVBUF);
	} else if (size == oldSize) {
		newBuff = src;
	} else { /* realloc -- XXX later maybe ignore shrink ? */
		newBuff = malloc(size, M_DEVBUF, M_NOWAIT | M_ZERO);
		if (newBuff != NULL) {
			if (size <= oldSize) { /* shrink, just copy back part of the data */
				RtlCopyMemory(newBuff, src, size);
			} else {
				RtlCopyMemory(newBuff, src, oldSize);
			}
		}
	}
	return newBuff;
}

/*
 * mbuf allocation and free.
 * XXX These are used in the generic code and must be verified.
 */
#if 0
static void
netmap_default_mbuf_destructor(struct mbuf *m)
{
	if (m->pkt != NULL) {
		free(m->pkt, M_DEVBUF);
		m->pkt = NULL;
	}
	free(m, M_DEVBUF);
	m = NULL;
}
#endif

struct mbuf *win_make_mbuf(struct net_device *, uint32_t, const char *);

#define nm_os_get_mbuf(ifp, _l)	win_make_mbuf(ifp, _l, NULL)
	// XXX do we also need the netmap_default_mbuf_destructor ?


static inline void 
win32_ndis_packet_freem(struct mbuf* m)
{
	if (m != NULL) {
		if (m->pkt != NULL) {
			//free(m->pkt, M_DEVBUF);
			ExFreeToNPagedLookasideList(&m->dev->mbuf_packets_pool, m->pkt);
			m->pkt = NULL;
		}
		ExFreeToNPagedLookasideList(&m->dev->mbuf_pool, m);
		//free(m, M_DEVBUF);

	}	
}

#define MBUF_LEN(m)					((m)->m_len)
/*
 * m_devget() is used to construct an mbuf from a host ring to the host stack
 */
#define m_devget(data, len, offset, dev, fn)		win_make_mbuf(dev, len, data)
#define m_freem(mbuf)					win32_ndis_packet_freem(mbuf);
#define m_copydata(source, offset, length, dst)		RtlCopyMemory(dst, source->pkt, length)


#define le64toh(x)		_byteswap_uint64(x)	//defined in intrin.h

struct net_device* ifunit_ref(const char *name);
void if_rele(struct net_device *ifp);

PVOID send_up_to_stack(struct ifnet *ifp, struct mbuf *m, PVOID head);

#define WNA(_ifp)		_ifp->na
#define NM_BNS_GET(b)	do { (void)(b); } while (0)
#define NM_BNS_PUT(b)   do { (void)(b); } while (0)

/*********************************************************
*                   ATOMIC OPERATIONS     		         *  
**********************************************************/
#define NM_ATOMIC_T 					volatile long
#define atomic_t						NM_ATOMIC_T
#define NM_ATOMIC_TEST_AND_SET(p)       (!InterlockedBitTestAndSet(p,0))
#define NM_ATOMIC_CLEAR(p)              InterlockedBitTestAndReset(p,0)
#define refcount_acquire(_a)    		InterlockedExchangeAdd((atomic_t *)_a, 1)
#define refcount_release(_a)    		(InterlockedDecrement((atomic_t *)_a) <= 0)
#define NM_ATOMIC_SET(p, v)             InterlockedExchange(p, v)
#define NM_ATOMIC_INC(p)                InterlockedIncrement(p)
#define NM_ATOMIC_READ_AND_CLEAR(p)     InterlockedExchange(p, 0)
#define NM_ATOMIC_READ(p)               InterlockedExchangeAdd(p, 0)


#define make_dev_credf(_a, _b, ...)	((void *)1)	// non-null

#define contigmalloc(sz, ty, flags, a, b, pgsz, c)	win_contigMalloc(sz,pgsz)
#define contigfree(va, sz, ty)				win_ContigFree(va)

#define vtophys						MmGetPhysicalAddress
#define MALLOC_DEFINE(a,b,c)
//--------------------------------------------------------
#endif //IS_USERSPACE

/*********************************************************
* SYSCTL emulation (copied from dummynet/glue.h)		 *
**********************************************************/
struct sock; // XXX unused

int do_netmap_set_ctl(struct sock *sk, int cmd, void __user *user, unsigned int len);
int do_netmap_get_ctl(struct sock *sk, int cmd, void __user *user, int *len);

enum sopt_dir { SOPT_GET, SOPT_SET };

struct  sockopt {
	enum    sopt_dir sopt_dir; /* is this a get or a set? */
	int     sopt_level;     /* second arg of [gs]etsockopt */
	int     sopt_name;      /* third arg of [gs]etsockopt */
#ifdef _X64EMU
	void* pad1;
	void* pad2;
#endif
	void   *sopt_val;       /* fourth arg of [gs]etsockopt */
	size_t  sopt_valsize;   /* (almost) fifth arg of [gs]etsockopt */
#ifdef _X64EMU
	void* pad3;
	void* pad4;
#endif
	struct  thread *sopt_td; /* calling thread or null if kernel */
};

#define STRINGIFY(x) #x

enum {
	SYSCTLTYPE_INT = 0,
	SYSCTLTYPE_UINT = 1,
	SYSCTLTYPE_SHORT = 2,
	SYSCTLTYPE_USHORT = 3,
	SYSCTLTYPE_LONG = 4,
	SYSCTLTYPE_ULONG = 5,
	SYSCTLTYPE_STRING = 6,

	/* the following are SYSCTL_PROC equivalents of the above,
	* where the SYSCTLTYPE is shifted 2 bits,
	* and SYSCTLTYPE_PROC is set
	*/
	SYSCTLTYPE_PROC = 0x100,
	CTLTYPE_INT = (0x100 | (0 << 2)),
	CTLTYPE_UINT = (0x100 | (1 << 2))
};

struct sysctlhead {
	uint32_t blocklen; //total size of the entry
	uint32_t namelen; //strlen(name) + '\0'
	uint32_t flags; //type and access
	uint32_t datalen;
};

#ifdef SYSCTL_NODE
#undef SYSCTL_NODE
#endif
#define SYSCTL_NODE(a,b,c,d,e,f)
#define SYSCTL_DECL(a)
#define SYSCTL_VNET_PROC(a,b,c,d,e,f,g,h,i)
#define GST_HARD_LIMIT 100

/* In the module, GST is implemented as an array of
* sysctlentry, but while passing data to the userland
* pointers are useless, the buffer is actually made of:
* - sysctlhead (fixed size, containing lengths)
* - data (typically 32 bit)
* - name (zero-terminated and padded to mod4)
*/

struct sysctlentry {
	struct sysctlhead head;
	char* name;
	void* data;
};

struct sysctltable {
	int count; //number of valid tables
	int totalsize; //total size of valid entries of al the valid tables
	void* namebuffer; //a buffer for all chained names
	struct sysctlentry entry[GST_HARD_LIMIT];
};

#ifdef SYSBEGIN
#undef SYSBEGIN
#endif
#define SYSBEGIN(x) void sysctl_addgroup_##x() {
#ifdef SYSEND
#undef SYSEND
#endif
#define SYSEND }

#define CTLFLAG_RD		1
#define CTLFLAG_RDTUN	1
#define CTLFLAG_RW		2
#define CTLFLAG_SECURE3	0 // unsupported
#define CTLFLAG_VNET    0	/* unsupported */

/* XXX remove duplication */
#define SYSCTL_INT(a,b,c,d,e,f,g) 				\
	sysctl_pushback(STRINGIFY(a) "." STRINGIFY(c) + 1,	\
		(d) | (SYSCTLTYPE_INT << 2), sizeof(*e), e)

#define SYSCTL_VNET_INT(a,b,c,d,e,f,g)				\
	sysctl_pushback(STRINGIFY(a) "." STRINGIFY(c) + 1,	\
		(d) | (SYSCTLTYPE_INT << 2), sizeof(*e), e)

#define SYSCTL_UINT(a,b,c,d,e,f,g)				\
	sysctl_pushback(STRINGIFY(a) "." STRINGIFY(c) + 1,	\
		(d) | (SYSCTLTYPE_UINT << 2), sizeof(*e), e)

#define SYSCTL_VNET_UINT(a,b,c,d,e,f,g)				\
	sysctl_pushback(STRINGIFY(a) "." STRINGIFY(c) + 1,	\
		(d) | (SYSCTLTYPE_UINT << 2), sizeof(*e), e)

#define SYSCTL_LONG(a,b,c,d,e,f,g)				\
	sysctl_pushback(STRINGIFY(a) "." STRINGIFY(c) + 1,	\
		(d) | (SYSCTLTYPE_LONG << 2), sizeof(*e), e)

#define SYSCTL_ULONG(a,b,c,d,e,f,g)				\
	sysctl_pushback(STRINGIFY(a) "." STRINGIFY(c) + 1,	\
		(d) | (SYSCTLTYPE_ULONG << 2), sizeof(*e), e)

#define TUNABLE_INT(a,b)

void keinit_GST(void);
void keexit_GST(void);
int kesysctl_emu_set(void* p, int l);
int kesysctl_emu_get(struct sockopt* sopt);
void sysctl_pushback(char* name, int flags, int datalen, void* data);
int sysctlbyname(const char *name, void *oldp, size_t *oldlenp, void *newp, size_t newlen);

int do_cmd(int optname, void *optval, uintptr_t optlen);

//--------------------------------------------------------

/*********************************************************
*			POLL VALUES DEFINITIONS						 *
**********************************************************/
#ifndef POLLRDNORM
#define POLLRDNORM  0x0040
#endif
#ifndef POLLRDBAND
#define POLLRDBAND  0x0080
#endif
#ifndef POLLIN
#define POLLIN		0x0001
#endif
#ifndef POLLPRI
#define POLLPRI     0x0002
#endif

#ifndef POLLWRNORM
#define POLLWRNORM  0x0100
#endif
#ifndef POLLOUT
#define POLLOUT		0x0004
#endif
#ifndef POLLWRBAND
#define POLLWRBAND  0x0200
#endif

#ifndef POLLERR
#define POLLERR     0x0008
#endif
#ifndef POLLHUP
#define POLLHUP     0x0010
#endif
#ifndef POLLNVAL
#define POLLNVAL    0x0020
#endif


#endif //_WIN_GLUE_H
