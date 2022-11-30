//=====================================================================
//
// KCP - A Better ARQ Protocol Implementation
// 原作者: skywind3000 (at) gmail.com, 2010-2011
//
// Features:
// + Average RTT reduce 30% - 40% vs traditional ARQ like tcp.
// + Maximum RTT reduce three times vs tcp.
// + Lightweight, distributed as a single source file.
//
// 当前项目地址: https://github.com/Mainvooid/kcp-hpp
//=====================================================================
#ifndef __IKCP_H__
#define __IKCP_H__

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>


//=====================================================================
// 32BIT INTEGER DEFINITION
//=====================================================================
#ifndef __INTEGER_32_BITS__
#define __INTEGER_32_BITS__
#if defined(_WIN64) || defined(WIN64) || defined(__amd64__) || \
    defined(__x86_64) || defined(__x86_64__) || defined(_M_IA64) || \
    defined(_M_AMD64)
typedef unsigned int ISTDUINT32;
typedef int ISTDINT32;
#elif defined(_WIN32) || defined(WIN32) || defined(__i386__) || \
    defined(__i386) || defined(_M_X86)
typedef unsigned long ISTDUINT32;
typedef long ISTDINT32;
#elif defined(__MACOS__)
typedef UInt32 ISTDUINT32;
typedef SInt32 ISTDINT32;
#elif defined(__APPLE__) && defined(__MACH__)
#include <sys/types.h>
typedef u_int32_t ISTDUINT32;
typedef int32_t ISTDINT32;
#elif defined(__BEOS__)
#include <sys/inttypes.h>
typedef u_int32_t ISTDUINT32;
typedef int32_t ISTDINT32;
#elif (defined(_MSC_VER) || defined(__BORLANDC__)) && (!defined(__MSDOS__))
typedef unsigned __int32 ISTDUINT32;
typedef __int32 ISTDINT32;
#elif defined(__GNUC__)
#include <stdint.h>
typedef uint32_t ISTDUINT32;
typedef int32_t ISTDINT32;
#else
typedef unsigned long ISTDUINT32;
typedef long ISTDINT32;
#endif // platform
#endif // __INTEGER_32_BITS__

//=====================================================================
// Integer Definition
//=====================================================================
#ifndef __IINT8_DEFINED
#define __IINT8_DEFINED
typedef char IINT8;
#endif

#ifndef __IUINT8_DEFINED
#define __IUINT8_DEFINED
typedef unsigned char IUINT8;
#endif

#ifndef __IUINT16_DEFINED
#define __IUINT16_DEFINED
typedef unsigned short IUINT16;
#endif

#ifndef __IINT16_DEFINED
#define __IINT16_DEFINED
typedef short IINT16;
#endif

#ifndef __IINT32_DEFINED
#define __IINT32_DEFINED
typedef ISTDINT32 IINT32;
#endif

#ifndef __IUINT32_DEFINED
#define __IUINT32_DEFINED
typedef ISTDUINT32 IUINT32;
#endif

#ifndef __IINT64_DEFINED
#define __IINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef __int64 IINT64;
#else
typedef long long IINT64;
#endif
#endif

#ifndef __IUINT64_DEFINED
#define __IUINT64_DEFINED
#if defined(_MSC_VER) || defined(__BORLANDC__)
typedef unsigned __int64 IUINT64;
#else
typedef unsigned long long IUINT64;
#endif
#endif

#ifndef INLINE
#if defined(__GNUC__)
#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
#define INLINE __inline__ __attribute__((always_inline))
#else
#define INLINE __inline__
#endif
#elif (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__))
#define INLINE __inline
#else
#define INLINE
#endif // __GNUC__
#endif // INLINE

#if (!defined(__cplusplus)) && (!defined(inline))
#define inline INLINE
#endif

//=====================================================================
// QUEUE DEFINITION
//=====================================================================
#ifndef __IQUEUE_DEF__
#define __IQUEUE_DEF__

namespace detail {
    /**
     @brief 通用链表实现的队列.
     @note
     用于管理Segment队列,
     通用链表可以支持在不同类型的链表中做转移,
     通用链表实际上管理的就是一个最小的链表节点,
     具体该链表节点所在的数据块可以通过该数据块在链表中的位置反向解析出来.
     @see IQUEUE_ENTRY
    */
    struct IQUEUEHEAD {
        struct IQUEUEHEAD *next, *prev;
    };
    typedef struct IQUEUEHEAD iqueue_head;
} // namespace detail

//---------------------------------------------------------------------
// queue start
//---------------------------------------------------------------------
/**
 @def IQUEUE_HEAD_INIT
 @brief 通用链表头初始化列表
*/
#define IQUEUE_HEAD_INIT(name) { &(name), &(name) }

/**
 @def IQUEUE_HEAD
 @brief 构造通用链表头
*/
#define IQUEUE_HEAD(name) \
    struct IQUEUEHEAD name = IQUEUE_HEAD_INIT(name)

/**
 @def IQUEUE_INIT
 @brief 节点初始化,指向自身
*/
#define IQUEUE_INIT(ptr) ( \
    (ptr)->next = (ptr), (ptr)->prev = (ptr))

/**
 @def IOFFSETOF
 @brief 取TYPE的MEMBER到该TYPE结构体基地址的偏移字节数
*/
#define IOFFSETOF(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 @def IOFFSETOF
 @brief 取结构体地址
*/
#define ICONTAINEROF(ptr, type, member) ( \
        (type*)( ((char*)((type*)ptr)) - IOFFSETOF(type, member)) )

/**
 @def IQUEUE_ENTRY
 @brief 通用链表取数据域指针
 @note
 关于 IQUEUE_ENTRY 宏:
 1.先看&((type *)0)->member:
     把"0"强制转化为指针类型,则该指针一定指向"0"(数据段基址).
     因为指针是"type *"型的,所以可取到以"0"为基地址的一个type型变量member域的地址.
     那么这个地址也就等于member域到结构体基地址的偏移字节数.

 2.再来看( (type *)( (char*)ptr-(size_t)(&((type *)0)->member) ) ):
     (char *)(ptr)使得指针的加减操作步长为一字节,
     (size_t)(&((type *)0)->member)等于ptr指向的member到该member所在结构体基地址的偏移字节数.
     二者一减便得出该结构体的地址.转换为(type *)型的指针,大功告成.

 比如 ikcp_parse_una 函数中的以下代码:
 @code
 struct IQUEUEHEAD *p, *next;
 for (p = kcp->snd_buf.next; p != &kcp->snd_buf; p = next)
 {
    ikcp_seg *seg = iqueue_entry(p, ikcp_seg, node);
 }
 @endcode
 其中的 ikcp_seg *seg = iqueue_entry(p, ikcp_seg, node) 展开则为:
        ikcp_seg *seg = ( (ikcp_seg*)( ( (char*)((ikcp_seg*)p) ) - ((size_t) &((ikcp_seg *)0)->node) ) );

 则可达到以下效果 :
     通过一个 IQUEUEHEAD 指针 p 得到一个指向该链表节点所在的数据块的 ikcp_seg 指针 seg

 要把源代码中的宏展开,其实只要使用gcc进行预处理即可.
 gcc -E source.c > out.txt
 -E 表示只进行预处理,不进行编译.
 预处理时会把注释当成空格处理掉,如果想保留其中的注释,可以加上 -C选项,即:
 gcc -E -C source.c > out.txt
*/
#define IQUEUE_ENTRY(ptr, type, member) ICONTAINEROF(ptr, type, member)


//---------------------------------------------------------------------
// queue operation
//---------------------------------------------------------------------

/**
 @def IQUEUE_ADD
 @brief 插到链表某一元素的后面
 @code
 head(prev=head next=head),node_1(prev=node_1,next=node_1),node_2(prev=node_2,next=node_2)
 IQUEUE_ADD(node_1,head)
 head<->node_1
 IQUEUE_ADD(node_2,node_1)
 head<->node_1<->node_2<->head
 @endcode
*/
#define IQUEUE_ADD(node, head) (\
    (node)->prev = (head), (node)->next = (head)->next, \
    (head)->next->prev = (node), (head)->next = (node))

/**
 @def IQUEUE_ADD_TAIL
 @brief 插到链表头节点的前面,对于通用循环链表,就是尾插
 @code
 head(prev=head,next=head),node_1(prev=node_1,next=node_1),node_2(prev=node_2,next=node_2)
 IQUEUE_ADD_TAIL(node_1,head)
 node_1<->head
 IQUEUE_ADD_TAIL(node_2,node_1)
 head<->node_2<->node_1<->head
 @endcode
*/
#define IQUEUE_ADD_TAIL(node, head) (\
    (node)->prev = (head)->prev, (node)->next = (head), \
    (head)->prev->next = (node), (head)->prev = (node))

/**
 @def IQUEUE_DEL_BETWEEN
 @brief 删除俩个节点之间的节点
*/
#define IQUEUE_DEL_BETWEEN(p, n) ((n)->prev = (p), (p)->next = (n))

/**
 @def IQUEUE_DEL
 @brief 链表中删除节点
*/
#define IQUEUE_DEL(entry) (\
    (entry)->next->prev = (entry)->prev, \
    (entry)->prev->next = (entry)->next, \
    (entry)->next = 0, (entry)->prev = 0)

/**
 @def IQUEUE_DEL_INIT
 @brief 删除节点并重新初始化
*/
#define IQUEUE_DEL_INIT(entry) do { \
    IQUEUE_DEL(entry); IQUEUE_INIT(entry); } while (0)

/**
 @def IQUEUE_IS_EMPTY
 @brief 链表判空
*/
#define IQUEUE_IS_EMPTY(entry) ((entry) == (entry)->next)

/**
 @def IQUEUE_FOREACH
 @brief 链表遍历(for头)
*/
#define IQUEUE_FOREACH(pos, head) \
    for( (pos) = (head)->next; (pos) != (head) ; (pos) = (pos)->next )

/**
 @def IQUEUE_FOREACH_ENTRY
 @brief 链表成员后向遍历(for头)
*/
#define IQUEUE_FOREACH_ENTRY(iterator, head, TYPE, MEMBER) \
    for ((iterator) = IQUEUE_ENTRY((head)->next, TYPE, MEMBER); \
        (!IQUEUE_IS_EMPTY((head))) && ((iterator) != nullptr) && (&((iterator)->MEMBER) != (head)); \
        (iterator) = IQUEUE_ENTRY((iterator)->MEMBER.next, TYPE, MEMBER))

/**
 @def IQUEUE_FOREACH_ENTRY_PREV
 @brief 链表成员前向遍历(for头)
*/
#define IQUEUE_FOREACH_ENTRY_PREV(iterator, head, TYPE, MEMBER) \
    for ((iterator) = IQUEUE_ENTRY((head)->prev, TYPE, MEMBER); \
        (!IQUEUE_IS_EMPTY((head))) && ((iterator) != nullptr) && (&((iterator)->MEMBER) != (head)); \
        (iterator) = IQUEUE_ENTRY((iterator)->MEMBER.prev, TYPE, MEMBER))

/**
 @def IQUEUE_SPLICE
 @brief 给list换头
 @code
 head(prev=head,next=head),list(prev=node_2,next=node_1)
 list<->node_1<->node_2<->list
 IQUEUE_SPLICE(list,head)
 head<->node_1<->node_2<->head
 @endcode
*/
#define IQUEUE_SPLICE(list, head) if (!IQUEUE_IS_EMPTY(list)) do { \
        iqueue_head *first = (list)->next, *last = (list)->prev; \
        iqueue_head *at = (head)->next; \
        (first)->prev = (head), (head)->next = (first);		\
        (last)->next = (at), (at)->prev = (last); }	while (0)

/**
 @def IQUEUE_SPLICE_INIT
 @brief 给list换头并重新初始化list头
*/
#define IQUEUE_SPLICE_INIT(list, head) do {	\
    IQUEUE_SPLICE(list, head);	IQUEUE_INIT(list); } while (0)

#define iqueue_init		          IQUEUE_INIT
#define iqueue_entry	          IQUEUE_ENTRY
#define iqueue_add		          IQUEUE_ADD
#define iqueue_add_tail	          IQUEUE_ADD_TAIL
#define iqueue_del		          IQUEUE_DEL
#define iqueue_del_init	          IQUEUE_DEL_INIT
#define iqueue_is_empty           IQUEUE_IS_EMPTY
#define iqueue_foreach            IQUEUE_FOREACH
#define iqueue_foreach_entry      IQUEUE_FOREACH_ENTRY
#define iqueue_foreach_entry_prev IQUEUE_FOREACH_ENTRY_PREV
// #define iqueue_splice        IQUEUE_SPLICE
// #define iqueue_splice_init   IQUEUE_SPLICE_INIT

#ifdef _MSC_VER
#pragma warning(disable:4311)
#pragma warning(disable:4312)
#pragma warning(disable:4996)
#endif

#endif // __IQUEUE_DEF__


//---------------------------------------------------------------------
// WORD ORDER
//---------------------------------------------------------------------
#ifndef IWORDS_BIG_ENDIAN

#ifdef _BIG_ENDIAN_
#if _BIG_ENDIAN_
#define IWORDS_BIG_ENDIAN 1
#endif
#endif // _BIG_ENDIAN_

#ifndef IWORDS_BIG_ENDIAN
#if defined(__hppa__) || \
            defined(__m68k__) || defined(mc68000) || defined(_M_M68K) || \
            (defined(__MIPS__) && defined(__MIPSEB__)) || \
            defined(__ppc__) || defined(__POWERPC__) || defined(_M_PPC) || \
            defined(__sparc__) || defined(__powerpc__) || \
            defined(__mc68000__) || defined(__s390x__) || defined(__s390__)
#define IWORDS_BIG_ENDIAN 1
#endif
#endif // IWORDS_BIG_ENDIAN

#ifndef IWORDS_BIG_ENDIAN
#define IWORDS_BIG_ENDIAN  0
#endif

#endif // IWORDS_BIG_ENDIAN

//=====================================================================
namespace detail {
    /**
     @brief KCP数据包
     @note
     cmd: command,标识Segment的指令类型:
          1.数据包(IKCP_CMD_PUSH):
            最基础的Segment,用于发送应用层数据给远端.
            每个数据包有自己的sn,发送后等远端返回ack包才会从缓存中移除(两端通过sn确认哪些包已收到).
          2.ACK包(IKCP_CMD_ACK):
            告诉远端自己已收到了远端发送的哪个编号的数据sn.
          3.窗口大小探测包(IKCP_CMD_WASK):
            询问远端的接收窗口大小.
            本地发送数据时,会根据远端的窗口大小来控制发送的数据量.
            每个数据包的包头中带有发送方当前的接收窗口大小wnd.
            当远端的接收窗口大小为0时,本机将不会再向远端发送数据,
            此时也就不会有远端的回传数据从而导致无法更新远端窗口大小.
            因此需要单独的一类远端窗口大小探测包,在远端接收窗口大小为0时,
            隔一段时间询问一次,从而让本地有机会再开始重传数据.
          4.窗口大小回应包(IKCP_CMD_WINS):
            回应远端自己的数据接收窗口大小.
     frg: fragment,记录了分片时的倒序序号, 当输出数据大于MSS 时,需要将数据进行分片.
          KCP发送数据时会给加入snd_queue的segment分配序号,
          接收端在接收到这些segment时(0代表数据包接收完毕),就会根据frg将若干个segment合并成一个,再返回给应用层.

     kcp包头,24bytes,包含了一些必要的信息,包的结构可以在函数ikcp_encode_seg函数的编码过程中看出来:
     @code
    |<------------ 4 bytes ------------>|
    +--------+--------+--------+--------+
    |               conv                | conv: 会话序号
    +--------+--------+--------+--------+ cmd:  指令类型
    |  cmd   |  frg   |       wnd       | frg:  分片序号
    +--------+--------+--------+--------+ wnd:  接收窗口大小
    |                ts                 | ts:   发送的时间戳
    +--------+--------+--------+--------+
    |                sn                 | sn:   包序号
    +--------+--------+--------+--------+
    |                una                | una:  当前未收到的序号
    +--------+--------+--------+--------+
    |                len                | len:  数据段长度
    +--------+--------+--------+--------+
     @endcode
    */
    struct IKCPSEG
    {
        struct IQUEUEHEAD node;/**< 节点用来串接多个KCP segment,也就是前后向指针. */
        IUINT32 conv;          /**< conversation,会话序号,接收到的数据包与发送的一致才接收此数据包. */
        IUINT32 cmd;           /**< command,标识Segment的指令类型. */
        IUINT32 frg;           /**< fragment,记录了分片时的倒序序号, 当输出数据大于MSS 时,需要将数据进行分片. */
        IUINT32 wnd;           /**< window,滑动接收窗口大小,用于流控, 也就是rcv_queue的可用大小,见ikcp_wnd_unused 函数. */
        IUINT32 ts;            /**< timestamp, 发送时的时间戳,用来估计 RTT. */
        IUINT32 sn;            /**< sequence number, segment的序号(包序号). */
        IUINT32 una;           /**< unacknowledged, 当前未收到的序号: 即代表这个序号之前的包均收到. */
        IUINT32 len;           /**< 数据段的长度. */
        IUINT32 resendts;      /**< resend timestamp, 指定重发的时间戳,当当前时间超过这个时间时,则再重发一次这个包. */
        IUINT32 rto;           /**< retransmit timeout, 超时重传时间间隔. */
        IUINT32 fastack;       /**< 记录ack跳过的次数,用于快速重传, 由函数ikcp_parse_fastack更新. */
        IUINT32 xmit;          /**< 记录发送Segment的次数,用于判断是否达到最大重传次数 */
        char data[1];          /**< 数据段,应用层要发送出去的数据. */
    };

    /**
     @brief KCP 会话上下文
     内部维护了4条队列分别用于管理收发的数据,一个ack数组记录数据包的ack.
    */
    struct IKCPCB
    {
#ifdef KCP_EXTENTION
        IUINT32 rdc_check_ts, rdc_check_interval;           /**< rdc检查时间;rdc检查间隔 */
        IINT32 rdc_rtt_limit, is_rdc_on;                    /**< rdc rtt上限;是否开启rdc */
        IINT32 rdc_close_try_times, rdc_close_try_threshold;/**< rdc关闭次数;rdc关闭次数阈值 */
        IUINT32 snd_cnt, timeout_resnd_cnt;                 /**< PUSH包发送计数;超时重传计数 */
        IUINT32 loss_rate, rdc_loss_rate_limit;             /**< 丢包率;rdc丢包率上限 */
#endif // KCP_EXTENTION
        IUINT32 conv, mtu, mss, state;                 /**< 会话ID;最大传输单元(含包头);最大分片大小(mtu-包头);连接状态(0xFFFFFFFF(-1)表示断开连接) */
        IUINT32 snd_una, snd_nxt, rcv_nxt;             /**< 第一个未确认的包序号;下一个待分配/发的包的序号;待接收的下一个消息序号 */
        IUINT32 ssthresh;                              /**< 拥塞窗口阈值 */
        IINT32 rx_rttval, rx_srtt, rx_rto, rx_minrto;  /**< ack接收rtt平均偏差,用来衡量rtt的抖动;ack接收rtt加权平均时延;由ack接收延迟计算出来的重传超时时间;最小超时重传时间 */
        IUINT32 snd_wnd, rcv_wnd, rmt_wnd, cwnd, probe;/**< 发送窗口大小;接收窗口大小;远端接收窗口大小;拥塞窗口大小;探查变量,IKCP_ASK_TELL告知远端窗口大小,IKCP_ASK_SEND请求远端告知窗口大小 */
        IUINT32 current, interval, ts_flush;           /**< 当前时间戳;内部flush刷新间隔;下一次flush刷新时间戳 */
        IUINT32 nrcv_buf, nsnd_buf;                    /**< 收发缓存区中的Segment数量 */
        IUINT32 nrcv_que, nsnd_que;                    /**< 收发队列中的Segment数量 <rcv_wnd */
        IUINT32 nodelay, updated;                      /**< 是否启动快速重传ack(非0开启);标识是否第一次update(kcp需要上层通过不断的ikcp_update和ikcp_check来驱动kcp的收发过程) */
        IUINT32 ts_probe, probe_wait;/**< 下次探查窗口的时间戳;探查窗口需要等待的时间 */
        IUINT32 dead_link, incr;     /**< 最大重传次数;可发送的最大数据量 */
        struct IQUEUEHEAD snd_queue; /**< 发送队列:send时将segment放入 */
        struct IQUEUEHEAD rcv_queue; /**< 接收队列:recv时将接收缓存rcv_buf中的segment移入接收队列 */
        struct IQUEUEHEAD snd_buf;   /**< 发送缓存:update时将segment从发送队列放入缓存 */
        struct IQUEUEHEAD rcv_buf;   /**< 接收缓存:存放底层接收的数据segment */
        IUINT32 *acklist;            /**< ack列表,所有收到的包ack将放在这里,依次存放sn和ts */
        IUINT32 ackcount;            /**< acklist中的ack数量 */
        IUINT32 ackblock;            /**< acklist数组可用长度,当acklist的容量不足时,需要进行扩容 */
        void *user;                  /**< 传递给output回调的用户数据指针 */
        char *buffer;                /**< 存储消息字节流的缓冲区(mtu+包头)*3,用于flush中发包 */
        int fastresend;              /**< 触发快速重传的重复ack个数 */
        int nocwnd, stream;          /**< 非退让流控模式(非0开启);流模式(非0开启) */
        int logmask;                 /**< 日志筛选类型,方便调试,~0开启全部日志 */
        int(*output)(const char *buf, int len, struct IKCPCB *kcp, void *user); /**< KCP打包后调用底层网络传输函数发送消息 */
        void(*writelog)(const char *log, struct IKCPCB *kcp, void *user);       /**< 日志回调 */
    };

    typedef struct detail::IKCPSEG ikcp_seg;

} // namespace detail

typedef struct detail::IKCPCB ikcpcb;


#define IKCP_LOG_OUTPUT			1
#define IKCP_LOG_INPUT			2
#define IKCP_LOG_SEND			4
#define IKCP_LOG_RECV			8
#define IKCP_LOG_IN_DATA		16
#define IKCP_LOG_IN_ACK			32
#define IKCP_LOG_IN_PROBE		64
#define IKCP_LOG_IN_WINS		128
#define IKCP_LOG_OUT_DATA		256
#define IKCP_LOG_OUT_ACK		512
#define IKCP_LOG_OUT_PROBE		1024
#define IKCP_LOG_OUT_WINS		2048

void ikcp_log(ikcpcb *kcp, int mask, const char *fmt, ...);
namespace detail {
    //=====================================================================
    // KCP BASIC
    //=====================================================================
#ifdef KCP_EXTENTION
    const IUINT32 IKCP_RDC_CHK_INTERVAL = 100;       //rdc检查间隔
    const IUINT32 IKCP_RDC_RTT_LIMIT = 111;          //rdc rtt上限
    const IUINT32 IKCP_RDC_CLOSE_TRY_THRESHOLD = 26; //rdc关闭次数阈值
    const IUINT32 IKCP_RDC_LOSS_RATE_LIMIT = 5;      //rdc丢包率上限值
#endif // KCP_EXTENTION

    const IUINT32 IKCP_RTO_NDL = 30;		// no delay min rto
    const IUINT32 IKCP_RTO_MIN = 100;		// normal min rto
    const IUINT32 IKCP_RTO_DEF = 200;
    const IUINT32 IKCP_RTO_MAX = 60000;
    const IUINT32 IKCP_CMD_PUSH = 81;		// cmd: push data
    const IUINT32 IKCP_CMD_ACK = 82;		// cmd: ACK
    const IUINT32 IKCP_CMD_WASK = 83;		// cmd: window probe (ask)
    const IUINT32 IKCP_CMD_WINS = 84;		// cmd: window size (tell)
    const IUINT32 IKCP_ASK_SEND = 1;		// need to send IKCP_CMD_WASK
    const IUINT32 IKCP_ASK_TELL = 2;		// need to send IKCP_CMD_WINS
    const IUINT32 IKCP_WND_SND = 32;
    const IUINT32 IKCP_WND_RCV = 128;       // must >= max fragment size
    const IUINT32 IKCP_MTU_DEF = 1400;
    const IUINT32 IKCP_ACK_FAST = 3;
    const IUINT32 IKCP_INTERVAL = 100;
    const IUINT32 IKCP_INTERVAL_MIN = 10;
    const IUINT32 IKCP_INTERVAL_LIMIT = 5000;
    const IUINT32 IKCP_OVERHEAD = 24;		// kcp packet header size:24bytes
    const IUINT32 IKCP_DEADLINK = 20;       // max resend number
    const IUINT32 IKCP_THRESH_INIT = 2;
    const IUINT32 IKCP_THRESH_MIN = 2;
    const IUINT32 IKCP_PROBE_INIT = 7000;	// 探查时间间隔 7 secs
    const IUINT32 IKCP_PROBE_LIMIT = 120000;// 探查时间上限 120 secs

    //---------------------------------------------------------------------
    // encode / decode
    //---------------------------------------------------------------------

    /** @brief encode 8 bits unsigned int */
    static inline char *ikcp_encode8u(char *p, unsigned char c) {
        *(unsigned char*)p++ = c;
        return p;
    }

    /** @brief decode 8 bits unsigned int */
    static inline const char *ikcp_decode8u(const char *p, unsigned char *c) {
        *c = *(unsigned char*)p++;
        return p;
    }

    /** @brief encode 16 bits unsigned int (lsb) */
    static inline char *ikcp_encode16u(char *p, unsigned short w)
    {
#if IWORDS_BIG_ENDIAN
        *(unsigned char*)(p + 0) = (w & 255);
        *(unsigned char*)(p + 1) = (w >> 8);
#else
        *(unsigned short*)(p) = w;
#endif
        p += 2;
        return p;
    }

    /** @brief decode 16 bits unsigned int (lsb) */
    static inline const char *ikcp_decode16u(const char *p, unsigned short *w)
    {
#if IWORDS_BIG_ENDIAN
        *w = *(const unsigned char*)(p + 1);
        *w = *(const unsigned char*)(p + 0) + (*w << 8);
#else
        *w = *(const unsigned short*)p;
#endif
        p += 2;
        return p;
    }

    /** @brief encode 32 bits unsigned int (lsb) */
    static inline char *ikcp_encode32u(char *p, IUINT32 l)
    {
#if IWORDS_BIG_ENDIAN
        *(unsigned char*)(p + 0) = (unsigned char)((l >> 0) & 0xff);
        *(unsigned char*)(p + 1) = (unsigned char)((l >> 8) & 0xff);
        *(unsigned char*)(p + 2) = (unsigned char)((l >> 16) & 0xff);
        *(unsigned char*)(p + 3) = (unsigned char)((l >> 24) & 0xff);
#else
        *(IUINT32*)p = l;
#endif
        p += 4;
        return p;
    }

    /** @brief decode 32 bits unsigned int (lsb) */
    static inline const char *ikcp_decode32u(const char *p, IUINT32 *l)
    {
#if IWORDS_BIG_ENDIAN
        *l = *(const unsigned char*)(p + 3);
        *l = *(const unsigned char*)(p + 2) + (*l << 8);
        *l = *(const unsigned char*)(p + 1) + (*l << 8);
        *l = *(const unsigned char*)(p + 0) + (*l << 8);
#else
        *l = *(const IUINT32*)p;
#endif
        p += 4;
        return p;
    }

    static inline IUINT32 _imin_(IUINT32 a, IUINT32 b) {
        return a <= b ? a : b;
    }

    static inline IUINT32 _imax_(IUINT32 a, IUINT32 b) {
        return a >= b ? a : b;
    }

    /** @brief 范围限制 保证结果值在范围内 */
    static inline IUINT32 _ibound_(IUINT32 lower, IUINT32 middle, IUINT32 upper) {
        return _imin_(_imax_(lower, middle), upper);
    }

    static inline long _itimediff(IUINT32 later, IUINT32 earlier) {
        return ((IINT32)(later - earlier));
    }

    static void*(*ikcp_malloc_hook)(size_t) = NULL;
    static void(*ikcp_free_hook)(void *) = NULL;

    /** @brief internal malloc */
    static void* ikcp_malloc(size_t size) {
        return ikcp_malloc_hook ? ikcp_malloc_hook(size) : malloc(size);
    }

    /** @brief internal free */
    static void ikcp_free(void *ptr) {
        ikcp_free_hook ? ikcp_free_hook(ptr) : free(ptr);
    }

    /** @brief allocate a new kcp segment */
    static ikcp_seg* ikcp_segment_new(ikcpcb *kcp, int size) {
        return (ikcp_seg*)ikcp_malloc(sizeof(ikcp_seg) + size);
    }

    /** @brief delete a segment */
    static void ikcp_segment_delete(ikcpcb *kcp, ikcp_seg *seg) {
        ikcp_free(seg);
    }

    /** @brief output segment */
    static int ikcp_output(ikcpcb *kcp, const void *data, int size) {
        assert(kcp);
        assert(kcp->output);
        ikcp_log(kcp, IKCP_LOG_OUTPUT, "[kcp] output %ld bytes", (long)size);
        if (size == 0) return 0;
        return kcp->output((const char*)data, size, kcp, kcp->user);
    }

    /** @brief 打印队列分片信息sn ts */
    static void ikcp_qprint(const char *name, const iqueue_head *head)
    {
        printf("<%s>: [", name);
        ikcp_seg *seg = nullptr;
        iqueue_foreach_entry(seg, head, ikcp_seg, node) {
            printf("(%lu %d)", (unsigned long)seg->sn, (int)(seg->ts % 10000));
        }
        printf("]\n");
    }

    /** @brief 根据rtt更新ACK */
    static void ikcp_update_ack(ikcpcb *kcp, IINT32 rtt)
    {
        if (kcp->rx_srtt == 0) {
            kcp->rx_srtt = rtt;
            kcp->rx_rttval = rtt / 2;
        }
        else {
            IINT32 delta = rtt - kcp->rx_srtt;
            if (delta < 0) delta = -delta;
            kcp->rx_rttval = (3 * kcp->rx_rttval + delta) / 4; // 平均偏差更新权重0.25
            kcp->rx_srtt = (7 * kcp->rx_srtt + rtt) / 8;       // 平均时延更新权重0.125
            if (kcp->rx_srtt < 1) kcp->rx_srtt = 1;
        }
        IINT32 rto = kcp->rx_srtt + _imax_(kcp->interval, 4 * kcp->rx_rttval); // 类似TCP
        kcp->rx_rto = _ibound_(kcp->rx_minrto, rto, IKCP_RTO_MAX); // 范围限制
    }

    /** @brief 更新本地snd_una:第一个未确认包序号 */
    static void ikcp_shrink_buf(ikcpcb *kcp)
    {
        if (!iqueue_is_empty(&kcp->snd_buf)) {
            ikcp_seg *seg = iqueue_entry(kcp->snd_buf.next, ikcp_seg, node);
            kcp->snd_una = seg->sn; // snd_una指向snd_buf首端
        }
        else {
            kcp->snd_una = kcp->snd_nxt;// snd_una指向下一个待分配包序号
        }
    }

    /** @brief 分析ack的sn,在snd_buf中找到第一个包序号为sn的ikcp_seg删除之 */
    static void ikcp_parse_ack(ikcpcb *kcp, IUINT32 sn)
    {
        if (sn < kcp->snd_una || sn >= kcp->snd_nxt)
            return;// sn小于snd_una或大于等于snd_nxt,忽略该包,snd_una之前是完备的,snd_nxt之后未发送,不应收到ACK

        iqueue_head *p;
        iqueue_foreach(p, &kcp->snd_buf) {
            ikcp_seg *seg = iqueue_entry(p, ikcp_seg, node);
            if (seg->sn == sn) {
                iqueue_del(p);
                ikcp_segment_delete(kcp, seg);
                kcp->nsnd_buf--;
                break;
            }
            if (seg->sn > sn) {
                break;
            }
        }
    }

    /** @brief 分析una,删除snd_buf中小于una的segment(都被远端收到了) */
    static void ikcp_parse_una(ikcpcb *kcp, IUINT32 una)
    {
        iqueue_head *p, *next;
        for (p = kcp->snd_buf.next; p != &kcp->snd_buf; p = next) {
            ikcp_seg *seg = iqueue_entry(p, ikcp_seg, node);
            next = p->next;
            if (seg->sn < una) {
                iqueue_del(p);
                ikcp_segment_delete(kcp, seg);
                kcp->nsnd_buf--;
            }
            else {
                break;
            }
        }
    }

    /**
     @brief 处理快速重传
     @note
     遍历snd_buf更新各个Segment中ACK跳过的次数,
     若Segment的sn小于接收到的ACK包的sn, 则Segment的fastack++,
     若fastack超过指定阈值,则启动快速重传.
    */
    static void ikcp_parse_fastack(ikcpcb *kcp, IUINT32 sn)
    {
        if (sn < kcp->snd_una || sn >= kcp->snd_nxt)
            return;

        ikcp_seg *seg = nullptr;
        iqueue_foreach_entry(seg, &kcp->snd_buf, ikcp_seg, node) {
            if (seg->sn < sn) { // 若seg的sn小于接收到的所有ACK包中的最大sn
                seg->fastack++;
            }
            else {
                break;
            }
        }
    }

    /**
     @brief 将该报文的确认ACK放入ACK列表acklist中
     push当前包的ACK给远端(会在flush中发送ACK出去)
    */
    static void ikcp_ack_push(ikcpcb *kcp, IUINT32 sn, IUINT32 ts)
    {
        IUINT32 newsize = kcp->ackcount + 1;
        if (newsize > kcp->ackblock) {
            IUINT32 *acklist;
            IUINT32 newblock;

            for (newblock = 8; newblock < newsize; newblock <<= 1); // newblock *= 2;
            acklist = (IUINT32*)ikcp_malloc(newblock * sizeof(IUINT32) * 2);
            assert(acklist != NULL);
            if (kcp->acklist != NULL) {
                IUINT32 i;
                for (i = 0; i < kcp->ackcount; i++) {
                    acklist[i * 2 + 0] = kcp->acklist[i * 2 + 0];
                    acklist[i * 2 + 1] = kcp->acklist[i * 2 + 1];
                }
                ikcp_free(kcp->acklist);
            }

            kcp->acklist = acklist;
            kcp->ackblock = newblock;
        }

        IUINT32 *ptr = &kcp->acklist[kcp->ackcount * 2];
        ptr[0] = sn;
        ptr[1] = ts;
        kcp->ackcount++;
    }

    /** @brief 从acklist中取sn和ts */
    static void ikcp_ack_get(const ikcpcb *kcp, int p, IUINT32 *sn, IUINT32 *ts)
    {
        if (sn) sn[0] = kcp->acklist[p * 2 + 0];
        if (ts) ts[0] = kcp->acklist[p * 2 + 1];
    }

    /**
     @brief 解析数据
     @note
     在rcv_buf中遍历一次,判断是否已经接收过这个数据包,
     如果数据包不存在则添加到rcv_buf中,之后将可用的Segment转移到rcv_queue中
    */
    static void ikcp_parse_data(ikcpcb *kcp, ikcp_seg *newseg)
    {
        assert(kcp);
        assert(newseg);

        if (newseg->sn >= (kcp->rcv_nxt + kcp->rcv_wnd)
            || newseg->sn < kcp->rcv_nxt) {
            ikcp_segment_delete(kcp, newseg);// 超出接收窗口大小或rcv_queue已经接收过这个sn的数据包了
            return;
        }

        // rcv_buf 从后往前遍历,判断是否已经接收过这个数据包,并且找到新数据newseg应该插入到 rcv_buf 的位置

        int repeat = 0;
        iqueue_head *p, *prev;
        for (p = kcp->rcv_buf.prev; p != &kcp->rcv_buf; p = prev) {
            ikcp_seg *seg = iqueue_entry(p, ikcp_seg, node);
            prev = p->prev;

            if (seg->sn == newseg->sn) {
                repeat = 1;//已经接收过
                break;
            }
            if (seg->sn < newseg->sn) {
                break;//未接收
            }
        }

        if (repeat == 0) {
            iqueue_init(&newseg->node);
            iqueue_add(&newseg->node, p);
            kcp->nrcv_buf++;
        }
        else {
            // 如果已经接收过了,则丢弃
            ikcp_segment_delete(kcp, newseg);
        }

#if 0
        ikcp_qprint("rcvbuf", &kcp->rcv_buf);
        printf("rcv_nxt=%lu\n", kcp->rcv_nxt);
#endif

        //遍历rcv_buf找到与下一个待接收的序号相同的segment,且接收队列还有位置
        //segment移出rcv_buf移入rcv_queue,rcv_nxt的连续性保证rcv_queue的有序
        ikcp_seg *seg = nullptr;
        iqueue_foreach_entry(seg, &kcp->rcv_buf, ikcp_seg, node) {
            if (seg->sn == kcp->rcv_nxt && kcp->nrcv_que < kcp->rcv_wnd) {
                iqueue_del_init(&seg->node);
                kcp->nrcv_buf--;
                iqueue_add_tail(&seg->node, &kcp->rcv_queue);
                kcp->nrcv_que++;
                kcp->rcv_nxt++;
            }
            else {
                break;
            }
        };

#if 0
        ikcp_qprint("queue", &kcp->rcv_queue);
        printf("rcv_nxt=%lu\n", kcp->rcv_nxt);
        printf("snd(buf=%d, queue=%d)\n", kcp->nsnd_buf, kcp->nsnd_que);
        printf("rcv(buf=%d, queue=%d)\n", kcp->nrcv_buf, kcp->nrcv_que);
#endif
    }

    /** @brief seg内容复制到ptr */
    static char *ikcp_encode_seg(char *ptr, const ikcp_seg *seg)
    {
        ptr = ikcp_encode32u(ptr, seg->conv);
        ptr = ikcp_encode8u(ptr, (IUINT8)seg->cmd);
        ptr = ikcp_encode8u(ptr, (IUINT8)seg->frg);
        ptr = ikcp_encode16u(ptr, (IUINT16)seg->wnd);
        ptr = ikcp_encode32u(ptr, seg->ts);
        ptr = ikcp_encode32u(ptr, seg->sn);
        ptr = ikcp_encode32u(ptr, seg->una);
        ptr = ikcp_encode32u(ptr, seg->len);
        return ptr;
    }

    /** @brief 返回剩余接收窗口大小(接收窗口大小-接收队列大小) */
    static inline int ikcp_wnd_unused(const ikcpcb *kcp)
    {
        if (kcp->nrcv_que < kcp->rcv_wnd) {
            return kcp->rcv_wnd - kcp->nrcv_que; // 剩余接收窗口大小(接收窗口大小-接收队列大小)
        }
        return 0;
    }

} // namespace detail


//=====================================================================
// interface
//=====================================================================


//---------------创建及设置-------------------

/** @brief 可以指定allocator */
void inline ikcp_allocator(void* (*new_malloc)(size_t), void(*new_free)(void*))
{
    using namespace detail;
    ikcp_malloc_hook = new_malloc;
    ikcp_free_hook = new_free;
};

/**
 @brief 从指针获取conv
 @param[in,out] ptr IUINT32*类型,读一个conv后ptr+=4
*/
IUINT32 ikcp_getconv(const void *ptr)
{
    using namespace detail;
    IUINT32 conv;
    ikcp_decode32u((const char*)ptr, &conv);
    return conv;
};

/**
 @brief 创建 kcp上下文
 @param conv 会话标识,俩端必须相同
 @param user 用户数据用于output回调使用
*/
ikcpcb* ikcp_create(IUINT32 conv, void *user)
{
    /*---------------------------------------------------------
    create a new kcpcb
    首先需要创建一个kcp用于管理接下来的工作过程,
    在创建的时候,默认的发送、接收以及远端的窗口大小均为32,
    mtu大小为1400bytes,mss为1400-24=1376bytes,
    超时重传时间为200毫秒,最小重传时间为100毫秒,
    kcp内部间隔最小时间为100毫秒(kcp->interval = IKCP_INTERVAL;),
    最大重发次数 dead_link 为IKCP_DEADLINK即20.
    ------------------------------------------------------------*/
    using namespace detail;
    ikcpcb *kcp = (ikcpcb*)ikcp_malloc(sizeof(ikcpcb));
    if (kcp == NULL) return NULL;

#ifdef KCP_EXTENTION
    kcp->rdc_check_ts = 0;
    kcp->rdc_check_interval = IKCP_RDC_CHK_INTERVAL;
    kcp->rdc_rtt_limit = IKCP_RDC_RTT_LIMIT;
    kcp->is_rdc_on = 0;
    kcp->rdc_close_try_times = 0;
    kcp->rdc_close_try_threshold = IKCP_RDC_CLOSE_TRY_THRESHOLD;
    kcp->snd_cnt = 0;
    kcp->timeout_resnd_cnt = 0;
    kcp->loss_rate = 0;
    kcp->rdc_loss_rate_limit = IKCP_RDC_LOSS_RATE_LIMIT;
#endif // KCP_EXTENTION

    kcp->conv = conv;
    kcp->user = user;
    kcp->snd_una = 0;
    kcp->snd_nxt = 0;
    kcp->rcv_nxt = 0;
    kcp->ts_probe = 0;
    kcp->probe_wait = 0;
    kcp->snd_wnd = IKCP_WND_SND;
    kcp->rcv_wnd = IKCP_WND_RCV;
    kcp->rmt_wnd = IKCP_WND_RCV;
    kcp->cwnd = 0;
    kcp->incr = 0;
    kcp->probe = 0;
    kcp->mtu = IKCP_MTU_DEF;
    kcp->mss = kcp->mtu - IKCP_OVERHEAD;
    kcp->stream = 0;

    kcp->buffer = (char*)ikcp_malloc((kcp->mtu + IKCP_OVERHEAD) * 3);
    if (kcp->buffer == NULL) {
        ikcp_free(kcp);
        return NULL;
    }

    iqueue_init(&kcp->snd_queue);
    iqueue_init(&kcp->rcv_queue);
    iqueue_init(&kcp->snd_buf);
    iqueue_init(&kcp->rcv_buf);
    kcp->nrcv_buf = 0;
    kcp->nsnd_buf = 0;
    kcp->nrcv_que = 0;
    kcp->nsnd_que = 0;
    kcp->state = 0;
    kcp->acklist = NULL;
    kcp->ackblock = 0;
    kcp->ackcount = 0;
    kcp->rx_srtt = 0;
    kcp->rx_rttval = 0;
    kcp->rx_rto = IKCP_RTO_DEF;
    kcp->rx_minrto = IKCP_RTO_MIN;
    kcp->current = 0;
    kcp->interval = IKCP_INTERVAL;
    kcp->ts_flush = IKCP_INTERVAL;
    kcp->nodelay = 0;
    kcp->updated = 0;
    kcp->logmask = ~0;
    kcp->ssthresh = IKCP_THRESH_INIT;
    kcp->fastresend = 0;
    kcp->nocwnd = 0;
    kcp->dead_link = IKCP_DEADLINK;
    kcp->output = NULL;
    kcp->writelog = NULL;

    return kcp;
};

/** @brief release kcp上下文 */
void ikcp_release(ikcpcb *kcp)
{
    using namespace detail;
    assert(kcp);
    if (kcp) {
        ikcp_seg *seg = nullptr;
        iqueue_foreach_entry(seg, &kcp->snd_buf, ikcp_seg, node) {
            iqueue_del(&seg->node);
            ikcp_segment_delete(kcp, seg);
        }
        iqueue_foreach_entry(seg, &kcp->rcv_buf, ikcp_seg, node) {
            iqueue_del(&seg->node);
            ikcp_segment_delete(kcp, seg);
        }
        iqueue_foreach_entry(seg, &kcp->snd_queue, ikcp_seg, node) {
            iqueue_del(&seg->node);
            ikcp_segment_delete(kcp, seg);
        }
        iqueue_foreach_entry(seg, &kcp->rcv_queue, ikcp_seg, node) {
            iqueue_del(&seg->node);
            ikcp_segment_delete(kcp, seg);
        }
        if (kcp->buffer) ikcp_free(kcp->buffer);
        if (kcp->acklist) ikcp_free(kcp->acklist);

        kcp->nrcv_buf = 0;
        kcp->nsnd_buf = 0;
        kcp->nrcv_que = 0;
        kcp->nsnd_que = 0;
        kcp->ackcount = 0;
        kcp->buffer = NULL;
        kcp->acklist = NULL;
        ikcp_free(kcp);
    }
};

/** @brief 设置MTU大小 默认1400 */
int ikcp_setmtu(ikcpcb *kcp, int mtu)
{
    using namespace detail;
    assert(kcp);
    if (mtu < 50 || mtu < (int)IKCP_OVERHEAD)
        return -1;
    char *buffer = (char*)ikcp_malloc((mtu + IKCP_OVERHEAD) * 3);
    if (buffer == NULL)
        return -2;

    kcp->mtu = mtu;
    kcp->mss = kcp->mtu - IKCP_OVERHEAD;

    ikcp_free(kcp->buffer);
    kcp->buffer = buffer;
    return 0;
};

/** @brief 设置收发窗口大小 */
int ikcp_wndsize(ikcpcb *kcp, int sndwnd, int rcvwnd)
{
    using namespace detail;
    assert(kcp);
    if (sndwnd > 0) {
        kcp->snd_wnd = sndwnd;
    }
    if (rcvwnd > 0) {   // must >= max fragment size
        kcp->rcv_wnd = _imax_(rcvwnd, IKCP_WND_RCV);
    }
    return 0;
};

/**
 @brief 快速发送设置
 @param nodelay 是否非退让流控模式,默认false 0
 @param interval 内部刷新间隔ms.默认100ms
 @param resend 是否快速重传,默认false 0,大于0的为快传阈值(ACK跨越几次重传)
 @param nc 0:普通拥塞控制(默认), 1:关闭拥塞控制
 @note
 fastest: ikcp_nodelay(kcp, 1, 20, 2, 1)
*/
int ikcp_nodelay(ikcpcb *kcp, int nodelay, int interval, int resend, int nc)
{
    using namespace detail;
    assert(kcp);
    if (nodelay >= 0) {
        kcp->nodelay = nodelay;
        if (nodelay) {
            kcp->rx_minrto = IKCP_RTO_NDL;  //最小重传超时时间(如果需要可以设置更小)
        }
        else {
            kcp->rx_minrto = IKCP_RTO_MIN;
        }
    }
    if (interval >= 0) {
        if (interval > IKCP_INTERVAL_LIMIT)
            interval = IKCP_INTERVAL_LIMIT;
        else if (interval < IKCP_INTERVAL_MIN)
            interval = IKCP_INTERVAL_MIN;
        kcp->interval = interval; //内部flush刷新时间
    }
    if (resend >= 0) {// ACK被跳过resend次数后直接重传该包, 而不等待超时
        kcp->fastresend = resend; // fastresend : 触发快速重传的重复ACK个数
    }
    if (nc >= 0) {
        kcp->nocwnd = nc;
    }
    return 0;
};

/** @brief 设置update内部刷新间隔,默认100ms */
int ikcp_interval(ikcpcb *kcp, int interval)
{
    if (interval > 5000) interval = 5000;
    else if (interval < 10) interval = 10;
    kcp->interval = interval;
    return 0;
};

/** @brief 记录日志 */
void ikcp_log(ikcpcb *kcp, int mask, const char *fmt, ...)
{
    if ((mask & kcp->logmask) == 0 || kcp->writelog == NULL) return;
    char buffer[1024];
    va_list argptr;
    va_start(argptr, fmt);
    vsprintf(buffer, fmt, argptr);
    va_end(argptr);
    kcp->writelog(buffer, kcp, kcp->user);
};

//---------------KCP驱动-------------------

/**
 @brief 探测何时需要调用ikcp_update
 @return ms
 @note
 具体的时间取决于上次update后更新的下次时间,snd_buf中的最小超时重传时间以及update内部更新时间中的最小值
 如果没有ikcp_input/_send调用,则使用ikcp_check返回的时间调用ikcp_update,以减少不必要的调用
 用途:实现一个类似epoll的机制,或在处理大量kcp连接时优化ikcp_update.
*/
IUINT32 ikcp_check(const ikcpcb *kcp, IUINT32 current)
{
    using namespace detail;
    IUINT32 ts_flush = kcp->ts_flush;
    IINT32 tm_packet = 0x7fffffff;//下一次重传时间差

    if (kcp->updated == 0) {
        return current;//如果未调用过update，立马调用
    }

    if (_itimediff(current, ts_flush) >= 10000 ||
        _itimediff(current, ts_flush) < -10000) {
        ts_flush = current; //超过10sec或下次刷新间隔太长,设置为应该立即调用
    }

    if (_itimediff(current, ts_flush) >= 0) {
        return current;
    }

    IUINT32 tm_flush = (IUINT32)_itimediff(ts_flush, current);// 下一次刷新时间差，此时ts_flush > current

    ikcp_seg *seg;
    iqueue_foreach_entry(seg, &kcp->snd_buf, ikcp_seg, node) {
        IINT32 diff = _itimediff(seg->resendts, current);
        if (diff <= 0) {
            return current;//立即重传
        }
        if (diff < tm_packet) tm_packet = diff;//遍历最小值
    }

    //计算重传时间差,下一次刷新时间差与内部刷新间隔之间的最小值
    IUINT32 minimal = _imin_(_imin_((IUINT32)tm_packet, tm_flush), kcp->interval);
    return current + minimal;
};

/**
 @brief 内部发包
 @note
 给远端发送ack确认包
 检查是否需要窗口探测(发送窗口探测包或窗口告知包)
 进行窗口计算控制，将需要发送的Segment从snd_queue移动到snd_buf中
 检查snd_buf，处理快速重传与超时重传
 @see ikcp_output
*/
void ikcp_flush(ikcpcb *kcp)
{
    using namespace detail;
    //检查 kcp->update 是否更新,未更新直接返回.
    //kcp->update 由 ikcp_update 更新,
    //上层应用需要每隔一段时间(10-100ms)调用 ikcp_update 来驱动 KCP 发送数据;
    if (kcp->updated == 0) return;

    IUINT32 current = kcp->current;
    char *buffer = kcp->buffer;
    char *ptr = buffer;

    int change = 0; // 标识需要快速重传(ack跳过次数超阈值)
    int lost = 0;   // 标识发生报文丢失(超时)

    ikcp_seg seg;
    seg.conv = kcp->conv;
    seg.cmd = IKCP_CMD_ACK;
    seg.frg = 0;
    seg.wnd = ikcp_wnd_unused(kcp);
    seg.una = kcp->rcv_nxt;
    seg.len = 0;
    seg.sn = 0;
    seg.ts = 0;

    // 准备将acklist中记录的ACK报文发送出去,取acklist中ACK报文的sn和ts字段,填充到待发ack包
    int ackcount = kcp->ackcount;
    for (int i = 0; i < ackcount; i++) {
        int size = (int)(ptr - buffer);//seg大小
        if (size + (int)IKCP_OVERHEAD > (int)kcp->mtu) {
            //1个mtu内无法继续添加ACK包，先发送缓存区内的数据包
            ikcp_output(kcp, buffer, size);
            ptr = buffer;
        }
        ikcp_ack_get(kcp, i, &seg.sn, &seg.ts);
        ptr = ikcp_encode_seg(ptr, &seg);
    }
    kcp->ackcount = 0;

    // 如果远端窗口大小为0需要对远端窗口进行探测.
    // 由于 KCP 流量控制依赖于远端通知其可接受窗口的大小,
    // 一旦远端接受窗口 kcp->rmt_wnd 为0,那么本地将不会再向远端发送数据,
    // 因此就没有机会从远端接受 ACK 报文,从而没有机会更新远端窗口大小.
    // 在这种情况下,KCP 需要发送窗口探测报文到远端,待远端回复窗口大小后,后续传输才可以继续
    if (kcp->rmt_wnd == 0) {
        if (kcp->probe_wait == 0) {
            // 初始化探测间隔和下一次探测时间
            kcp->probe_wait = IKCP_PROBE_INIT;
            kcp->ts_probe = kcp->current + kcp->probe_wait;
        }
        else {
            if (_itimediff(kcp->current, kcp->ts_probe) >= 0) { // 当前时间 >= 下一次探查窗口的时间
                if (kcp->probe_wait < IKCP_PROBE_INIT)
                    kcp->probe_wait = IKCP_PROBE_INIT;
                kcp->probe_wait += kcp->probe_wait / 2;   // 等待时间变为之前的1.5倍
                if (kcp->probe_wait > IKCP_PROBE_LIMIT)
                    kcp->probe_wait = IKCP_PROBE_LIMIT;   // 若超过上限,设置为上限值
                kcp->ts_probe = kcp->current + kcp->probe_wait;  // 计算下次探查窗口的时间戳
                kcp->probe |= IKCP_ASK_SEND;         // 设置探查变量.IKCP_ASK_SEND表示请求远端告知窗口大小
            }
        }
    }
    else {
        kcp->ts_probe = 0;
        kcp->probe_wait = 0;
    }

    // 发送窗口探测报文
    if (kcp->probe & IKCP_ASK_SEND) {
        seg.cmd = IKCP_CMD_WASK;
        int size = (int)(ptr - buffer);
        if (size + (int)IKCP_OVERHEAD > (int)kcp->mtu) {
            ikcp_output(kcp, buffer, size);
            ptr = buffer;
        }
        ptr = ikcp_encode_seg(ptr, &seg);
    }

    // 发送窗口告知报文
    if (kcp->probe & IKCP_ASK_TELL) {
        seg.cmd = IKCP_CMD_WINS;
        int size = (int)(ptr - buffer);
        if (size + (int)IKCP_OVERHEAD > (int)kcp->mtu) {
            ikcp_output(kcp, buffer, size);
            ptr = buffer;
        }
        ptr = ikcp_encode_seg(ptr, &seg);
    }

    kcp->probe = 0;

    // 计算本次发送可用的窗口大小,这里 KCP 采用了可以配置的策略,
    // 正常流控下,KCP 的窗口大小由
    // 发送窗口snd_wnd和远端接收窗口rmt_wnd以及根据拥塞控制计算得到的kcp->cwnd三者共同决定;
    // 但是当开启了nocwnd模式(非退让流控)时,窗口大小仅由前两者决定
    IUINT32 cwnd = _imin_(kcp->snd_wnd, kcp->rmt_wnd);
    if (kcp->nocwnd == 0) cwnd = _imin_(kcp->cwnd, cwnd);

    // 将缓存在snd_queue中的数据移到snd_buf中等待发送
    // 移动的包的数量不会超过snd_una + cwnd - snd_nxt,确保发送的数据不会让接收方的接收队列溢出.
    // 该功能类似于TCP协议中的滑动窗口.
    while ((kcp->snd_nxt < kcp->snd_una + cwnd) && !iqueue_is_empty(&kcp->snd_queue))
    {
        ikcp_seg *newseg = iqueue_entry(kcp->snd_queue.next, ikcp_seg, node);
        iqueue_del(&newseg->node);                      //从发送消息队列中,删除节点
        iqueue_add_tail(&newseg->node, &kcp->snd_buf);  //然后把删除的节点,加入到kcp的发送缓存队列中
        kcp->nsnd_que--;
        kcp->nsnd_buf++;

        newseg->conv = kcp->conv;     //会话id
        newseg->cmd = IKCP_CMD_PUSH;
        newseg->wnd = seg.wnd;
        newseg->ts = current;
        newseg->sn = kcp->snd_nxt++;  //下一个待发包的序号
        newseg->una = kcp->rcv_nxt;   //待接收的下一个消息序号
        newseg->resendts = current;   //下次超时重传的时间戳
        newseg->rto = kcp->rx_rto;    //由ack接收延迟计算出来的重传超时时间
        newseg->fastack = 0;          //收到ACK时计算的该分片被跳过的累计次数
        newseg->xmit = 0;             //发送分片的次数,每发送一次加一
    }

    // 在发送数据之前,先设置快重传的次数和重传间隔;
    // KCP 允许设置快重传的次数,即 fastresend 参数.
    // 例如设置 fastresend 为2,并且发送端发送了1,2,3,4,5几个包,
    // 收到远端的ACK: 1, 3, 4, 5,当收到ACK3时,KCP知道2被跳过1次,
    // 收到ACK4时,知道2被“跳过”了2次,此时可以认为2号丢失,不用等超时,直接重传2号包;
    // 每个报文的 fastack 记录了该报文被跳过了几次,
    // 由函数 ikcp_parse_fastack 更新.于此同时,KCP 也允许设置 nodelay 参数,
    // 当激活该参数时,每个报文的超时重传时间将由 x2 变为 x1.5,即加快报文重传:
    IUINT32 resent = (kcp->fastresend > 0) ? (IUINT32)kcp->fastresend : 0xffffffff; // 快重传次数
    IUINT32 rtomin = (kcp->nodelay == 0) ? (kcp->rx_rto >> 3) : 0; // nodelay模式没有最小rto，否则为rx_rto/8

    // 遍历snd_buf处理第一次发送及重传
    ikcp_seg * segment = nullptr;
    iqueue_foreach_entry(segment, &kcp->snd_buf, ikcp_seg, node)
    {
        int needsend = 0;
        if (segment->xmit == 0) {
            // 第一次发送,xmit为0,赋值rto及resendts
            needsend = 1;
            segment->xmit++;
            segment->rto = kcp->rx_rto;
            segment->resendts = current + segment->rto + rtomin;
        }
        else if (_itimediff(current, segment->resendts) >= 0) {
            // 超时重传,超过segment重发时间,却仍在send_buf中,说明长时间未收到ACK,认为丢失,重发
            needsend = 1;
            segment->xmit++;
#ifdef KCP_EXTENTION
            kcp->timeout_resnd_cnt++;
#endif // KCP_EXTENTION
            // 更新重传时间,默认rto*2,开启nodelay模式则rto*1.5,并记录lost标志.
            if (kcp->nodelay == 0) {
                segment->rto += kcp->rx_rto; // 以2倍的方式来增长(TCP的RTO默认也是2倍增长)
            }
            else {
                segment->rto += kcp->rx_rto / 2; // 可以以1.5倍的速度增长
            }
            segment->resendts = current + segment->rto;
            lost = 1; // 记录出现了报文丢失
        }
        else if (segment->fastack >= resent) {
            // 快速重传,达到快速重传阈值,重新发送
            needsend = 1;
            segment->xmit++;
            segment->fastack = 0;
            segment->resendts = current + segment->rto;
            change = 1;  // 标识快重传发生
        }

        if (needsend) {
            segment->ts = current;
            segment->wnd = seg.wnd;
            segment->una = kcp->rcv_nxt;

            int size = (int)(ptr - buffer);//缓存kcp包大小
            int need = IKCP_OVERHEAD + segment->len; //seg包头大小 + seg数据段大小
            if (size + need > (int)kcp->mtu) {
                ikcp_output(kcp, buffer, size);//1个mtu内无法继续添加需求大小的数据包，先发送当前缓存区的数据包
#ifdef KCP_EXTENTION
                kcp->snd_cnt++;
#endif // KCP_EXTENTION
                ptr = buffer;
            }
            ptr = ikcp_encode_seg(ptr, segment);

            if (segment->len > 0) {//复制数据到kcp包头之后(ptr已指向数据段起始点)
                memcpy(ptr, segment->data, segment->len);
                ptr += segment->len;
            }

            if (segment->xmit >= kcp->dead_link) {
                //超过最大重传次数,认为连接已断
                //state并未被使用到,仅仅发生lost后调整拥塞窗口为1
                kcp->state = -1;
            }
        }
    }

    //发送缓存区内剩余的数据包
    int size = (int)(ptr - buffer);
    if (size > 0) {
        ikcp_output(kcp, buffer, size);
#ifdef KCP_EXTENTION
        kcp->snd_cnt++;
#endif // KCP_EXTENTION
    }

    if (change) {
        // 如发生了快速重传(ack跳过),进行拥塞控制
        IUINT32 inflight = kcp->snd_nxt - kcp->snd_una;//计算当前发送窗口
        kcp->ssthresh = inflight / 2;      // 调整拥塞窗口阈值为当前发送窗口的一半
        if (kcp->ssthresh < IKCP_THRESH_MIN)
            kcp->ssthresh = IKCP_THRESH_MIN;
        kcp->cwnd = kcp->ssthresh + resent;// 调整拥塞窗口大小
        kcp->incr = kcp->cwnd * kcp->mss;  // 重置最大可发送数据量
    }

    if (lost) {
        // 发生了超时重传,原因是有包丢失了,并且该包之后的包也没有收到,这很有可能是网络死了,
        // 这时候,拥塞窗口直接变为1.
        kcp->ssthresh = cwnd / 2;
        if (kcp->ssthresh < IKCP_THRESH_MIN)
            kcp->ssthresh = IKCP_THRESH_MIN;
        kcp->cwnd = 1;
        kcp->incr = kcp->mss;
    }

    if (kcp->cwnd < 1) {
        kcp->cwnd = 1;
        kcp->incr = kcp->mss;
    }
};

/**
 @brief 上层调用驱动kcp状态,设置下一次flush刷新时间戳,并调用ikcp_flush函数,每10ms-100ms调用一次
 @param current 当前时间戳ms
 @note
 使用ikcp_check获取下一次调用时间，减少ikcp_update调用次数
 可以在ikcp_input/_send后立即调用或者在一个interval后立即调用
 每次驱动的时间间隔由interval来决定,
 interval可以通过函数ikcp_interval来设置,间隔时间在10毫秒到5秒之间,
 初始默认值为100毫秒.
*/
void ikcp_update(ikcpcb *kcp, IUINT32 current)
{
    using namespace detail;
    kcp->current = current;

    if (kcp->updated == 0) {
        kcp->updated = 1;
        kcp->ts_flush = kcp->current; // 设置立马刷新
    }

    IINT32 slap = _itimediff(kcp->current, kcp->ts_flush);
    if (slap >= 10000 || slap < -10000) {
        kcp->ts_flush = kcp->current;//超过刷新时间10sec或下一次刷新间隔太长,设置立马刷新
        slap = 0;
    }

    if (slap >= 0) {
        kcp->ts_flush += kcp->interval; // 设置下一次flush刷新时间戳
        if (_itimediff(kcp->current, kcp->ts_flush) >= 0) {
            kcp->ts_flush = kcp->current + kcp->interval;
        }
        ikcp_flush(kcp);
        }
    };

/** @brief 获取接收队列下一个包的大小 */
int ikcp_peeksize(const ikcpcb *kcp)
{
    using namespace detail;
    ikcp_seg *seg = nullptr;
    int length = 0;

    assert(kcp);

    if (iqueue_is_empty(&kcp->rcv_queue))
        return -1;

    seg = iqueue_entry(kcp->rcv_queue.next, ikcp_seg, node);
    if (seg->frg == 0) return seg->len;

    if (kcp->nrcv_que < seg->frg + 1)
        return -1;// 包的分片还未完全接收到

    //计算分片总长即包大小
    iqueue_foreach_entry(seg, &kcp->rcv_queue, ikcp_seg, node) {
        length += seg->len;
        if (seg->frg == 0) break;
    }

    return length;
};

/** @brief 获取待发送包的数量 */
inline int ikcp_waitsnd(const ikcpcb *kcp)
{
    return kcp->nsnd_buf + kcp->nsnd_que;
};

//---------------KCP下层输入输出-------------------

/**
 @brief 用户接收的来自远端的底层网络数据(udp报文)通过input传入kcp
 然后把底层网络数据解码成kcp报文进行缓存.
 @note
 对于用户传入的数据,kcp会先对数据头部进行解包,判断数据包的大小、会话序号等信息,
 同时更新远端窗口大小.通过调用 parse_una 来确认远端收到的数据包,
 将远端接收到的数据包从 snd_buf 中移除.然后调用shrink_buf来更新kcp中snd_una信息,
 用于告诉远端自己已经确认被接收的数据包信息.

 之后根据不同的数据包cmd类型分别处理对应的数据包 :
 IKCP_CMD_ACK  : 对应ACK包,
     kcp通过判断当前接收到ACK的时间戳和ACK包内存储的发送时间戳来更新rtt和rto的时间.
 IKCP_CMD_PUSH : 对应数据包,kcp首先会判断sn号是否超出了当前窗口所能接收的范围,
     如果超出范围将直接丢弃这个数据包,
     如果是已经确认接收过的重复包也直接丢弃,然后将数据转移到新的Segment中,
     通过 parse_data 将Segment放入rcv_buf中,
     在 parse_data 中首先会在rcv_buf中遍历一次,判断是否已经接收过这个数据包,
     如果数据包不存在则添加到rcv_buf中,之后将可用的Segment再转移到rcv_queue中.
 IKCP_CMD_WASK : 对应远端的窗口探测包,设置probe标志,在之后发送本地窗口大小.
 IKCP_CMD_WINS : 对应远端的窗口更新包,无需做额外的操作.

 然后根据接收到的ACK遍历 snd_buf 队列更新各个Segment中ACK跳过的次数,用于之后判断是否需要快速重传.
 最后进行窗口慢启动与拥塞控制
 @code
 |<------------ 4 bytes ------------>|
 +--------+--------+--------+--------+
 |  conv                             | conv: 会话序号
 +--------+--------+--------+--------+ cmd:  指令类型
 |  cmd   |  frg   |			   wnd  | frg:  分片序号
 +--------+--------+--------+--------+ wnd:  接收窗口大小
 |							   ts	| ts:   发送的时间戳
 +--------+--------+--------+--------+
 |							   sn	| sn:   包序号
 +--------+--------+--------+--------+
 |							   una	| una:  当前未收到的序号
 +--------+--------+--------+--------+
 |							   len  | len:  数据段长度
 +--------+--------+--------+--------+
 @endcode
*/
int ikcp_input(ikcpcb *kcp, const char *data, long size)
{
    using namespace detail;
    assert(kcp);
    if (data == NULL || (int)size < (int)IKCP_OVERHEAD)
        return -1;

    IUINT32 una = kcp->snd_una; // 缓存一下当前的 snd_una
    IUINT32 maxack = 0;
    int is_first_maxack = 0;

    ikcp_log(kcp, IKCP_LOG_INPUT, "[kcp] input %d bytes", size);

    // 逐步解析data中的数据
    while (1) {
        IUINT32 conv, ts, sn, len;
        IUINT16 wnd;
        IUINT8 cmd, frg;
        ikcp_seg *seg;

        // 解析数据中的KCP头部
        if (size < (int)IKCP_OVERHEAD) break;

        data = ikcp_decode32u(data, &conv);
        if (conv != kcp->conv) return -1;

        data = ikcp_decode8u(data, &cmd);
        data = ikcp_decode8u(data, &frg);
        data = ikcp_decode16u(data, &wnd);
        data = ikcp_decode32u(data, &ts);
        data = ikcp_decode32u(data, &sn);
        data = ikcp_decode32u(data, &una);
        data = ikcp_decode32u(data, &len);

        // kcp包头一共24个字节, size减去IKCP_OVERHEAD即24个字节应该不小于len
        size -= IKCP_OVERHEAD;
        if ((long)size < (long)len) return -2;

        if (cmd != IKCP_CMD_PUSH && cmd != IKCP_CMD_ACK &&
            cmd != IKCP_CMD_WASK && cmd != IKCP_CMD_WINS)
            return -3;

        // 获得远端的窗口大小
        kcp->rmt_wnd = wnd;

        // 分析una,删除snd_buf中小于una的segment(都被远端收到了)
        ikcp_parse_una(kcp, una);

        // 更新本地 snd_una 数据
        ikcp_shrink_buf(kcp);

        if (cmd == IKCP_CMD_ACK) {
            // 如果收到的是远端发来的ACK包
            if (_itimediff(kcp->current, ts) >= 0) {
                // 根据RTT更新ACK
                ikcp_update_ack(kcp, _itimediff(kcp->current, ts));
            }

            // 分析sn,在snd_buf中找到第一个包序号为sn的ikcp_seg删除之
            ikcp_parse_ack(kcp, sn);

            // 因为snd_buf可能改变了,更新当前的 snd_una
            ikcp_shrink_buf(kcp);

            // 记录最大的ACK包的sn值
            if (is_first_maxack == 0) {
                is_first_maxack = 1;
                maxack = sn;
            }
            else {
                if (sn > maxack) {
                    maxack = sn;
                }
            }

            // 记录sn, rtt, rto
            ikcp_log(kcp, IKCP_LOG_IN_ACK,
                "[kcp] input ack: [sn]=%lu [rtt]=%lu [rto]=%lu", sn,
                (long)_itimediff(kcp->current, ts),
                (long)kcp->rx_rto);
        }
        else if (cmd == IKCP_CMD_PUSH)
        {
            // 如果收到的是远端发来的数据包
            ikcp_log(kcp, IKCP_LOG_IN_DATA, "[kcp] input psh: [sn]=%lu [len]=%lu [ts]=%lu", sn, len, ts);

            // 如果还有足够多的接收窗口
            if (sn < kcp->rcv_nxt + kcp->rcv_wnd) {
                // 对该报文的确认 ACK 报文放入 ACK 列表acklist中
                // push当前包的ACK给远端(会在flush中发送ACK出去)
                ikcp_ack_push(kcp, sn, ts);

                if (sn >= kcp->rcv_nxt) {
                    seg = ikcp_segment_new(kcp, len);
                    seg->conv = conv;
                    seg->cmd = cmd;
                    seg->frg = frg;
                    seg->wnd = wnd;
                    seg->ts = ts;
                    seg->sn = sn;
                    seg->una = una;
                    seg->len = len;

                    if (len > 0) {
                        memcpy(seg->data, data, len);
                    }

                    // 解析data,
                    // 在rcv_buf中遍历一次, 判断是否已经接收过这个数据包,
                    // 如果数据包不存在则添加到rcv_buf中, 之后将可用的Segment转移到rcv_queue中
                    ikcp_parse_data(kcp, seg);
                }
            }
        }
        else if (cmd == IKCP_CMD_WASK) {
            // 如果收到的包是远端发过来询问窗口大小的包
            // 设置探查变量,将在ikcp_flush中发送IKCP_CMD_WINS,告知远端我的接收窗口大小
            kcp->probe |= IKCP_ASK_TELL;
            ikcp_log(kcp, IKCP_LOG_IN_PROBE, "[kcp] input probe:IKCP_ASK_TELL");
        }
        else if (cmd == IKCP_CMD_WINS) {
            // 如果收到的包是远端发过来告知窗口大小的包
            // do nothing
            ikcp_log(kcp, IKCP_LOG_IN_WINS, "[kcp] input wins: %lu", (IUINT32)(wnd));
        }
        else {
            return -3;
        }

        data += len;
        size -= len;
    }

    if (is_first_maxack != 0) {
        // 遍历snd_buf更新各个Segment中ACK跳过的次数,
        // 若Segment的sn小于接收到的ACK包的sn, 则Segment的fastack++,
        // 若fastack超过指定阈值, 则启动快速重传.
        ikcp_parse_fastack(kcp, maxack);
    }

    // snd_una与之前缓存的 una 比较
    // 若 snd_una>una,说明收到了有效的una或ACK之后已经更新了snd_una了,
    // 接下来要做流量控制和拥塞控制
    if (kcp->snd_una > una) {
        if (kcp->cwnd < kcp->rmt_wnd) {
            IUINT32 mss = kcp->mss;
            if (kcp->cwnd < kcp->ssthresh) { // 慢启动阶段
                kcp->cwnd++;
                kcp->incr += mss;
            }
            else { // 拥塞控制阶段
                if (kcp->incr < mss)
                    kcp->incr = mss;
                kcp->incr += (mss * mss) / kcp->incr + (mss / 16);
                if ((kcp->cwnd + 1) * mss <= kcp->incr)
                    kcp->cwnd++;
            }
            if (kcp->cwnd > kcp->rmt_wnd) {
                kcp->cwnd = kcp->rmt_wnd;
                kcp->incr = kcp->rmt_wnd * mss;
            }
        }
    }
    return 0;
};

/** @brief 设置kcp输出回调 */
inline void ikcp_setoutput(ikcpcb *kcp, int(*output)(const char *buf, int len,
    ikcpcb *kcp, void *user))
{
    assert(kcp);
    kcp->output = output;
};

//---------------KCP上层输入输出-------------------

/**
 @brief 上层调用接收数据(去除kcp头的用户数据)
 @param[in,out] buffer 用户接收数据区
 @param[in] len 准备接收的数据大小<缓冲区大小
 @note
    user/upper level recv: returns size, returns below zero for EAGAIN
    kcp_recv函数,用户获取接收到数据(去除kcp头的用户数据).
    该函数根据frg,把kcp包数据进行组合返回给用户.

    上层调用kcp的receive函数,
    会将rcv_queue中的数据分段整理好填入用户数据区(即 ikcp_recv 函数中的形参char *buffer)中,
    然后删除对应的Segment,在做数据转移前会先计算一遍本次数据包的总大小,
    只有大小合适时用户才会收到数据.

    然后在接收缓冲区中寻找下一个需要接收的Segment,
    如果找到则将该Segment转移到rcv_queue中等待下次用户再调用receive接收数据 .

    需要注意的是,Segment在从buf转到queue中时会确保转移的Segment的sn号为下次需要接收的,
    否则将不做转移,rcv_queue 的数据是连续的,rcv_buf 可能是间隔的

    之后根据用户接收数据后的窗口变化来告诉远端进行窗口恢复.
*/
int ikcp_recv(ikcpcb *kcp, char *buffer, int len)
{
    using namespace detail;
    assert(kcp);
    assert(buffer);

    int ispeek = (len < 0) ? 1 : 0;
    int peeksize;
    int recover = 0;
    ikcp_seg *seg = nullptr;

    if (iqueue_is_empty(&kcp->rcv_queue))
        return -1;

    if (len < 0) len = -len;

    peeksize = ikcp_peeksize(kcp);
    if (peeksize < 0)
        return -2;
    if (peeksize > len)
        return -3;

    /*
    首先检测一下本次接收数据之后,是否需要进行窗口恢复.
    在前面的内容中解释过,KCP 协议在远端窗口为0的时候将会停止发送数据,
    此时如果远端调用 ikcp_recv 将数据从 rcv_queue 中移动到应用层 buffer 中之后,
    表明其可以再次接受数据,为了能够恢复数据的发送,
    远端可以主动发送 IKCP_ASK_TELL 来告知窗口大小
    */
    if (kcp->nrcv_que >= kcp->rcv_wnd)
        recover = 1; // 当前可用接收窗口为0 标记需要进行窗口恢复

    /*
    merge fragment
    按序拷贝rcv_queue数据到用户buffer,当碰到某个 segment 的 frg 为 0 时跳出循环,表明本次数据接收结束.
    frg 为 0 的数据包标记着完整接收到了一次 send 发送过来的数据;
    */
    iqueue_head *p = nullptr;
    for (len = 0, p = kcp->rcv_queue.next; p != &kcp->rcv_queue; ) {
        seg = iqueue_entry(p, ikcp_seg, node);
        p = p->next;//由于!ispeek seg会被删除,需提前缓存

        if (buffer) {
            memcpy(buffer, seg->data, seg->len);
            buffer += seg->len;
        }

        len += seg->len;

        ikcp_log(kcp, IKCP_LOG_RECV, "[kcp] recv [sn]=%lu [frg]=%lu [len]=%lu", seg->sn, seg->frg, seg->len);

        if (!ispeek) { // 入参len>=0则peek
            iqueue_del(&seg->node);
            ikcp_segment_delete(kcp, seg);
            kcp->nrcv_que--;
        }

        if (seg->frg == 0)
            break;
    }
    assert(len == peeksize);

    /*
    将 rcv_buf 中的数据转移到 rcv_queue 中,
    这个过程根据报文的 sn 编号来确保转移到 rcv_queue 中的数据一定是按序的
    */
    iqueue_foreach_entry(seg, &kcp->rcv_buf, ikcp_seg, node) {
        if (seg->sn == kcp->rcv_nxt && kcp->nrcv_que < kcp->rcv_wnd) {
            iqueue_del(&seg->node);
            kcp->nrcv_buf--;
            iqueue_add_tail(&seg->node, &kcp->rcv_queue);
            kcp->nrcv_que++;
            kcp->rcv_nxt++;
        }
        else {
            break;
        }
    }

    /*
    fast recover
    此时如果 recover 标记为1,表明在此次接收之前,可用接收窗口为0,
    如果经过本次接收之后,可用窗口大于0,需要进行窗口恢复:
    主动发送 IKCP_ASK_TELL 数据包来通知对方已可以接收数据
    */
    if (kcp->nrcv_que < kcp->rcv_wnd && recover) {
        //准备在ikcp_flush中发送IKCP_CMD_WINS 以告知远端本地接收窗口大小
        kcp->probe |= IKCP_ASK_TELL;
    }

    return len;
};

/**
 @brief 上层调用发送数据
 @note
    user/upper level send, returns below zero for error

    该函数的功能非常简单,把用户发送的数据根据MSS进行分片.
    用户发送1900字节的数据,MTU为1400byte.
    因此,该函数会把1900byte的用户数据分成两个包,一个数据大小为1400,头frg设置为1,
    len设置为1400;第二个包,头frg设置为0,len设置为500.
    切好KCP包之后,放入到名为snd_queue的待发送队列中.
    注:
    包模式下,用户数据%MSS的包,也会作为一个包发送出去.
    流模式情况下,kcp会把两次发送的数据衔接为一个完整的kcp包.

    当设置好输出函数之后,上层应用可以调用 ikcp_send 来发送数据.
    ikcpcb 中定义了发送相关的缓冲队列和 buf,分别是 snd_queue 和 snd_buf.
    应用层调用 ikcp_send 后,数据将会进入到 snd_queue 中,
    而下层函数 ikcp_flush 将会决定将多少数据从 snd_queue 中移到 snd_buf 中进行发送.

    我们首先来看 ikcp_send 的主要功能 :
    kcp发送的数据包分为2种模式,包模式和流模式.
    以mss为依据对用户数据分segment (即分片过程fragment) :
    包模式,数据分片赋予独立id,依次放入snd_queue,接收方按照id合并分片数据,分片大小 <= mss
    流模式,检测上一个分片是否达到mss,如未达到则填充,利用率高一些
*/
int ikcp_send(ikcpcb *kcp, const char *buffer, int len)
{
    using namespace detail;
    assert(kcp->mss > 0);
    assert(len > 0);
    if (len <= 0) return -1;

    ikcp_seg *seg;

    /*
    如果当前的 KCP 开启流模式,取出 snd_queue 的最后一个报文(即 kcp->snd_queue.prev)
    将其填充到 mss 的长度,并设置其 frg 为 0.
    */
    if (kcp->stream != 0) {
        if (!iqueue_is_empty(&kcp->snd_queue)) {
            ikcp_seg *old = iqueue_entry(kcp->snd_queue.prev, ikcp_seg, node);
            if (old->len < kcp->mss) {
                int capacity = kcp->mss - old->len; //分片剩余容量
                int extend = (len < capacity) ? len : capacity;// 分片待填充大小
                seg = ikcp_segment_new(kcp, old->len + extend);
                assert(seg);
                if (seg == NULL) {
                    return -2;
                }
                iqueue_add_tail(&seg->node, &kcp->snd_queue);
                memcpy(seg->data, old->data, old->len);
                if (buffer) {
                    memcpy(seg->data + old->len, buffer, extend);
                    buffer += extend;
                }
                seg->len = old->len + extend;
                seg->frg = 0;
                len -= extend;
                iqueue_del_init(&old->node);
                ikcp_segment_delete(kcp, old);
            }
        }
        if (len <= 0) {
            return 0;// 数据完全填充
        }
    }

    // 计算剩下的数据需要分成几段
    int count = len <= (int)kcp->mss ? 1 : (len + kcp->mss - 1) / kcp->mss;
    if ((IUINT32)count >= IKCP_WND_RCV) return -2;

    // 为剩下的数据创建 KCP segment并追加到发送队列
    for (int i = 0; i < count; i++) {
        int size = len > (int)kcp->mss ? (int)kcp->mss : len;
        seg = ikcp_segment_new(kcp, size);
        assert(seg);
        if (seg == NULL) {
            return -2;
        }
        if (buffer && size > 0) {
            memcpy(seg->data, buffer, size);
            buffer += size;
        }
        seg->len = size;
        // frg用来表示被分片的序号,倒序; 流模式下分片编号不用填
        seg->frg = (kcp->stream == 0) ? (count - i - 1) : 0;
        iqueue_init(&seg->node);
        iqueue_add_tail(&seg->node, &kcp->snd_queue);
        kcp->nsnd_que++;
        len -= size;
    }

    return 0;
};

//---------------RDC-------------------

/**
 @def KCP_EXTENTION
 @brief 可靠性数据支持或丢包率数据统计等扩展原始KCP的部分使用此宏包围
*/
#ifdef KCP_EXTENTION
/**
 @brief 检查RDC状态,应用层调用此方法检查丢包率和时延是否超限
 @return 丢包率和时延超限时返回1,否则返回0
*/
int ikcp_rdc_check(ikcpcb *kcp)
{
    using namespace detail;
    IINT32 slap = _itimediff(kcp->current, kcp->rdc_check_ts);
    if (slap < 0 && slap > -10000)
        return kcp->is_rdc_on;//未到检查点

    kcp->rdc_check_ts = kcp->current + kcp->rdc_check_interval;

    if (kcp->snd_cnt > 0)
        kcp->loss_rate = (int)(1.0 * kcp->timeout_resnd_cnt / kcp->snd_cnt * 100);//计算丢包率的值(超时重传次数/数据包发送数)
    ikcp_log(kcp, IKCP_LOG_OUT_DATA, "[kcp] resend loss rate:%d%%", kcp->loss_rate);
    ikcp_log(kcp, IKCP_LOG_OUT_DATA, "[kcp] resend srtt:%d", kcp->rx_srtt);

    kcp->timeout_resnd_cnt = 0;
    kcp->snd_cnt = 0;

    if (!kcp->is_rdc_on
        && kcp->loss_rate >= kcp->rdc_loss_rate_limit //5%
        && kcp->rx_srtt >= kcp->rdc_rtt_limit)//111
        kcp->is_rdc_on = 1;//丢包率或平均时延超限
    else if (kcp->is_rdc_on
        && (kcp->loss_rate < kcp->rdc_loss_rate_limit || kcp->rx_srtt < kcp->rdc_rtt_limit)//未超限
        /*&& (++kcp->rdc_close_try_times >= kcp->rdc_close_try_threshold)*/)
    {//尝试关闭RDC次数大于阈值,说明网络波动但保持连接,可以阶段性关闭RDC状态了
        kcp->is_rdc_on = 0;
        //kcp->rdc_close_try_times = 0;
    }
    return kcp->is_rdc_on;
};
#endif // KCP_EXTENTION

#endif // __IKCP_H__