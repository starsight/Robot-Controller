---

### 18-05-21

### **修改说明**

从师兄工程库转移到新的repository，完成各个模块编译，为了修改CMakeLists.txt中link_directories路径，把CMakeCache.txt文件删除重新cmake。

- rc-runtime/src/CMakeLists.txt 此操作为修改示教器lib目录，固定操作，不是为模拟所用。

```
-link_directories(/usr/xenomai/lib /root/workspace/ychj/RobotController/rc-runtime/lib)
+link_directories(/usr/xenomai/lib /home/ychj/wenjie/Robot-Controller/rc-runtime/lib)
```

为了完成系统的模拟运行，修改一下几个文件：

- sysmanager/src/main.cc     

```c++
sys_task_start(){
    insert_device_interface(dev_pid);
    //rt_task_bind(&dev_task_desc,DEV_TASK_NAME,TM_INFINITE); //注释此行
    insert_plc_task(plc_pid);
    insert_rc_task(rc_pid);
    return 0;
}
```

- iec-runtime/include/libsys.h
```c++
inline void sfun_servo_poweron(IValue *reg_base){
    outfile << "[sfun_servo_poweron]" << std::endl;
    sv_shm->ctrl_area.ControlWord = SERVO_RUN;
    //reg_base->v.value_i = sv_shm->ctrl_area.PowerOnFlag; // 注释此行
     reg_base->v.value_i = 1; //不注释此行
    // printf("伺服启动完成？　%d\n",sv_shm->ctrl_area.PowerOnFlag);
}
```

- rc-runtime/src/main.cc

```c++
// 模拟示教盒操作
/*	//取消模拟示教盒操作
...
*/
```

- include/logger. 控制台输出内容等级控制
```
-#define LOGGER_LEVEL	LEVEL_TRA
+#define LOGGER_LEVEL	LEVEL_DBG
```

若要上机测试需要进行反向恢复
