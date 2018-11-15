### 18-11-15

### **修改说明**
修改`do_ret`宏定义，其中给功能块的返回count数计算错误，未减去input变量的个数。（未测试）

---

### 18-05-25

### **修改说明**
修改`do_ucall`与`do_ret`宏定义，将函数与功能块分开处理。

对于`do_ucall`，原来的思路是一个FB变量对应一个功能块定义，根据两者关联，由功能块声明找到统一集合vref中的fb变量，然后把所有变量信息读取到新栈帧的寄存器组中，现在的思路是在编译器端已经把FB变量所有信息依次放到旧栈帧的寄存器组中，新栈帧只需要读取`UPOU_INPUTC(i) + UPOU_INOUTC(i) + UPOU_OUTPUTC(i) + UPOU_LOCALC(i)`个寄存器即可，对于函数，只需要读取`UPOU_INPUTC(i) + UPOU_INOUTC(i)`个寄存器。
对于`do_ret`，当使用`ucall A Bx`执行子程序后返回。对于函数，把返回值放在旧栈帧的的A位置；对于功能块，把`INOUT ，OUTPUT ，LOCAL`变量放在旧栈帧的`A+UPOU_INPUTC(CURR_SF.pou)`位置（只是为了直观性），然后在接下来的程序中，会把这些值存回统一集合vref中的fb变量中。原来的思路是，直接在运行系统把`INOUT ，OUTPUT ，LOCAL`三类变量写回vref中的fb变量，不返回任何值（因为功能块就是没有返回值的，只有输出变量）。

---
### 18-05-24

### **修改说明**
将部分使用`value_p`修改为`value_i/value_u`类型

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
