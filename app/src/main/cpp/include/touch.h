#ifndef TOUCH__h
#define TOUCH__h

#ifdef __cplusplus
extern "C"
{
#endif


#define TEST 1


#define DEFAULT_MAX_BODY_LEN 20480          /*单个任务最大报文体长度*/

#define DEFAULT_MIN_THR_NUM     3         /*最小发送任务线程数*/
#define DEFAULT_MAX_THR_NUM     10        /*最大发送任务线程数*/
#define DEFAULT_QUEUE_MAX_SIZE  100       /*每个队列最大任务数*/
#define DEFAULT_PACK_TASK_NUM 10          /*每次发送打包任务个数*/
#define DEFAULT_ADD_STEP_NUM  50          /*每次发送打包任务个数*/
#define DEFAULT_SEND_INTERVAL 10          /*10秒*/


#define MIN_WAIT_TASK_NUM 10            /*如果queue_size > MIN_WAIT_TASK_NUM 添加新的线程到线程池*/
#define DEFAULT_THREAD_VARY 10          /*每次创建和销毁线程的个数*/

#define true 1
#define false 0

typedef struct
{
    int  msg_type;                /**消息类型 1-track, 2-login**/
    int  is_login_id;          /**false时distinct_id代表的是匿名标示，true时distinct_id代表的是登录账号**/
    char distinct_id[32];         /*用户标示*/
    long datetime;            /**SDK采集时间戳 必有 "2020-03-20 12:23:45.547" **/
    char event_code[32];          /**事件code **/
    char original_id[32];        /**匿名用户标示**/
    char properties[DEFAULT_MAX_BODY_LEN];        /**properties:json串**/
} TASK;


typedef struct Node
{
    TASK * data;
    struct LinkQueueNode *next;
}LinkQueueNode;//队列的结点结构

typedef struct
{
    LinkQueueNode *front;
    LinkQueueNode *rear;
}LinkQueue;//队列的链表结构

typedef struct
{
    pthread_mutex_t lock;               /*用于锁住当前这个结构体体taskpoll*/
    pthread_mutex_t thread_counter;     /*记录忙状态线程个数*/
    pthread_cond_t queue_not_full;      /*当任务队列满时，添加任务的线程阻塞，等待此条件变量*/
    pthread_cond_t queue_not_empty;     /*任务队列里不为空时，通知等待任务的线程*/

    pthread_t *threads;                 /*保存工作线程tid的数组*/
    pthread_t adjust_tid;               /*管理线程tid*/

    LinkQueue receiveDeque;               /*任务队列:接收到埋点数据队列*/

    int min_thr_num;                    /*线程组内默认最小线程数*/
    int max_thr_num;                    /*线程组内默认最大线程数*/
    int live_thr_num;                   /*当前存活线程个数*/
    int busy_thr_num;                   /*忙状态线程个数*/
    int wait_exit_thr_num;              /*要销毁的线程个数*/
    int queue_front;                    /*队头索引下标*/
    int queue_rear;                     /*队未索引下标*/
    int queue_size;                     /*队中元素个数*/
    int queue_max_size;                 /*队列中最大容纳个数*/
    int shutdown;                       /*线程池使用状态，true或false*/

    /*自定义*/
    int step_num;                       /*当前任务发送步长*/
    int pack_task_num;                  /*每包最大任务数*/
    int add_step_num;                   /*任务发送失败,每次增加步长*/
    int send_interval;                  /*任务循环时间间隔*/

    /*
    int flush_num = 100;
    int enable = 1;//是否采集埋点数据 默认true
    char sdk_version[12] = "1.0.1";
    */


    char app_url[128];
    char app_key[64];
    char app_secret[64];
    char app_enable[8];
    char public_app_name[64];            /*项目应用名称*/
    char public_app_version[12];         /*项目应用版本*/

    char superProperties[10240];     /*公共属性*/

} TOUCH;

/**
     * 创建埋点信息处理线程池
     *
     * @param min_thr_num   线程池最小1个发送线程（目前只支持赋值为1）
     * @param max_thr_num   线程池最多1个发送线程（目前只支持赋值为1）
     * @param queue_max_size  队列可容纳最大值任务数
     * @param pack_task_num 每包的最大任务数
     * @param add_step_num  发送失败后失败步长
     * @param send_interval 任务轮询间隔
     */

TOUCH *touch_create(int min_thr_num, int max_thr_num, int queue_max_size, int pack_task_num, int add_step_num, int send_interval);


/**
* 设置所有相关APP属性
*
* @param superPropertiesMap
*/
int touch_init(TOUCH *touch, char *app_url, char *app_key, char *app_secret, char *app_enable, char *public_app_name, char *public_app_version);

/**
* 设置所有埋点数据中的公共属性
*
* @param superPropertiesMap
*/
int touch_registerSuperProperties(TOUCH *touch, char *superPropertiesMap );

/**
* 事件埋点信息添加到内存队列中
*
* @param distinctId 设备Id
* @param isLoginId  是否为登录ID
* @param eventCode  事件code
* @param properties 事件属性
*/
int touch_track(TOUCH *touch, char *distinctId, int isLoginId, char *eventCode, char *properties);

/**
* 登录id和匿名id绑定(记录用户注册事件)
* 将用户绑定埋点信息添加到内存队列中
*
* @param loginId     登录ID
* @param anonymousId 匿名ID
*/
int touch_trackSignUp(TOUCH *touch, char *loginId, char * anonymousId, char *properties);





#ifdef __cplusplus
}
#endif

#endif
