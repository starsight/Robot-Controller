// #include "plc.h"
#include <exception>

#include <native/task.h>
#include <native/timer.h>

#include <signal.h>
#include <unistd.h>
#include <setjmp.h>  						// 用于设置长跳转，实现解释器运行控制
#include "opmanager.hh"

#include "RCInterpreter.h"
#include "RSIExecutor.h"

// RT_TASK 		rc_supervisor_desc;			/* RC监控器任务描述符 */
// RT_TASK_INFO  	rc_supervisor_info;		/* RC监控器任务状态 */

RT_TASK 		rc_manager_desc;			/* RC管理器任务描述符 */
RT_TASK_INFO  	rc_manager_info;			/* RC管理器任务状态 */

RT_TASK 		rc_interp_desc;				/* RC interp任务描述符 */
RT_TASK_INFO  	rc_interp_info;				/* RC interp任务状态 */

RT_TASK 		rc_executor_desc;			/* RC执行器任务描述符 */
RT_TASK_INFO  	rc_executor_info;			/* RC执行器任务状态 */

RT_TASK 		rc_rsi_desc;				/* RSI任务描述符 */
RT_TASK_INFO  	rc_rsi_info;				/* RSI任务状态 */

RT_COND 		rc_cond_desc;               /* RC/PLC同步对象－－条件变量描述符 */
RT_MUTEX 		rc_mutex_desc;             	/* RC/PLC同步对象－－互斥量描述符 */
RT_HEAP 		rc_heap_desc;				/* 共享内存堆描述符　由系统管理器创建　RC绑定　*/
RT_HEAP  		rc_conf_desc;				/* 系统配置堆描述符　由系统管理器创建　RC绑定　*/
RobotConfig 	*rc_conf;					/* 机器人配置信息变量指针 */
RCMem 			*rc_shm;					/* RC与PLC共享内存区指针 */

// RobotConfig  	rc_runtime_param;			/* 机器人runtime parameters */	
RobotModel* pRobot;
InterpolationInterface* pInterpolator;
// std::shared_ptr<RobotModel> pRobot;
// std::shared_ptr<InterpolationInterface> pInterpolator;

RCOperationCtrl rc_core;					/* RC 运行状态核心数据　*/

jmp_buf 		exec_startpoint;			/* 解释执行器起点　*/
jmp_buf 		interp_startpoint;			/* RC 插补器起点　*/
jmp_buf 		rsi_startpoint;				/* RSI实时任务起点 */

RT_QUEUE 		mq_rc_manager_desc;			/* RC 运行管理器消息队列　由RC创建　*/
RT_QUEUE 		mq_rc_exec_desc;			/* RC 解释执行器消息队列　由RC创建　*/
RT_QUEUE 		mq_rc_interp_desc;			/*　RC 插补器消息队列　由PLC创建　RC 绑定　*/
RT_QUEUE		mq_plc_desc;				/*　PLC 消息队列　由PLC创建　RC 绑定　*/
RT_QUEUE 		mq_rc_rsi_desc;				/* RSI 消息队列描述符 */

ROBOT_INST 		robot_inst_buffer_code; 	/* 指令缓冲区　*/
bool 			robot_inst_buffer_flag=false;
RT_COND 		inst_cond_desc;				/* 指令缓冲区条件变量描述符　*/
RT_MUTEX		inst_mutex_desc;			/* 指令缓冲区互斥锁描述符　*/

bool INRSIPeriod;
bool posFirstFlag;
/**
 * 函数名：rsi_routine
 * 函数功能：RC RSI实时任务
 * 参数：cookie  用户给定参数
 * 返回值：无
 */
void rsi_routine(void *cookie) {
	std::cout << "rsi start" << std::endl;
	
	rt_queue_flush(&mq_rc_rsi_desc);

	std::string *rsiFileName = (std::string*)cookie;
	/* create a RSI Executor according to file name */
	RSIExecutor rsiExec(*rsiFileName);

 	/* compile the specified rsi file to obtain address space and code shadow */
 	try {
		rsiExec.compile();
	} catch(rc_exception &e) {
		e.what();
	} catch(std::exception &e) {
		std::cout << "C++ runtime exception" << std::endl;
	}
	/* set RSIStopFlag as false to start to running RSI */
	RSIStopFlag = false;
	INRSIPeriod = false;
	/* set RSI executor running period */
	rt_task_set_periodic(NULL, TM_NOW, RSI_RUN_PERIOD);
	int jstatus = setjmp(rsi_startpoint);
	posFirstFlag = true;
	/* start running RSI periodic */
	while(!RSIStopFlag){
		rt_task_wait_period(NULL);
		INRSIPeriod = !INRSIPeriod;
		std::cout << "InRSIPeriod = " << INRSIPeriod << std::endl;
		try {
			RTIME start = rt_timer_read();
			rsiExec.execute();
			// INRSIPeriod = !INRSIPeriod;
#ifndef RSI_DEBUG
			if(!RSIStopFlag) {
				rsi_step();
			}
#endif
			RTIME end = rt_timer_read();
			std::cout << "RSI cost " << end << "---" << start << "---" << end - start << " ns" << std::endl;
		} catch(rc_exception &e) {
			e.what();
			RSIStopFlag = true;
		} catch(std::exception &e) {
			std::cout << "C++ runtime exception" << std::endl;
			RSIStopFlag = true;
		}
	}
}



/**
 * 函数名：interp_routine
 * 函数功能：RC插补器实时任务
 * 参数：cookie  用户给定参数
 * 返回值：无
 */
static void interp_routine(void *cookie){

	char** argv = (char**)cookie;
	pInterpolator = new InterpolationDefault(pRobot);
	// pInterpolator = std::make_shared<InterpolationDefault>(InterpolationDefault(pRobot));
	int jstatus = setjmp(interp_startpoint);
	while(1){
		fprintf(stderr,"\n|<< ------------------------------ interp start ---------------------- >>|\n");
		rc_core.interp_status = 0;
		// waiting for order input
		// read order
		rc_core.jog_mode = 0;
		ROBOT_INST temp_inst;

		inst_buffer_read(temp_inst);
		
		rc_core.interp_status = 1;
		// rc_shm->interp_startup_flag = 1;
		// rc_shm->interp_status = 1;
		// waiting for start interp msg

		interp_compute(temp_inst);
		// std::cout << "**************** interp compute fininsh" << std::endl;

		if(rc_core.jog_mode == 1) {
			rc_core.jog_mode = 0; // clear jog mode
		} else {
			send_cmd_to_exec(CMD_NEXT);
		}
		// std::cout << "******************* interp routine send message to executor" << std::endl;
		SHM_INTERP_START_CLR(rc_shm);
		SHM_INTERP_STATUS_CLR(rc_shm);
	
		// TODO: send NEXT msg to exec
		rt_task_sleep(1000000);
	}
}

/**
 * 函数名：executor_routine
 * 函数功能：RC解释执行器实时任务
 * 参数：cookie  用户给定参数
 * 返回值：无
 */
static void executor_routine(void *cookie){

	char** argv = (char**)cookie;

	while(1){
		int jstatus = setjmp(exec_startpoint);
		rc_core.exec_run_mode = 0;
		if(rc_core.startup) { 			/* 判断是否启动运行 */
			rc_core.exec_run_mode = 1;

			// 测试RSI
			// std::string project("./rc-runtime/test/rsidemo");
			// std::string program("./rc-runtime/test/rsidemo/rsisample");
			
			// std::string project("./rc-runtime/test/Home");
			// std::string program("./rc-runtime/test/Home/sample");
			// RCInterpreter executor(project,program);
			RCInterpreter executor(rc_core.cur_project, rc_core.cur_program);

			try{
				executor.compile();
				STEPCHECK();
				executor.execute();
			} catch(rc_exception &e) {
				e.what();
				rc_core.startup = 0;
				longjmp(exec_startpoint, 0);        /* 跳转至解释器起点 */
			} catch(std::exception &e) {
				std::cout << "C++ runtime exception" << std::endl;
				rc_core.startup = 0;
				longjmp(exec_startpoint, 0);        /* 跳转至解释器起点 */
			}
		}
		rt_task_sleep(1000000);
	}
}

/**
 * 函数名：supervisor_routine
 * 函数功能：RC运行控制器实时任务，
 * 参数：cookie  用户给定参数
 * 返回值：无
 */
static void manager_routine(void *cookie){
	int err = 0;
	// init_runtime_param();
	/* 与系统中其他任务创建的信息进行绑定　*/
	rc_conf_bind(&rc_conf_desc,&rc_conf);
	rc_mem_bind(&rc_heap_desc,&rc_shm);			/* rc_shm绑定共享内存区地址 */
	rc_syncobj_bind(&rc_mutex_desc, &rc_cond_desc);    /* 绑定RC/PLC同步对象 */
	
	pRobot = new RobotModel(rc_conf);
	// pRobot = std::make_shared<RobotModel>(RobotModel(rc_conf));
	// pRobot->PrintInformation(std::cout);

	if(rt_queue_bind(&mq_rc_interp_desc, MQ_RC_INTERP_NAME, TM_INFINITE) < 0){
		fprintf(stderr,"interp message queue bind error\n");
		exit(1);
	}
	if(rt_queue_bind(&mq_plc_desc, MQ_PLC_NAME, TM_INFINITE) < 0){
		fprintf(stderr,"plc message queue bind error\n");
		exit(1);
	}
	/* RC 任务中需要创建的同步对象 */
	rt_queue_create(&mq_rc_manager_desc, MQ_RC_MANAGER_NAME, 50, Q_UNLIMITED, Q_FIFO);
	rt_queue_create(&mq_rc_exec_desc, MQ_RC_EXEC_NAME, 50, Q_UNLIMITED, Q_FIFO);
	rt_queue_create(&mq_rc_rsi_desc, MQ_RC_RSI_NAME, 50, 1, Q_FIFO);
	rt_mutex_create(&inst_mutex_desc, INST_MUTEX_NAME);
	rt_cond_create(&inst_cond_desc, INST_COND_NAME);

	err = rt_task_create(&rc_executor_desc, RC_EXECUTOR_NAME, 0, RC_EXECUTOR_PRIORITY, T_JOINABLE|T_FPU|T_CPU(1));
	err = rt_task_create(&rc_interp_desc, RC_INTERP_NAME, 0, RC_INTERP_PRIORITY, T_JOINABLE|T_FPU|T_CPU(1));
	// err = rt_task_create(&rc_rsi_desc, RC_RSI_NAME, 0, RC_RSI_PRIORITY, T_JOINABLE|T_FPU);
	if(!err){
		rt_task_start(&rc_executor_desc, &executor_routine, NULL);
		rt_task_start(&rc_interp_desc, &interp_routine, NULL);
		// rt_task_start(&rc_rsi_desc, &rsi_routine, NULL);
	}else{
		fprintf(stderr,"can't start executor\n");
		exit(1);
	}
	rc_core_init();					// rc状态信息初始化
	start_client_service(0, 0);   	// listen to teachbox
}

void cleanup(void){
	int ret = 0;
	ret = rt_task_delete(&rc_interp_desc);
	if(!ret)
		ret = rt_task_delete(&rc_executor_desc);
	if(!ret)
		ret = rt_task_delete(&rc_manager_desc);
	// rt_task_delete(&rc_rsi_desc);
	if(!ret)
		ret = rt_queue_delete(&mq_rc_exec_desc);
	if(!ret)
		ret = rt_queue_delete(&mq_rc_manager_desc);
	if(!ret)
		ret = rt_queue_delete(&mq_rc_rsi_desc);
	if(!ret)
		ret = rt_mutex_delete(&inst_mutex_desc);
	if(!ret)
		ret = rt_cond_delete(&inst_cond_desc);
	if(ret) 
		printf("RC clean up error\n");
	printf("RC clean up finish\n");
}

void sig_handler(int signo){
	if(signo == SIGINT){
	    printf("Receive Signal No: %d \n", signo);
	    cleanup();
	    exit(0);
  	} else if(signo == SIGUSR1){
  		printf("Receive User1 Signal\n");
	}
}


int main(int argc, char **argv) {

	signal(SIGINT, sig_handler);
	signal(SIGUSR1, sig_handler);
	printf("RC Start ...\n");

	int err = rt_task_create(&rc_manager_desc, RC_MANAGER_NAME, 0, RC_MANAGER_PRIORITY, T_JOINABLE|T_CPU(1));

	if(!err){
		rt_task_start(&rc_manager_desc, &manager_routine, argv);
	}

	printf("I am in RC main\n");
	printf("Robot Current Operation Status:\n");
	printf("RCMode :");
	printf((rc_core.mode == OP_TEACH ? "Teach mode\n" : "PlayBack mode\n"));
	printf("Move Mode : ");
	rc_core.stepflag == 1 ? printf("Step mode\n") : printf("Continuous mode\n");
	// 模拟示教盒操作
	
	printf("Please enter Teach Box Command\n");
	printf("p: import project\n");
	printf("f: import program file\n");
	printf("r: reset the program pointer\n");
	printf("s: start button press down\n");
	printf("w: start button up\n");
	printf("c: change run mode\n");
	printf("t: turn on or off the power\n");
	int stepflag = 0;
	while(1){
		char cmd;
		scanf("%c",&cmd);
		switch(cmd){
			case 'p':
				printf("project home imported\n");
				replyFileAndNames("Home");
				break;
			case 'f':
				printf("program file imported\n");
				importFileChanged("sample");
				break;
			case 'r':
				printf("the pointer reseted\n");
				resetPointer(2,7);
				break;
			case 's':
				printf("start down!\n");
				startKeyDown();
				break;
			case 'w':
				printf("start up!\n");
				startKeyUp();
				break;
			case 'c':
				printf("run mode changed to ");
				stepflag = (int)(!stepflag);
				statusChange(1,stepflag);
				if(rc_core.stepflag) printf("step\n");
				else 		 printf("continuous\n");
				break;
			case 't':
				printf("The servo Powered ");
				if(SHM_RC_MODE_GET(rc_shm) == OP_TEACH)
					SHM_RC_MODE_SET(rc_shm);
					// rc_shm->rc_mode = OP_RUN;
				else 
					SHM_RC_MODE_CLR(rc_shm);
					// rc_shm->rc_mode = OP_TEACH;
				printf("%s",(SHM_RC_MODE_GET(rc_shm) ? "ON\n":"OFF\n"));
				break;
			default:
				break;
		}
	}

	pause();

	return 0;
}
