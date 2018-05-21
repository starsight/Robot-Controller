#include <native/task.h>
#include <native/heap.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <iostream>
#include "sysconfig.h"
#include "io.h"
using namespace std;

#define SYS_SUPERVISOR_NAME 		"sys_supervisor_task"
#define SYS_SUPERVISIOR_PRIORITY 	99
#define SYS_SUPERVISIOR_PERIOD 		1000000000

#define SYS_MANAGER_NAME 			"sys_manager_task"
#define SYS_MANAGER_PRIORITY		98

#define DEV_TASK_NAME				"dev_task"

RT_TASK 		sys_manager_desc;		/* 系统管理器线程描述符 */
RT_TASK 		sys_supervisor_desc;	/* 系统监视器线程描述符 */
RT_TASK 		dev_task_desc;

extern RT_HEAP 		ioconf_heap;		/* IO配置堆描述符 */
extern RT_HEAP 		svconf_heap;		/* 伺服配置堆描述符 */
extern RT_HEAP 		rcconf_heap;		/* rc配置堆描述符 */
extern RT_HEAP		sv_heap_desc;		/* 伺服映像区堆描述符 */
extern RT_HEAP		rc_heap_desc;		/* RC与PLC共享内存区堆描述符 */
extern IOMem 		io_shm;				/* I/O映像区(包含了分配堆描述符，和返回指针) */

pid_t dev_pid, plc_pid, rc_pid;			/* 设备接口任务 PLC任务进程 RC任务进程号 */

/**
 *	函数功能：启动硬件设备接口进程
 *	正常返回 0 
 */
int insert_device_interface(pid_t dev_pid){
	if((dev_pid = fork()) < 0)
		LOGGER_ERR(E_SVTASK_FORK,"");
	else if(dev_pid == 0){
		if(execlp("./devices/dev-task","dev-task",NULL) < 0)
			LOGGER_ERR(E_SVTASK_EXEC,"");
	}
	else{
		return 0;
	}
}

/**
 *	函数功能：启动PLC控制进程
 *	正常返回 0 
 */
int insert_plc_task(pid_t plc_pid){
	if((plc_pid = fork()) < 0)
		LOGGER_ERR(E_PLCTASK_FORK,"");
	else if(plc_pid == 0){
		if(execlp("./iec-runtime/iec-runtime", "iec-runtime",NULL) < 0)
			LOGGER_ERR(E_PLCTASK_EXEC,"");
	}else{
		return 0;
	}
}

/**
 *	函数功能：启动RC控制进程
 *	正常返回 0 
 */
int insert_rc_task(pid_t rc_pid){
	if((rc_pid = fork()) < 0)
		LOGGER_ERR(E_RCTASK_FORK,"");
	else if(rc_pid == 0){
		if(execlp("./rc-runtime/rc-runtime","rc-runtime",NULL) < 0)
			LOGGER_ERR(E_RCTASK_EXEC,"");
	}else{
		return 0;
	}
}

void sig_handler(int signo){
	if(signo == SIGINT){
		printf("cp1\n");
		// if(kill(dev_pid,SIGUSR1) < 0)
		// 	printf("send signal to dev-task error\n");
		printf("cp3\n");
		rt_task_delete(&sys_supervisor_desc);
		rt_task_delete(&sys_manager_desc);
		io_conf_delete(&ioconf_heap);
		io_mem_delete(&io_shm);
		sv_conf_delete(&svconf_heap);
		sv_mem_delete(&sv_heap_desc);
		rc_mem_delete(&rc_heap_desc);

		printf("exit sys\n");
		exit(0);
	}else{

	}
}

// 子任务启动过程（后续可能加入当硬件设备任务启动完成-->启动PLC任务，等待PLC任务启动完成-->启动RC任务）
int sys_task_start(){
	insert_device_interface(dev_pid);
	//rt_task_bind(&dev_task_desc,DEV_TASK_NAME,TM_INFINITE);
	insert_plc_task(plc_pid);
	insert_rc_task(rc_pid);
	return 0;
}

// 系统监控器线程
static void system_supervisor_routine(void *cookie){
	rt_task_set_periodic(NULL,TM_NOW, SYS_SUPERVISIOR_PERIOD);
	while(1){
		rt_task_wait_period(NULL);
		// cout << "shabi" << endl;
	}
}

// 系统管理器线程 
static void sys_manager_routine(void *cookie){
	int err = 0;
	err = rt_task_create(&sys_supervisor_desc, SYS_SUPERVISOR_NAME,0,SYS_SUPERVISIOR_PRIORITY,T_JOINABLE);
	err = SystemConfig();
	err = sys_task_start();
	if(!err){
		rt_task_start(&sys_supervisor_desc,&system_supervisor_routine,NULL);
	}
	pause();
}

int main(){
	if(signal(SIGINT,&sig_handler) == SIG_ERR){
		std::cout << "SIGINT error" << std::endl;
	}
	int err = 0;
	err = rt_task_create(&sys_manager_desc, SYS_MANAGER_NAME,0,SYS_SUPERVISIOR_PRIORITY,T_JOINABLE);
	if(!err)
		rt_task_start(&sys_manager_desc,&sys_manager_routine,NULL);
	pause();
	return 0;
}