#include <sys/mman.h>	/* required for mlockall() */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <native/heap.h>
#include <native/mutex.h>
#include <native/cond.h>
#include <native/queue.h>

#include "loader.h"
#include "vm.h"
#include "../../include/robotctl.h"
#include "../../include/servo.h"
#include "../../include/logger.h"

#include <fstream>
std::ofstream outfile;                  /* for debug */

#define MAX_OBJNAME	50
char objname[MAX_OBJNAME] = "exec.obj";

TaskList plc_task;

RT_HEAP	  rc_heap_desc;					/* RC/PLC共享数据区描述符 */
RT_HEAP   sv_heap_desc;					/* 伺服映像数据共享区描述符 */

// rc_cond_desc未使用
RT_COND   rc_cond_desc;					/* RC/PLC同步对象－－条件变量描述符 */
// rc_mutex_desc未使用
RT_MUTEX  rc_mutex_desc;				/* RC/PLC同步对象－－互斥量描述符 */

RT_MUTEX  sv_mutex_desc;				/* PLC/Servo同步对象－－互斥量描述符 */

RT_QUEUE  mq_rc_interp_desc;			/* 消息队列描述符 PLC --> RC插补器 */
RT_QUEUE  mq_plc_desc;					/* 消息队列描述符 RC插补器 --> PLC */

SVMem* 	 sv_shm;						/* 伺服共享区指针 */
RCMem* 	 rc_shm;						/* RC/PLC共享区指针 */

void syncobj_create(){
	sv_mem_bind(&sv_heap_desc, &sv_shm);
	rc_mem_bind(&rc_heap_desc, &rc_shm);
	sv_syncobj_create(&sv_mutex_desc);
	rc_syncobj_create(&rc_mutex_desc,&rc_cond_desc);
	rt_queue_create(&mq_rc_interp_desc, MQ_RC_INTERP_NAME, 50, Q_UNLIMITED, Q_FIFO);
	rt_queue_create(&mq_plc_desc, MQ_PLC_NAME, 50, Q_UNLIMITED, Q_FIFO);
}

void sig_handler(int signo){
	LOGGER_DBG(DFLAG_SHORT, "PLC Kernel Recieved Signal: %d",(int)signo);
	if(signo == SIGINT){
		plc_task_delete(&plc_task);
		exit(0);
	}
	//outfile.close();
}

int main(int argc, char* argv[]){
	mlockall(MCL_CURRENT|MCL_FUTURE);
    //outfile.open("/home/seu/workspace/logfile/plcloginfo.txt");	
    outfile.open("/home/ychj/wenjie/plcloginfo.txt");
    outfile << "logfile" << std::endl;
	if(signal(SIGINT, &sig_handler) == SIG_ERR){
		LOGGER_ERR(E_SIG_PLCKILL,"");
	}
	syncobj_create();
	load_task_list(objname, &plc_task);
	plc_task_init(&plc_task);
	plc_task_start(&plc_task);
	pause();
}