cmake_minimum_required(VERSION 3.12)
project(GateServer)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -Wextra -Werror=return-type -pthread)
# 关键：动态库+静态库混合链接的链接器选项
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -Wl,--no-as-needed")

add_definitions(-DPROJECT_ROOT_DIR="${CMAKE_SOURCE_DIR}")

# ================= 1. 路径配置 =================
set(gRPC_ROOT "/usr/local")
set(Protobuf_ROOT "/usr/local")

# ================= 2. 依赖库查找 =================
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSONCPP REQUIRED jsoncpp)

find_package(Boost REQUIRED COMPONENTS thread system filesystem date_time regex)
if(NOT Boost_FOUND)
    message(FATAL_ERROR "Boost库未找到！执行：sudo apt install libboost-all-dev")
endif()

# Protobuf/gRPC 用动态库（系统自动处理依赖）
# 手动指定 protoc 路径（关键！你之前没定义这个变量）
set(Protobuf_PROTOC_EXECUTABLE /usr/local/bin/protoc)
set(Protobuf_LIBRARIES "/usr/local/lib/libprotobuf.so")
set(gRPC_LIBRARIES
    "${gRPC_ROOT}/lib/libgrpc++.so"
    "${gRPC_ROOT}/lib/libgrpc.so"
    "${gRPC_ROOT}/lib/libgpr.so"
    "${gRPC_ROOT}/lib/libupb.so"
)

# ================= 3. Proto 编译（适配你的项目结构） =================
file(GLOB PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/proto/*.proto)
# 直接生成到你项目的 src 和 include 目录（和你之前的逻辑一致）
# 在遍历 proto 文件前，加上这两行
set(PROTO_HDR_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROTO_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
file(MAKE_DIRECTORY ${PROTO_HDR_DIR} ${PROTO_SRC_DIR})
set(PROTO_SRCS "")

foreach(PROTO_FILE ${PROTO_FILES})
    get_filename_component(PROTO_NAME ${PROTO_FILE} NAME_WE)
    get_filename_component(PROTO_PATH ${PROTO_FILE} PATH)
    
    set(PB_H ${PROTO_HDR_DIR}/${PROTO_NAME}.pb.h)
    set(PB_CC ${PROTO_SRC_DIR}/${PROTO_NAME}.pb.cc)
    set(GRPC_PB_H ${PROTO_HDR_DIR}/${PROTO_NAME}.grpc.pb.h)
    set(GRPC_PB_CC ${PROTO_SRC_DIR}/${PROTO_NAME}.grpc.pb.cc)

    add_custom_command(
        OUTPUT ${PB_H} ${PB_CC} ${GRPC_PB_H} ${GRPC_PB_CC}
        COMMAND ${Protobuf_PROTOC_EXECUTABLE}
        ARGS 
            --cpp_out=${PROTO_SRC_DIR}
            --grpc_out=${PROTO_SRC_DIR}
            --plugin=protoc-gen-grpc=/usr/local/bin/grpc_cpp_plugin
            -I${PROTO_PATH}
            ${PROTO_FILE}
        # 把生成的 .h 文件移动到 include 目录
        COMMAND ${CMAKE_COMMAND} -E rename ${PROTO_SRC_DIR}/${PROTO_NAME}.pb.h ${PB_H}
        COMMAND ${CMAKE_COMMAND} -E rename ${PROTO_SRC_DIR}/${PROTO_NAME}.grpc.pb.h ${GRPC_PB_H}
        DEPENDS ${PROTO_FILE}
        COMMENT "Compiling proto: ${PROTO_FILE}"
    )
    list(APPEND PROTO_SRCS ${PB_CC} ${GRPC_PB_CC})
endforeach()

# ================= 4. 源文件与包含目录 =================
include_directories(
    ${PROJECT_SOURCE_DIR}/include
    ${PROJECT_SOURCE_DIR}/src
    ${Boost_INCLUDE_DIRS}
    ${JSONCPP_INCLUDE_DIRS}
    ${gRPC_ROOT}/include
    ${Protobuf_ROOT}/include
    /usr/include/mysql
    /usr/include/cppconn
)

file(GLOB ROOT_SRC "${PROJECT_SOURCE_DIR}/*.cpp")
file(GLOB SRC_DIR_SRC "${PROJECT_SOURCE_DIR}/src/*.cpp")
set(SOURCE_FILES ${ROOT_SRC} ${SRC_DIR_SRC} ${PROTO_SRCS})

# ================= 5. 生成可执行文件 =================
add_executable(GateServer ${SOURCE_FILES})
set(EXECUTABLE_OUTPUT_PATH ${PROJECT_BINARY_DIR}/bin)

# ================= 5. 最终链接（纯静态库版，绝对能找到文件） =================
# 核心：使用 --start-group 解决静态库循环依赖，不需要纠结顺序
# 修正链接器标志（必须配对 start-group 和 end-group）
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -pthread -Wl,--start-group")

target_link_libraries(GateServer
    PRIVATE
    # 1. 业务库：Redis++
    /usr/local/lib/libredis++.a
    /usr/local/lib/libhiredis.a

    # 2. MySQL：C++ 驱动在前，C 驱动在后（关键修正！）
    /usr/lib/x86_64-linux-gnu/libmysqlcppconn.a
    /usr/lib/x86_64-linux-gnu/libmysqlclient.a

    # 3. gRPC 核心静态库
    /usr/local/lib/libgrpc++.a
    /usr/local/lib/libgrpc.a
    /usr/local/lib/libgpr.a
    /usr/local/lib/libupb.a

    # 4. Protobuf 静态库
    /usr/local/lib/libprotobuf.a

    # 5. Abseil 静态库（补全所有缺失符号）
    /usr/local/lib/libabsl_synchronization.a
    /usr/local/lib/libabsl_strings.a
    /usr/local/lib/libabsl_strings_internal.a
    /usr/local/lib/libabsl_str_format_internal.a
    /usr/local/lib/libabsl_status.a
    /usr/local/lib/libabsl_base.a
    /usr/local/lib/libabsl_log_severity.a
    /usr/local/lib/libabsl_raw_logging_internal.a
    /usr/local/lib/libabsl_time.a
    /usr/local/lib/libabsl_time_zone.a
    /usr/local/lib/libabsl_civil_time.a
    /usr/local/lib/libabsl_int128.a
    /usr/local/lib/libabsl_hash.a
    /usr/local/lib/libabsl_hashtablez_sampler.a
    /usr/local/lib/libabsl_cord.a
    # /usr/local/lib/libabsl_cord_internal.a  # 新增
    /usr/local/lib/libabsl_raw_hash_set.a
    /usr/local/lib/libabsl_graphcycles_internal.a
    /usr/local/lib/libabsl_city.a
    /usr/local/lib/libabsl_exponential_biased.a
    /usr/local/lib/libabsl_spinlock_wait.a
    /usr/local/lib/libabsl_stacktrace.a
    /usr/local/lib/libabsl_symbolize.a
    /usr/local/lib/libabsl_debugging_internal.a
    /usr/local/lib/libabsl_demangle_internal.a
    /usr/local/lib/libabsl_malloc_internal.a
    /usr/local/lib/libabsl_bad_optional_access.a
    /usr/local/lib/libabsl_bad_variant_access.a
    /usr/local/lib/libabsl_throw_delegate.a

    # 6. gRPC 底层依赖静态库
    /usr/local/lib/libre2.a
    /usr/local/lib/libcares.a
    /usr/local/lib/libaddress_sorting.a

    # 7. 系统库（最后放）
    ${Boost_LIBRARIES}
    ${JSONCPP_LIBRARIES}
    /usr/lib/x86_64-linux-gnu/libssl.a
    /usr/lib/x86_64-linux-gnu/libcrypto.a
    /usr/lib/x86_64-linux-gnu/libz.a
    dl
    pthread
    rt
    -Wl,--end-group
)