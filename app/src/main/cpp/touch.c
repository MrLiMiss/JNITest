#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <iconv.h>
#include <curl/curl.h>
#include <openssl/pem.h>
#include <openssl/md5.h>
#include <sys/time.h>
#include "cJSON.h"
#include "touch.h"

TOUCH * g_touch=NULL;

int g_touch_print_flag=0;  /*1-打印,  0-不打印日志*/

/*日志打印*/
int print_log(char *file_name, int line, char *_fmt, ... )
{
    char time_buf[32];
    char       msg[DEFAULT_MAX_BODY_LEN];
    va_list    ap;
    struct tm *newtime;
    time_t t; 

    if(g_touch_print_flag==0)
    {
        return 0;
    }

    memset(msg, 0x00, sizeof(msg));

    t = time(NULL);
    newtime = localtime(&t);


    va_start( ap, _fmt );
    vsnprintf( msg, sizeof(msg), _fmt, ap );
    va_end( ap );

    time_buf[strftime(time_buf, sizeof(time_buf), "%H:%M:%S", newtime)] = '\0';

    printf("FILE:[%s]LINE[%d]TIME[%s][%s]\n", file_name, line, time_buf, msg );

    return 0;
}



//初始化队列
int init_link_queue(LinkQueue *LQ)
{
	//创建头结点
	LinkQueueNode *pHead = (LinkQueueNode *)malloc(sizeof(LinkQueueNode));
	if (pHead == NULL)
	{
		print_log(_FL_, "init_link_queue fail:malloc error\n");
		return -1;
	}
	LQ->front = LQ->rear = pHead;//队头和队尾指向头结点 ps:重要，后面的操作都是基于此
	LQ->front->next = NULL;
    return 0;
}

//判断队列是否为空
int is_empty_queue(LinkQueue *LQ)
{
	if (LQ->front == LQ->rear)//队头指针与队尾指针相等，队列为空
	{
		return 1;//队列为空
	}
	return 0;
}

//新元素入队
void enter_link_queue(LinkQueue *LQ, TASK * data)
{
	LinkQueueNode *pNewNode = (LinkQueueNode *)malloc(sizeof(LinkQueueNode));
	if (pNewNode == NULL)
	{
		print_log(_FL_, "enter_link_queue fail:malloc error\n");
		return;
	}
	//链式队列的结点是动态开辟的，没有判断队列是否为满的操作
	//建立结点
	pNewNode->data = data;//将数据元素赋值给结点的数据域
	pNewNode->next = NULL;//将结点的指针域置空
	//实现插入
	LQ->rear->next = pNewNode;//新结点接到上一个结点尾部
	LQ->rear = pNewNode;//队尾指针重新指向，新任队尾
}

//新元素入队
void re_enter_link_queue(LinkQueue *LQ, TASK * data)
{
	LinkQueueNode *pNewNode = (LinkQueueNode *)malloc(sizeof(LinkQueueNode));
	if (pNewNode == NULL)
	{
		print_log(_FL_, "enter_link_queue fail:malloc error\n");
		return;
	}
	//链式队列的结点是动态开辟的，没有判断队列是否为满的操作
	//建立结点
	pNewNode->data = data;//将数据元素赋值给结点的数据域
	pNewNode->next = LQ->front->next;//将结点的指针域置空
	//实现插入
	LQ->front->next = pNewNode;//队头指针重新指向，新任队头

	if (LQ->rear->next == pNewNode)//如果队列中只有一个元素,将队尾指针指向头结点，队列为空
	{
		LQ->rear = pNewNode;
	}
	
}


//元素出队
void delete_link_queue(LinkQueue *LQ, TASK * *data)
{
	if (is_empty_queue(LQ))
	{
		print_log(_FL_, "队列为空，出队失败\n");
		return;
	}
	//pDel指向队头元素，由于队头指针front指向头结点，所以pDel指向头结点的下一个结点
	LinkQueueNode *pDel = LQ->front->next;
	*data = pDel->data;//将要出队的元素赋给data
	LQ->front->next = pDel->next;//使指向头结点的指针指向pDel的下一个结点
	if (LQ->rear == pDel )//如果队列中只有一个元素,将队尾指针指向头结点，队列为空
	{
		LQ->rear = LQ->front;
	}
	free(pDel);//释放pDel指向的空间
}

//获取队头元素
int get_head_data(LinkQueue *LQ, TASK * *data)
{
	if (is_empty_queue(LQ))
	{
		print_log(_FL_, "队列为空\n");
		return 0;
	}
	LinkQueueNode *pCur = LQ->front->next;//pCur指向队列的第一个元素，即头结点的下一个结点
	*data = pCur->data;
	return *data;
}

//获取队列长度
int get_link_queue_length(LinkQueue *LQ)
{
	int length = 0;
	LinkQueueNode *pCur = LQ->front->next;
	while (pCur != NULL)
	{
		length++;
		pCur = pCur->next;
	}
	return length;
}

//打印队列元素
void print_link_queue(LinkQueue *LQ)
{
	LinkQueueNode *pCur;
	pCur = LQ->front->next;
	while (pCur != NULL)
	{
		print_log(_FL_, "%d ", pCur->data);
		pCur = pCur->next;
	}
	print_log(_FL_, "\n");
}

//清空队列
void clear_link_queue(LinkQueue *LQ)
{
	while (LQ->front != NULL)
	{
		LQ->rear = LQ->front->next;//当删除队列时，队尾指针就没用处了，此处采用队尾指针来保存队头指针的下一个结点
		free(LQ->front);//全部释放，包括头结点
		LQ->front = LQ->rear;
	}
}

int get_task_tm(long *send_time)
{
    struct timeval  ltimeb;
    long ltime;

    gettimeofday(&ltimeb, NULL);
    ltime=ltimeb.tv_sec*1000 + ltimeb.tv_usec / 1000;
    *send_time=ltime;

    return 0;
}



int md5_encode(char *in_str, int in_len, char *out_str)
{
        MD5_CTX ctx;
        unsigned char md5_16[16];
        char md5_32[32+1];
        int i=0;

        memset(md5_32, 0x00, sizeof(md5_32));

        MD5_Init(&ctx);
        MD5_Update(&ctx, in_str, in_len);
        MD5_Final(md5_16, &ctx);

        for (i=0; i<16; i++)
        {
            sprintf(&md5_32[i*2], "%02x", md5_16[i]);
        }

		print_log(_FL_, "md5[%s]\n", md5_32);
        strcpy(out_str, md5_32);

        return 0;
}

int base64_encode(char *in_str, int in_len, char *out_str, int * out_len)
{

    BIO *b64, *bio;

    BUF_MEM *bptr = NULL;

    if (in_str == NULL || out_str == NULL)
        return -1;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);

    BIO_write(bio, in_str, in_len);
    BIO_flush(bio);

    BIO_get_mem_ptr(bio, &bptr);

    memcpy(out_str, bptr->data, bptr->length);
    out_str[bptr->length] = '\0';
    *out_len= bptr->length;

    BIO_free_all(bio);

    return 0;

}

int base64_decode(char *in_str, int in_len, char *out_str, int *out_len)
{

    BIO *b64, *bio;
    BUF_MEM *bptr = NULL;
    int counts;

    if (in_str == NULL || out_str == NULL)
        return -1;

    b64 = BIO_new(BIO_f_base64());
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);

    bio = BIO_new_mem_buf(in_str, in_len);
    bio = BIO_push(b64, bio);
    *out_len= BIO_read(bio, out_str, in_len);
    out_str[(*out_len)] = '\0';

    BIO_free_all(bio);

    return 0;
}


int thread_json_data(TOUCH * pool, TASK * *task_list, int task_num, void *out, int *out_len )
{
    int i=0;
	cJSON *array=NULL;
    cJSON *root=NULL;
    cJSON *item=NULL;
	cJSON *superProperties=NULL;
    cJSON *sum_properties=NULL;
    char * properties=NULL;
	char * js_str=NULL;
    TASK *p_maidian_data=NULL;

    TOUCH * touch=(TOUCH *)pool;

	print_log(_FL_, "touch->superProperties:[%s]task_num[%d]\n",touch->superProperties, task_num);

	
	array=cJSON_CreateArray();
    for(i=0; i<task_num; i++)
    {
        p_maidian_data=task_list[i];
	    print_log(_FL_, "task->properties:[%s]\n",p_maidian_data->properties);
	    superProperties=NULL;
        superProperties= cJSON_Parse(touch->superProperties);


        root=cJSON_CreateObject();
        if(p_maidian_data ->msg_type == 1)
        {
            if(p_maidian_data->is_login_id== 1)
            {
                cJSON_AddBoolToObject(root, "is_login_id", 1);
            }
            else
            {
                cJSON_AddBoolToObject(root, "is_login_id", 0);
            }

            cJSON_AddStringToObject(root, "distinct_id", p_maidian_data->distinct_id);
            cJSON_AddNumberToObject(root, "_datetime", p_maidian_data->datetime);
            cJSON_AddStringToObject(root, "type", "track");
            cJSON_AddStringToObject(root, "event_code", p_maidian_data->event_code);
            item = cJSON_Parse(p_maidian_data->properties);

            sum_properties=cJSON_Print_2Object(item, superProperties);

            cJSON_AddItemToObject(root, "properties", sum_properties);

        }
        else if(p_maidian_data ->msg_type == 2)
        {

            cJSON_AddStringToObject(root, "distinct_id", p_maidian_data->distinct_id);
            cJSON_AddStringToObject(root, "original_id", p_maidian_data->original_id);
            cJSON_AddNumberToObject(root, "_datetime", p_maidian_data->datetime);
            cJSON_AddStringToObject(root, "type", "_signUp");
            cJSON_AddStringToObject(root, "event_code", "_signUp");
                
            /*properties */
            item = cJSON_Parse(p_maidian_data->properties);
            sum_properties=cJSON_Print_2Object(item, superProperties);

            cJSON_AddItemToObject(root, "properties", sum_properties);
        }
		
		cJSON_AddItemToArray(array, root);

	    cJSON_Delete(superProperties);
    }


    js_str=cJSON_Print(array);
    *out_len=strlen(js_str);

	if(*out_len > (DEFAULT_MAX_BODY_LEN*task_num / 8 * 6))
	{
		cJSON_Delete(array);
		free(js_str);
		return -2;
	}

	memcpy(out, js_str, *out_len);


	cJSON_Delete(array);
	free(js_str);


    return 0;
}

int thread_base64_data(unsigned char* in, int in_len, unsigned char *out, int *out_len )
{
	int i=0;
	int j=0;
	unsigned char *tmp=NULL;;
    base64_encode(in, in_len, out, out_len);
	tmp=out;
	

	for(j=0; j<*out_len; j++)
	{
		if(tmp[j] != '\n'){
		    out[i] = tmp[j];
			i++;
		}
	}

    out[i]='\0';
	*out_len = i;


	return 0;
	
}


int thread_get_http_rsp(void * buffer, int size, int nmemb, void * arg)
{
    int len=0;
    len=size * nmemb;
    memcpy(arg, buffer, len);
    
    return len;
}

int thread_parse_http_rsp(char * buffer, char *rsp_code, char * rsp_message, char *rsp_content)
{
    cJSON *root=NULL;
    cJSON *item=NULL;

	char * js_code=NULL;
	char * js_message=NULL;
	char * js_content=NULL;


    
    root=cJSON_Parse(buffer);
    if (!root) 
    {
        print_log(_FL_, "ERROR:thread_parse_http_rsp cJSON_Parse failed:buff:[%s]", buffer);
        return -1;
    }

    item=cJSON_GetObjectItem(root,"success") ;
    if ( NULL == item )
    {
        print_log(_FL_, "success为空，报错");
		return -1;
    }
    else
    {
		js_code=cJSON_Print(item);
        strcpy(rsp_code, js_code);
		free(js_code);
    }

    item=cJSON_GetObjectItem(root,"message") ;
    if ( NULL == item )
    {
        print_log(_FL_, "message 为空，报错");
		return -1;
    }
    else
    {
		js_message=cJSON_Print(item);
        strcpy(rsp_message, js_message);
		free(js_message);
    }

    item=cJSON_GetObjectItem(root,"content") ;
    if ( NULL == item )
    {
        print_log(_FL_, "content 为空，报错");
		return -1;
    }
    else
    {
		js_content=cJSON_Print(item);
        strcpy(rsp_content, js_content);
		free(js_content);
    }

    cJSON_Delete(root);

    return 0;
}

int thread_send_http_req(TOUCH *pool, void * in, int in_len)
{

    CURL *curl;
    CURLcode res;
    char curl_url[256];
    char token[32+1];
    long send_time;
    char t_str[128];

    char rsp_buf[10240];
    char rsp_code[64+1];
    char rsp_message[128+1];
    char rsp_content[10240];
    struct curl_slist *req_header=NULL;



    int t_len=0;
	TOUCH *touch=(TOUCH *)pool;
    

    memset(curl_url, 0x00, sizeof(curl_url));
    memset(token, 0x00, sizeof(token));
    memset(t_str, 0x00, sizeof(t_str));
    memset(rsp_buf, 0x00, sizeof(rsp_buf));
    memset(rsp_code, 0x00, sizeof(rsp_code));
    memset(rsp_message, 0x00, sizeof(rsp_message));
    memset(rsp_content, 0x00, sizeof(rsp_content));

    curl = curl_easy_init();
    if (!curl) 
    {
        print_log(_FL_, "thread 0x%x end working, curl_easy_init() failed \n", (unsigned int)pthread_self());
        return -1;
    }

    /*sendtime:毫秒*/
    get_task_tm(&send_time);


    /*token是time_appkey_appsecrt的md5*/
    sprintf(t_str, "%ld_%s_%s", send_time, touch->app_key, touch->app_secret); 
    t_len=strlen(t_str);
    md5_encode(t_str, t_len, &token);

    /*设置easy handle属性*/
	/*strcpy(curl_url, "http://130.1.10.158:8888/data/encryption?appkey=default&token=88a5753fb4519ed97e992871d31ba0c8&t=1607938338786");*/
    sprintf(curl_url, "%s/encryption?appkey=%s&token=%s&t=%ld", touch->app_url, touch->app_key, token, send_time);
    
    print_log(_FL_, "thread 0x%x curl_url[%s] \n\n", (unsigned int)pthread_self(), curl_url);


    req_header=curl_slist_append(req_header, "Accept-Encoding: gzip,deflate");
    req_header=curl_slist_append(req_header, "Content-Type:application/json;charset=UTF-8");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, req_header);
    curl_easy_setopt(curl, CURLOPT_URL, curl_url);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, in_len);

    curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");

    /* Set the expected POST data */

    curl_easy_setopt(curl,CURLOPT_POSTFIELDS, in);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION, thread_get_http_rsp);
    curl_easy_setopt(curl,CURLOPT_WRITEDATA, rsp_buf);


    curl_easy_setopt (curl,CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt (curl,CURLOPT_SSL_VERIFYHOST, 0L);
    
    curl_easy_setopt (curl,CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt (curl,CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt (curl,CURLOPT_CONNECTTIMEOUT, 10L);

    /*执行数据请求*/
    res = curl_easy_perform(curl);

    if(res !=CURLE_OK) 
    {
        print_log(_FL_, "thread 0x%x end working, curl_easy_perform() failed: %s \n", (unsigned int)pthread_self(), curl_easy_strerror(res));
		return -1;
    }

    thread_parse_http_rsp(rsp_buf, rsp_code, rsp_message, rsp_content );
    
    print_log(_FL_, "rsp_buf:[%s]\n", rsp_buf);
    print_log(_FL_, "rsp_code:[%s]\n", rsp_code);
    print_log(_FL_, "rsp_message:[%s]\n", rsp_message);
    print_log(_FL_, "rsp_content:[%s]\n", rsp_content);

    curl_easy_cleanup(curl);

	/*
	strcpy(rsp_code,"false");
	*/
    if(strncmp(rsp_code, "true", 4) !=0 )
    {
        return -1;
    }

    return 0;
}

int thread_send_task_list(TOUCH *pool, TASK ** task_list, int task_num)
{
    int ret=0;
    int json_len=0;
    int base64_len=0;
    char * json_fmt=malloc(DEFAULT_MAX_BODY_LEN*task_num / 8 * 6);
    char * base64_fmt=malloc(DEFAULT_MAX_BODY_LEN*task_num);


    ret=thread_json_data(pool, task_list, task_num, json_fmt, &json_len );
	if(ret !=0)
	{
       free(json_fmt);
       free(base64_fmt);
	   return ret;
	}

    if(g_touch_print_flag != 0)
    {
	    printf("thread 0x%x...json_len[%d]json_fmt:[%s]\n",(unsigned int)pthread_self(), json_len, json_fmt);
    }

    ret=thread_base64_data(json_fmt, json_len, base64_fmt, &base64_len );
	if(ret !=0)
	{
       print_log(_FL_, "thread_base64_data,ret[%d]\n", ret);
       free(json_fmt);
       free(base64_fmt);
	   return ret;
	}

    if(g_touch_print_flag != 0)
    {
	    printf("thread 0x%x...base64_len[%d]base64:[%s]\n",(unsigned int)pthread_self(), base64_len, base64_fmt);
    }

    free(json_fmt);
    ret= thread_send_http_req(pool, base64_fmt, base64_len);

	print_log(_FL_, "thread_send_http_req,ret[%d]\n", ret);
    
    free(base64_fmt);
    
    return ret;
}


/**
 * @function void *touch_send(void *threadpool)
 * @desc the worker thread
 * @param threadpool the pool which own the thread
 */
void touch_send(void *threadpool);
/**
 * @function void *adjust_thread(void *threadpool);
 * @desc manager thread
 * @param threadpool the threadpool
 */
void adjust_thread(TOUCH *threadpool);
void check_task_thread(TOUCH *threadpool);
/**
 * check a thread is alive
 */
int is_thread_alive(pthread_t tid);

int threadpool_free(TOUCH *pool);

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

TOUCH *touch_create(int min_thr_num, int max_thr_num, int queue_max_size, int pack_task_num, int add_step_num, int send_interval)
{
    int i;
    int ret=0;
    TOUCH *pool = NULL;

    curl_global_init(CURL_GLOBAL_ALL);

    do{
        if((pool = (TOUCH *)malloc(sizeof(TOUCH))) == NULL)
        {
            print_log(_FL_, "touch create:malloc threadpool fail");
            break;          /*跳出do while*/
        }

        pool->min_thr_num = (min_thr_num==0 ? DEFAULT_MIN_THR_NUM : min_thr_num);
        pool->max_thr_num = (max_thr_num==0 ? DEFAULT_MAX_THR_NUM : max_thr_num);
        pool->busy_thr_num = 0;
        pool->live_thr_num =(min_thr_num==0 ? DEFAULT_MIN_THR_NUM : min_thr_num);
        pool->wait_exit_thr_num=0;
        pool->queue_size = 0;
        pool->queue_max_size = (queue_max_size==0 ? DEFAULT_QUEUE_MAX_SIZE : queue_max_size);
        pool->queue_front = 0;
        pool->queue_rear = 0;
        pool->shutdown = false;

		pool->step_num=0;
        pool->pack_task_num=(pack_task_num==0 ? DEFAULT_PACK_TASK_NUM : pack_task_num);
        pool->add_step_num=(add_step_num==0 ? DEFAULT_ADD_STEP_NUM : add_step_num);
        pool->send_interval=(send_interval==0 ? DEFAULT_SEND_INTERVAL : send_interval);

        ret=init_link_queue(&(pool->receiveDeque));
        if(ret !=0)
        {
            print_log(_FL_, "touch create:init_link_queue fail");
            break;
        }

        pool->threads = (pthread_t *)malloc(sizeof(pthread_t)*max_thr_num);
        if (pool->threads == NULL)
        {
            print_log(_FL_, "touch create:malloc threads fail");
            break;
        }
        memset(pool->threads, 0, sizeof(pool->threads));
        if (pthread_mutex_init(&(pool->lock), NULL) != 0
                || pthread_mutex_init(&(pool->thread_counter), NULL) != 0
                || pthread_cond_init(&(pool->queue_not_empty), NULL) != 0
                || pthread_cond_init(&(pool->queue_not_full), NULL) != 0)
        {
            print_log(_FL_, "touch create:init the lock or cond fail");
            break;
        }

        /* 启动min_thr_num个work thread */
        for (i = 0; i < min_thr_num; i++)
        {
            pthread_create(&(pool->threads[i]), NULL, touch_send, pool);
            print_log(_FL_, "pthread create:create work thread 0x%x  success...\n", (unsigned int)pool->threads[i]);
        }

        pthread_create(&(pool->check_tid), NULL, check_task_thread, (void *)pool);
        print_log(_FL_, "pthread create:create check thread 0x%x  success...\n", (unsigned int)pool->check_tid);

        /*pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool);
        print_log(_FL_, "pthread create:create adjust thread 0x%x  success...\n", (unsigned int)pool->adjust_tid);*/

        return pool;

    }while(0);
    
    threadpool_free(pool);      /*前面代码调用失败时，释放poll存储空间*/


  return NULL;
}

int touch_readd_task(TOUCH *pool, TASK ** task_list, int task_num)
{

  TASK * item=NULL;

  int i=0;
  int j=0;

  /*添加任务到任务队列里*/

    if(pool->queue_size +task_num > pool->queue_max_size)
    {
        j=pool->queue_size+task_num-pool->queue_max_size;

        for(i=0; i<j; i++)
        {
             delete_link_queue(&(pool->receiveDeque), &item);
             free(item);
             item=NULL;
             pool->queue_size--;
        }
    }



    for(i=task_num-1; i>=0 ; i--)
    {

        re_enter_link_queue(&(pool->receiveDeque), task_list[i]);
        pool->queue_size++;

        print_log(_FL_, "------>>>>thread 0x%x.queue_size[%06d].put cyc queue,amount[%d],task_list[%p] distinct_id[%s]\n\t\t\tproperties[%s]\n",(unsigned int)pthread_self(),pool->queue_size,get_link_queue_length(&pool->receiveDeque),task_list[i], task_list[i]->distinct_id, task_list[i]->properties);


    }


    if(pool->queue_size>=pool->pack_task_num+pool->step_num)
    {
        /*任务队列不为空，唤醒等待处理任务的线程*/
	    print_log(_FL_, "thread 0x%x...touch readd task send signal-> queue_not_empty \n", (unsigned int)pthread_self());
        pthread_cond_signal(&(pool->queue_not_empty));
    }

    return 0;
}



int touch_add_task(TOUCH *pool, TASK *arg)
{

	int i=0;
	int j=0;
	int task_num=1;
    TASK *item=NULL;


	print_log(_FL_, "thread 0x%x...Enter touch add task \n", (unsigned int)pthread_self() );
    pthread_mutex_lock(&(pool->lock));

	/*
    while ((pool->queue_size >= pool->queue_max_size) && (!pool->shutdown))
    {
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    }
	*/


    if (pool->shutdown)
    {
        pthread_mutex_unlock(&(pool->lock));
    }

    /*添加任务到任务队列里*/

    if(pool->queue_size +task_num > pool->queue_max_size)
    {
        j=pool->queue_size+task_num-pool->queue_max_size;

        for(i=0; i<j; i++)
        {
             delete_link_queue(&(pool->receiveDeque), &item);
             pool->queue_size--;
             print_log(_FL_, "------>>>>thread 0x%x..delete queue, after-->queue_size[%d],amount[%d]p[%p]distinct_id:[%s]\n\t\t\tproperties[%s]\n",(unsigned int)pthread_self(),pool->queue_size, get_link_queue_length(&pool->receiveDeque),item, item->distinct_id, item->properties);
             free(item);
             item=NULL;
        }
    }

    enter_link_queue(&(pool->receiveDeque), arg);
    pool->queue_size++;

	item=NULL;
	item=pool->receiveDeque.rear->data;
    print_log(_FL_, "------>>>>thread 0x%x..put queue,queue_size[%d],amount[%d]p[%p]distinct_id:[%s]\n\t\t\tproperties[%s]\n",(unsigned int)pthread_self(),pool->queue_size, get_link_queue_length(&pool->receiveDeque),item, item->distinct_id, item->properties);

    pthread_mutex_unlock(&(pool->lock));

	print_log(_FL_, "thread 0x%x...exit touch add task\n", (unsigned int)pthread_self());
    if(pool->queue_size>=pool->pack_task_num + pool->step_num)
    {
        /*任务队列不为空，唤醒等待处理任务的线程*/
	    print_log(_FL_, "thread 0x%x...touch add task send signal-> queue_not_empty \n", (unsigned int)pthread_self());
        pthread_cond_signal(&(pool->queue_not_empty));
    }

    //pthread_mutex_unlock(&(pool->lock));



    return 0;
}

void touch_send(void *threadpool)
{
    int i=0;
	int ret=0;
	int step_num=0;
    TOUCH *pool = (TOUCH *)threadpool;
	int task_num=pool->pack_task_num;
    TASK* *task_list=NULL;

	TASK *item=NULL;


	print_log(_FL_, "Enter touch_send: thread 0x%x,queue_size[%d] \n", (unsigned int)pthread_self(), pool->queue_size);

    while(true)
    {
        /* Lock must be taken to wait on conditional variable */
        /*刚创建出线程，等待任务队列里有任务，否则阻塞等待任务队列里有任务后再唤醒接收任务*/
        pthread_mutex_lock(&(pool->lock));

        print_log(_FL_, "thread 0x%x is waitingwaiting queue_size[%d],step_num[%d],add_step_num[%d],pack_task_num[%d]\n", (unsigned int)pthread_self(), pool->queue_size, pool->step_num, pool->add_step_num, pool->pack_task_num);

        if(pool->step_num >0)
        {
            if(pool->queue_size <pool->add_step_num) /*1,队列数据小于步长*/
            {
                print_log(_FL_, "thread 0x%x is waiting1\n", (unsigned int)pthread_self());
                pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
                print_log(_FL_, "thread 0x%x is waiting11\n", (unsigned int)pthread_self());
            }
            else if(pool->queue_size<pool->pack_task_num)  /*2,队列任务数小于包任务数*/
            {
                print_log(_FL_, "thread 0x%x is waiting2\n", (unsigned int)pthread_self());
                pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
                print_log(_FL_, "thread 0x%x is waiting21\n", (unsigned int)pthread_self());
            }
            else if(pool->queue_size<pool->pack_task_num+pool->step_num)  /*3,队列任务数小于包任务数加步长*/
            {
                print_log(_FL_, "thread 0x%x is waiting3\n", (unsigned int)pthread_self());
                pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
                print_log(_FL_, "thread 0x%x is waiting31\n", (unsigned int)pthread_self());
            }
        }

        while((pool->queue_size == 0) && (!pool->shutdown))
        {
            print_log(_FL_, "thread 0x%x is waiting4\n", (unsigned int)pthread_self());

            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));

            /*清除指定数目的空闲线程，如果要结束的线程个数大于0，结束线程*/
            print_log(_FL_, "thread 0x%x is waiting41\n", (unsigned int)pthread_self());
            if (pool->wait_exit_thr_num > 0)
            {
                pool->wait_exit_thr_num--;
                /*如果线程池里线程个数大于最小值时可以结束当前线程*/
                if (pool->live_thr_num > pool->min_thr_num)
                {
                    print_log(_FL_, "thread 0x%x is exiting\n", (unsigned int)pthread_self());
                    pool->live_thr_num--;
                    pthread_mutex_unlock(&(pool->lock));
                    pthread_exit(NULL);
                }
            }

        }

        /*如果指定了true，要关闭线程池里的每个线程，自行退出处理*/
        if (pool->shutdown)
        {
            pthread_mutex_unlock(&(pool->lock));
            print_log(_FL_, "thread 0x%x shutdown\n", (unsigned int)pthread_self());
            pthread_exit(NULL);
        }
        /*从任务队列里获得任务*/
		if(pool->queue_size <pool->pack_task_num)
		{
			task_num=pool->queue_size;
		}

        task_list=(TASK *)malloc(sizeof(TASK *) * (task_num));
		if(task_list==NULL)
		{
		   print_log(_FL_, "task_list is NULL\n");
		}

        for(i=0; i<task_num; i++)
        {
            delete_link_queue(&(pool->receiveDeque), &task_list[i]);

		    print_log(_FL_, "------>>>>thread 0x%x..get queue, queue_size[%d]amount[%d]task_list[%d][%p] distinct_id:[%s]\n\t\t\tproperties[%s]\n",(unsigned int)pthread_self(), pool->queue_size, get_link_queue_length(&pool->receiveDeque)+1,i, task_list[i], task_list[i]->distinct_id, task_list[i]->properties);

            pool->queue_size--;
        }


		ret=thread_send_task_list(pool, task_list, task_num);
        if(ret== 0 || ret==-2 )
		{
            pool->step_num=0;
            for(i=0; i<task_num; i++)
            {
                free(task_list[i]);
            }

            free(task_list);
		
		}
		else
        {

			if(pool->queue_size+pool->step_num+pool->add_step_num <= pool->queue_max_size) /*队列未满*/
			{
                step_num=(pool->queue_size / pool->add_step_num + 1)  * pool->add_step_num ;
                if(step_num>0)
                {
                    pool->step_num=step_num;
                }
			}
            else
            {
                pool->step_num= pool->queue_max_size-pool->queue_size;
            }

            touch_readd_task(pool, task_list, task_num);

        }

        /*任务结束处理*/ 
	    print_log(_FL_, "thread 0x%x end working,queue_size[%d] \n\n", (unsigned int)pthread_self(), pool->queue_size);

        /*通知可以有新的任务添加进来*/
		/*
        pthread_cond_broadcast(&(pool->queue_not_full));
		*/

        pthread_mutex_unlock(&(pool->lock));
    }

    pthread_exit(NULL);
}


void adjust_thread(TOUCH *threadpool)
{
    int i;

    int add=0;
    int minus=0;
    int queue_size=0;
    int live_thr_num=0;
    int busy_thr_num=0;
    int add_thr_num=0;
    int minus_thr_num=0;
    int num=0;

    TOUCH *pool = (TOUCH *)threadpool;
    while (!pool->shutdown)
    {
        sleep(pool->send_interval);                                    /*延时10秒*/

        pthread_mutex_lock(&(pool->lock));
        queue_size = pool->queue_size;
        live_thr_num = pool->live_thr_num;
        pthread_mutex_unlock(&(pool->lock));

        pthread_mutex_lock(&(pool->thread_counter));
        busy_thr_num = pool->busy_thr_num;
        pthread_mutex_unlock(&(pool->thread_counter));

        /*任务数大于最小线程池个数并且存活的线程数少于最大线程个数时，创建新线程*/
        if ( busy_thr_num == live_thr_num && live_thr_num < pool->max_thr_num)
        {
            pthread_mutex_lock(&(pool->lock));
            add = 0;
            /*一次增加add_thr_num个线程*/
            num=(pool->max_thr_num-pool->live_thr_num) / 4;
            add_thr_num= num>1 ? num : 1;

            for (i = 0; i < pool->max_thr_num && add < add_thr_num && pool->live_thr_num < pool->max_thr_num; i++)
            {
                if (pool->threads[i] == 0 || !is_thread_alive(pool->threads[i]))
                {
                    pthread_create(&(pool->threads[i]), NULL, touch_send, pool);
                    add++;
                    pool->live_thr_num++;
                }
            }
            pthread_mutex_unlock(&(pool->lock));
        }

        /*销毁多余的空闲线程*/
        if ((busy_thr_num * 2) < live_thr_num
                && live_thr_num > pool->min_thr_num)
        {
            /*一次销毁DEFAULT_THREAD个线程*/
            pthread_mutex_lock(&(pool->lock));
            num=(live_thr_num-pool->min_thr_num) / 4;
            minus_thr_num=num>1 ? num : 1; 
            pool->wait_exit_thr_num =minus_thr_num;
            pthread_mutex_unlock(&(pool->lock));

            for (i = 0; i<minus_thr_num; i++)
            {
                /*通知处在空闲状态的线程*/
                pthread_cond_signal(&(pool->queue_not_empty));
            }
        }
    }
}

void check_task_thread(TOUCH *threadpool)
{
    int i;
    TOUCH *pool = (TOUCH *)threadpool;
    while (!pool->shutdown)
    {
        sleep(pool->send_interval);                                    /*默认延时10秒*/

     
        if(pool->step_num==0)  /*交易正常*/
        {
            if(pool->queue_size>0)
            {
                /*通知处在空闲状态的线程*/
                pthread_cond_signal(&(pool->queue_not_empty));
            }
        }
    }
}


int threadpool_destroy(TOUCH *pool)
{
    int i;
    if (pool == NULL)
    {
        return -1;
    }

    pool->shutdown = true;
    /*先销毁管理线程*/
    pthread_join(pool->check_tid, NULL);
    pthread_join(pool->adjust_tid, NULL);
    for (i = 0; i < pool->live_thr_num; i++)
    {
        /*通知所有的空闲线程*/
        pthread_cond_broadcast(&(pool->queue_not_empty));
        pthread_join(pool->threads[i], NULL);
    }
    threadpool_free(pool);

    /*curl cleanup*/
    curl_global_cleanup();

    return 0;
}

int threadpool_free(TOUCH *pool)
{
    if (pool == NULL)
    {
        return -1;
    }

    if (pool->threads)
    {
        free(pool->threads);
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_mutex_lock(&(pool->thread_counter));
        pthread_mutex_destroy(&(pool->thread_counter));
        pthread_cond_destroy(&(pool->queue_not_empty));
        pthread_cond_destroy(&(pool->queue_not_full));
    }
    free(pool);
    pool = NULL;

    clear_link_queue(&(pool->receiveDeque));

    return 0;
}

int threadpool_all_threadnum(TOUCH *pool)
{
    int all_threadnum = -1;
    pthread_mutex_lock(&(pool->lock));
    all_threadnum = pool->live_thr_num;
    pthread_mutex_unlock(&(pool->lock));
    return all_threadnum;
}

int threadpool_busy_threadnum(TOUCH *pool)
{
    int busy_threadnum = -1;
    pthread_mutex_lock(&(pool->thread_counter));
    busy_threadnum = pool->busy_thr_num;
    pthread_mutex_unlock(&(pool->thread_counter));
    return busy_threadnum;
}

int is_thread_alive(pthread_t tid)
{
    int kill_rc = pthread_kill(tid, 0);
    if (kill_rc == ESRCH)
    {
        return false;
    }
    return true;
}

     /**
     * 设置所有相关APP属性
     *
     * @param superPropertiesMap
     */

int touch_init(TOUCH *touch, char *app_url, char *app_key, char *app_secret, char *app_enable, char *public_app_name, char *public_app_version)
{    
    strcpy(touch->app_url, app_url);
    strcpy(touch->app_key, app_key);
    strcpy(touch->app_secret, app_secret);
    strcpy(touch->app_enable, app_enable);
    strcpy(touch->public_app_name, public_app_name);
    strcpy(touch->public_app_version, public_app_version);

    return 0;
}


     /**
     * 事件埋点信息添加到内存队列中
     *
     * @param distinctId 设备Id
     * @param isLoginId  是否为登录ID
     * @param eventCode  事件code
     * @param properties 事件属性
     */

int touch_track(TOUCH *touch, char *distinctId, int isLoginId, char *eventCode, char *properties)
{
    TASK *md_track; 
    long datetime;

    print_log(_FL_, "touch track:distinctId[%s], isLoginId[%d], eventCode[%s], properties[%s]\n", distinctId, isLoginId, eventCode, properties);

    md_track = (TASK*)malloc(sizeof(TASK));
    memset(md_track, 0x00, sizeof(TASK));

    get_task_tm(&datetime);
    md_track->msg_type=1;  /*消息类型 1-track*/
    strcpy(md_track->distinct_id, distinctId);
    md_track->is_login_id=isLoginId;
    strcpy(md_track->event_code, eventCode);
    md_track->datetime=datetime;
    strcpy(md_track->properties, properties);


    touch_add_task(touch, md_track);

    return 0;
}


     /**
     * 登录id和匿名id绑定(记录用户注册事件)
     * 将用户绑定埋点信息添加到内存队列中
     *
     * @param loginId     登录ID
     * @param anonymousId 匿名ID
     */
int touch_trackSignUp(TOUCH *touch, char *loginId, char * anonymousId, char *properties)
{
    TASK *md_trackSignUp; 
    long datetime;

    print_log(_FL_, "touch_trackSignUp:loginId[%s],anonymousId[%s], properties[%s]\n", loginId, anonymousId, properties);

    md_trackSignUp = (TASK*)malloc(sizeof(TASK));
    memset(md_trackSignUp, 0x00, sizeof(TASK));

    get_task_tm(&datetime);
    md_trackSignUp->msg_type=2; /*消息类型 2-login*/
    strcpy(md_trackSignUp->distinct_id, loginId);
    strcpy(md_trackSignUp->original_id, anonymousId);
    md_trackSignUp->datetime=datetime;
    strcpy(md_trackSignUp->properties, properties);

    touch_add_task(touch, md_trackSignUp);

    return 0;
}

     /**
     * 设置所有埋点数据中的公共属性
     *
     * @param superPropertiesMap
     */

int touch_registerSuperProperties(TOUCH *touch, char *superPropertiesMap )
{

    cJSON *oSuperProperties=NULL;
    cJSON *nSuperProperties=NULL;
    cJSON *sSuperProperties=NULL;
    char * SuperProperties=NULL;

    oSuperProperties= cJSON_Parse(touch->superProperties);
    nSuperProperties= cJSON_Parse(superPropertiesMap);

    sSuperProperties=cJSON_Print_2Object(nSuperProperties, oSuperProperties);
	SuperProperties=cJSON_Print(sSuperProperties);



	if(SuperProperties !=NULL)
	{
		strcpy(touch->superProperties, SuperProperties); 
	}

    cJSON_Delete(oSuperProperties);
    cJSON_Delete(nSuperProperties);
	if(SuperProperties !=NULL)
	{
		free(SuperProperties);
	}

    return 0;
}

     /**
     * 设置全局日志打印参数
     *
     * @param 无
     */


int touch_set_print_flag(int flag)
{
    if(flag==0 || flag==1)
    {
        g_touch_print_flag=flag;
    }
    
    return 0;
}


     /**
     * 获取线程池
     *
     * @param 无
     */


TOUCH * touch_get_instance()
{
    if(g_touch == NULL)
    {
        /*按默认参数创建线程池*/
	    g_touch=touch_create(0,0,0,0,0,0); 
        print_log(_FL_, "touch_get_instance: touch_create again by thread 0x%x \n", (unsigned int)pthread_self());
    }
    else
    {
        return g_touch;
    }
}


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

int touch_global_create(int min_thr_num, int max_thr_num, int queue_max_size, int pack_task_num, int add_step_num, int send_interval)
{
    g_touch=touch_create(min_thr_num, max_thr_num, queue_max_size, pack_task_num, add_step_num, send_interval);
    if(g_touch == NULL)
    {
        print_log(_FL_, "touch global create fail:thread 0x%x \n", (unsigned int)pthread_self());
        return -1;
    }

    return 0;
}


     /**
     * 设置所有相关APP属性
     *
     *
     */

int touch_global_init(char *app_url, char *app_key, char *app_secret, char *app_enable, char *public_app_name, char *public_app_version)
{
    int  ret=0;

    TOUCH * touch=touch_get_instance();
    if(touch == NULL)
    {
        print_log(_FL_, "touch_global_init get instance fail:thread 0x%x \n", (unsigned int)pthread_self());
        return -1;
    }

    ret= touch_init(touch, app_url, app_key, app_secret, app_enable, public_app_name, public_app_version);

    return ret;
}


     /**
     * 事件埋点信息添加到内存队列中
     *
     * @param distinctId 设备Id
     * @param isLoginId  是否为登录ID
     * @param eventCode  事件code
     * @param properties 事件属性
     */

int touch_global_track(char *distinctId, int isLoginId, char *eventCode, char *properties)
{
    int  ret=0;

    TOUCH * touch=touch_get_instance();
    if(touch == NULL)
    {
        print_log(_FL_, "touch_global_track get instance fail:thread 0x%x \n", (unsigned int)pthread_self());
        return -1;
    }

    ret=touch_track(touch, distinctId, isLoginId, eventCode, properties);

    print_log(_FL_, "touch_global_track_success \n", (unsigned int)pthread_self());

    return ret;
}


     /**
     * 登录id和匿名id绑定(记录用户注册事件)
     * 将用户绑定埋点信息添加到内存队列中
     *
     * @param loginId     登录ID
     * @param anonymousId 匿名ID
     */

int touch_global_trackSignUp(char *loginId, char * anonymousId, char *properties)
{
    int  ret=0;

    TOUCH * touch=touch_get_instance();
    if(touch == NULL)
    {
        print_log(_FL_, "touch_global_trackSignUp get instance fail:thread 0x%x \n", (unsigned int)pthread_self());
        return -1;
    }

    ret=touch_trackSignUp(touch, loginId, anonymousId, properties);

    return ret;
}


     /**
     * 设置所有埋点数据中的公共属性
     *
     * @param superPropertiesMap
     */

int touch_global_registerSuperProperties( char *superPropertiesMap )
{
    int  ret=0;

    TOUCH * touch=touch_get_instance();
    if(touch == NULL)
    {
        print_log(_FL_, "touch_global_registerSuperProperties get instance fail:thread 0x%x \n", (unsigned int)pthread_self());
        return -1;
    }

    ret=touch_registerSuperProperties(touch, superPropertiesMap );

    return ret;
}


/*测试使用，作成库时请注释掉下面代码*/ 
#if TEST
int main(void)
{
	int i=0;
	int ret=0;

    char distinct_id[64+1]={0};
    char properties[128]={0};

    /*TOUCH *touch_create(int min_thr_num, int max_thr_num, int queue_max_size, int pack_task_num, int add_step_num, int send_interval)*/
	//TOUCH *touch = touch_create(3,100,1000,100,50,10);    /*线程池里最小1个线程，最大100个，队列最大值12,每包任务数，失败步长，任务轮询间隔*/

	/**strcpy(url, "http://130.1.10.158:8888/data/encryption?appkey=default&token=88a5753fb4519ed97e992871d31ba0c8&t=1607938338786");*/
    /*print_log(_FL_, "touch_init\n");    
    touch_init(touch, "http://130.1.10.158:8888/data", "default", "1qweasd!", "true", "app name", "1.0.0");

    touch_registerSuperProperties(touch, "{\"card_no0\": 1234567890 }" );
    touch_registerSuperProperties(touch, "{\"card_no1\": \"11111111\", \"card_no2\": \"22222222\", \"card_no3\": \"33333333\"}" );
    touch_registerSuperProperties(touch, "{\"card_no9\": 1234567890123 }" );
    touch_registerSuperProperties(touch, "{\"card_no4\": \"44444444\",\"card_no5\": \"55555555\"}" );
    touch_registerSuperProperties(touch, "{\"card_no6\": \"66666666\",\"card_no7\": \"77777777\", \"card_no5\": \"56565656\"}" );

	sleep(2);

    for(i=1; i<=10000; i++)
    {
        sprintf(distinct_id, "%d",i);

        sprintf(properties, "{\"no\": %d, \"card_no\": \"%06dXXXX\", \"bgint\":1234567890123}", i, i);
        touch_track(touch, distinct_id, 1, "AppClick", properties);

    }
    */

    /*
    for(i=1; i<=4000; i++)
    {
        sprintf(distinct_id, "%d",i);
        sprintf(properties, "{\"no\": %d, \"card_no\": \"%06dXXXX\"}", i, i);
        touch_trackSignUp(touch, distinct_id,distinct_id, properties);
	}
	*/


    touch_set_print_flag(1);  /*设置日志打印参数 ：1-打印日志，0-不打印*/

	ret=touch_global_create(3,100,1000,100,50,10);    /*线程池里最小1个线程，最大100个，队列最大值12,每包任务数，失败步长，任务轮询间隔*/

    if(ret != 0)
    {
        return -1;
    }

    print_log(_FL_, "touch_init\n");    
    touch_global_init( "http://130.1.10.158:8888/data", "default", "1qweasd!", "true", "app name", "1.0.0");

    touch_global_registerSuperProperties( "{\"card_no0\": 1234567890 }" );
    touch_global_registerSuperProperties( "{\"card_no1\": \"11111111\", \"card_no2\": \"22222222\", \"card_no3\": \"33333333\"}" );
    touch_global_registerSuperProperties( "{\"card_no9\": 1234567890123 }" );
    touch_global_registerSuperProperties( "{\"card_no4\": \"44444444\",\"card_no5\": \"55555555\"}" );
    touch_global_registerSuperProperties( "{\"card_no6\": \"66666666\",\"card_no7\": \"77777777\", \"card_no5\": \"56565656\"}" );

	sleep(2);

    for(i=1; i<=10000; i++)
    {
        sprintf(distinct_id, "%d",i);

        sprintf(properties, "{\"no\": %d, \"card_no\": \"%06dXXXX\", \"bgint\":1234567890123}", i, i);
        touch_global_track(distinct_id, 1, "AppClick", properties);

    }


     /*
    for(i=1; i<=4000; i++)
    {
        sprintf(distinct_id, "%d",i);
        sprintf(properties, "{\"no\": %d, \"card_no\": \"%06dXXXX\"}", i, i);
        touch_global_trackSignUp(touch, distinct_id,distinct_id, properties);
	}
	*/



	/*threadpool_destroy(touch);*/

	while(true)
	{
       sleep(5);	
	}

	return 0;
}
#endif

