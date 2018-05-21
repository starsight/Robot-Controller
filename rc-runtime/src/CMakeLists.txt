# This file is in /src

# SET(CMAKE_C_COMPILER g++)

SET(CMAKE_CXX_COMPILER g++)
SET(CMAKE_C_COMPILER g++)

# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -Wno-unused-variable -pthread")
# SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -g -Wall -Wno-unused-variable -pthread")
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11  -pthread")
# SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c++0x -g -I/home/mynfs/usr/xenomai/include -L/home/mynfs/usr/xenomai/lib")

link_directories(/usr/xenomai/lib /home/ychj/wenjie/Robot-Controller/rc-runtime/lib)
SET(LIBRARIES libnative.so libnative.a libxenomai.so libxenomai.a libpthread_rt.so libpthread_rt.a libServerT.so libantlr4-runtime.so)

AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR} MAIN_SRC)
# AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/rcinterpolator STAGE6_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/rcinterpolator/estun STAGE6_ESTUN_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/plcInterface PLC_INTERFACE)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/teachbox TEACHBOX_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/RSI RSI_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/RSI/TinyXml TINYXML_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/codeparser CODEPARSER_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/dataparser DATAPARSER_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/rcinterpreter RCINTERPRETER_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/utility UTILITY_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/robotmodel ROBOTMODEL_SRC)
AUX_SOURCE_DIRECTORY(${CMAKE_CURRENT_SOURCE_DIR}/Interpolation INTERPOLATION_SRC)


ADD_EXECUTABLE(rc-runtime       ${MAIN_SRC}
                                ${STAGE6_SRC}
                                ${STAGE6_ESTUN_SRC}
                                ${PLC_INTERFACE}
                                ${TEACHBOX_SRC}
                                ${RSI_SRC}
                                ${TINYXML_SRC}
                                ${CODEPARSER_SRC}
                                ${DATAPARSER_SRC}
                                ${RCINTERPRETER_SRC}
                                ${UTILITY_SRC}
                                ${ROBOTMODEL_SRC}
                                ${INTERPOLATION_SRC})
target_link_libraries(rc-runtime ${LIBRARIES})
