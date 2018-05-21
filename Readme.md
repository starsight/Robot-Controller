工业机器人通用控制器运行系统软件ｖ2.0

系统运行环境：　Ubuntu 14.04 + Xenomai 2.6.4
EtherCAT软主站: IgH EtherCAT Master 1.5.2

共包含４个进程，需要分别编译　
系统管理进程(sysmanager目录)　　　　cd ./sysmanager/src && make
软PLC进程(iec-runtime目录)     cd ./iec-runtime/kernel && make
RC进程(rc-runtime目录)
根据实际示教动态链接库目录修改./rc-runtime/src/CMakeLists.txt中link_directories          
cd ./rc-runtime/build/ && cmake .. && make

EtherCAT总线进程(devices目录)   cd ./devices && make

---

更新日志

[update-log](update-log.md)