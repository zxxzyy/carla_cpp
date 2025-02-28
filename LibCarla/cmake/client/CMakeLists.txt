cmake_minimum_required(VERSION 3.5.1)
project(libcarla-client)

# 启用/安装 RSS VARIANT
option (BUILD_RSS_VARIANT "Enables ad-rss-lib based RSS components" OFF)
set(carla_target_postfix "")
if (BUILD_RSS_VARIANT)
  set(carla_target_postfix "_rss")
  set(ADRSS_INCLUDE_DIRS)
  set(ADRSS_LIBS)

  # 快速C++日志库：spdlog
  find_package(spdlog CONFIG REQUIRED
    HINTS ${ADRSS_INSTALL_DIR}/spdlog/)
  get_target_property(spdlog_include_dirs spdlog::spdlog INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(spdlog_file spdlog::spdlog LOCATION)
  foreach(dir ${spdlog_include_dirs})
    file(GLOB spdlog_includes "${dir}/*")
    install(DIRECTORY ${spdlog_includes} DESTINATION include/system)
    list(APPEND ADRSS_INCLUDE_DIRS ${dir})
  endforeach()
  # DESTINATION给出相对路径时，它将相对于 CMAKE_INSTALL_PREFIX 变量（PythonAPI\carla\dependencies）
  install(FILES ${spdlog_file} DESTINATION lib)
  list(APPEND ADRSS_LIBS ${spdlog_file})

  set(proj_include_dir ${ADRSS_INSTALL_DIR}/../../proj-install/include)
  set(proj_lib ${ADRSS_INSTALL_DIR}/../../proj-install/lib/libproj.a)
  install(DIRECTORY ${proj_include_dir} DESTINATION include/system)
  list(APPEND ADRSS_INCLUDE_DIRS ${proj_include_dir})
  install(FILES ${proj_lib} DESTINATION lib)
  list(APPEND ADRSS_LIBS ${proj_lib})

  # 只要 libosm2odr 使用相同的 odrSpiral 接口，复制构建的静态库就足够了
  set(odr_lib ${ADRSS_INSTALL_DIR}/../src/ad-rss-lib/dependencies/map/dependencies/odrSpiral/lib/libodrSpiral.a)
  install(FILES ${odr_lib} DESTINATION lib)
  list(APPEND ADRSS_LIBS ${odr_lib})

  foreach(ad_lib ad_physics ad_rss ad_map_access ad_map_opendrive_reader ad_rss_map_integration)
    set(${ad_lib}_file ${ADRSS_INSTALL_DIR}/${ad_lib}/lib/lib${ad_lib}.a)
    install(FILES ${${ad_lib}_file} DESTINATION lib)
    list(APPEND ADRSS_LIBS ${${ad_lib}_file})

    # 如果可用，请安装相应的 Python 库
    file(GLOB ${ad_lib}_python_libs "${ADRSS_INSTALL_DIR}/${ad_lib}/lib/lib${ad_lib}_python*.a")
    foreach(ad_python_lib ${${ad_lib}_python_libs})
      if (EXISTS ${ad_python_lib})
        install(FILES ${ad_python_lib} DESTINATION lib)
      endif()
    endforeach()

    set(${ad_lib}_include_dir ${ADRSS_INSTALL_DIR}/${ad_lib}/include)
    file(GLOB ${ad_lib}_includes "${${ad_lib}_include_dir}/*")
    install(DIRECTORY ${${ad_lib}_includes} DESTINATION include/system)
    list(APPEND ADRSS_INCLUDE_DIRS ${${ad_lib}_include_dir})
  endforeach()
endif()

# 安装 Recast&Detour 库
install(DIRECTORY "${RECAST_INCLUDE_PATH}/recast" DESTINATION include/system)
file(GLOB libcarla_carla_recastlib "${RECAST_LIB_PATH}/*.*")
install(FILES ${libcarla_carla_recastlib} DESTINATION lib)

#安装 rpclib
install(DIRECTORY "${RPCLIB_INCLUDE_PATH}/rpc" DESTINATION include/system)
file(GLOB libcarla_carla_rpclib "${RPCLIB_LIB_PATH}/*.*")
install(FILES ${libcarla_carla_rpclib} DESTINATION lib)

# 安装 Boost 头文件
# 待办：从 LibCarla.client 的公共接口中移除 Boost
install(DIRECTORY "${BOOST_INCLUDE_PATH}/boost" DESTINATION include/system)
file(GLOB boost_libraries "${BOOST_LIB_PATH}/*.*")
install(FILES ${boost_libraries} DESTINATION lib)

# Windows 需要安装 libpng 以及 zlib。
if (WIN32)
    # 安装 zlib 头文件
    file(GLOB zlib_includes "${ZLIB_INCLUDE_PATH}/*")
    install(FILES ${zlib_includes} DESTINATION include)
    # 安装 zlib 库
    file(GLOB zlib_libraries "${ZLIB_LIB_PATH}/*")
    install(FILES ${zlib_libraries} DESTINATION lib)

    # 安装 libpng 头文件
    file(GLOB pnglib_includes "${LIBPNG_INCLUDE_PATH}/*")
    install(FILES ${pnglib_includes} DESTINATION include)
    # 安装 zlib 库.
    file(GLOB libpng_libraries "${LIBPNG_LIB_PATH}/*")
    install(FILES ${libpng_libraries} DESTINATION lib)
else ()
  # 安装图片库（libpng）
  install(DIRECTORY "${LIBPNG_INCLUDE_PATH}" DESTINATION include/system)
  file(GLOB libcarla_carla_libpnglib "${LIBPNG_LIB_PATH}/*.*")
  install(FILES ${libcarla_carla_libpnglib} DESTINATION lib)
endif (WIN32)


# 添加和libcarla客户端相关的源代码
# 添加 LibCarla 目录（LibCarla/source/carla）下的*.cpp和.h文件
file(GLOB libcarla_carla_sources
    "${libcarla_source_path}/carla/*.cpp"
    "${libcarla_source_path}/carla/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_sources}")
install(FILES ${libcarla_carla_sources} DESTINATION include/carla)

# 添加客户端目录（LibCarla/source/carla/client）下的*.cpp和.h文件
file(GLOB libcarla_carla_client_sources
    "${libcarla_source_path}/carla/client/*.cpp"
    "${libcarla_source_path}/carla/client/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_client_sources}")
install(FILES ${libcarla_carla_client_sources} DESTINATION include/carla/client)

# 添加客户端详细目录（LibCarla/source/carla/client/detail）下的*.cpp和.h文件
file(GLOB libcarla_carla_client_detail_sources
    "${libcarla_source_path}/carla/client/detail/*.cpp"
    "${libcarla_source_path}/carla/client/detail/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_client_detail_sources}")
install(FILES ${libcarla_carla_client_detail_sources} DESTINATION include/carla/client/detail)

# 添加几何（LibCarla/source/carla/geom）相关代码
file(GLOB libcarla_carla_geom_sources
    "${libcarla_source_path}/carla/geom/*.cpp"
    "${libcarla_source_path}/carla/geom/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_geom_sources}")
install(FILES ${libcarla_carla_geom_sources} DESTINATION include/carla/geom)

# 添加图像（LibCarla/source/carla/image）相关代码
file(GLOB libcarla_carla_image_sources
    "${libcarla_source_path}/carla/image/*.cpp"
    "${libcarla_source_path}/carla/image/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_image_sources}")
install(FILES ${libcarla_carla_image_sources} DESTINATION include/carla/image)

# 添加导航（LibCarla/source/carla/nav）相关代码
file(GLOB libcarla_carla_nav_sources
    "${libcarla_source_path}/carla/nav/*.cpp"
    "${libcarla_source_path}/carla/nav/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_nav_sources}")
install(FILES ${libcarla_carla_nav_sources} DESTINATION include/carla/nav)

# 添加开源路网（LibCarla/source/carla/opendrive/）相关代码
file(GLOB libcarla_carla_opendrive_sources
    "${libcarla_source_path}/carla/opendrive/*.cpp"
    "${libcarla_source_path}/carla/opendrive/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_opendrive_sources}")
install(FILES ${libcarla_carla_opendrive_sources} DESTINATION include/carla/opendrive)

# 添加开源路网解析（LibCarla/source/carla/parser/）相关代码
file(GLOB libcarla_carla_opendrive_parser_sources
    "${libcarla_source_path}/carla/opendrive/parser/*.cpp"
    "${libcarla_source_path}/carla/opendrive/parser/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_opendrive_parser_sources}")
install(FILES ${libcarla_carla_opendrive_parser_sources} DESTINATION include/carla/opendrive/parser)

# 添加点云（LibCarla/source/carla/pointcloud/）相关代码
file(GLOB libcarla_carla_pointcloud_sources
    "${libcarla_source_path}/carla/pointcloud/*.cpp"
    "${libcarla_source_path}/carla/pointcloud/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_pointcloud_sources}")
install(FILES ${libcarla_carla_pointcloud_sources} DESTINATION include/carla/pointcloud)

# 添加性能分析器（LibCarla/source/carla/profiler/）的头文件
file(GLOB libcarla_carla_profiler_headers
    "${libcarla_source_path}/carla/profiler/*.h")
install(FILES ${libcarla_carla_profiler_headers} DESTINATION include/carla/profiler)

# 添加道路（LibCarla/source/carla/road/）相关代码
file(GLOB libcarla_carla_road_sources
    "${libcarla_source_path}/carla/road/*.cpp"
    "${libcarla_source_path}/carla/road/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_road_sources}")
install(FILES ${libcarla_carla_road_sources} DESTINATION include/carla/road)

# 添加道路元素（LibCarla/source/carla/road/element/）相关代码
file(GLOB libcarla_carla_road_element_sources
    "${libcarla_source_path}/carla/road/element/*.cpp"
    "${libcarla_source_path}/carla/road/element/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_road_element_sources}")
install(FILES ${libcarla_carla_road_element_sources} DESTINATION include/carla/road/element)

# 没有general这个文件夹
file(GLOB libcarla_carla_road_general_sources
    "${libcarla_source_path}/carla/road/general/*.cpp"
    "${libcarla_source_path}/carla/road/general/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_road_general_sources}")
install(FILES ${libcarla_carla_road_general_sources} DESTINATION include/carla/road/general)

# 添加道路对象（LibCarla/source/carla/road/object/）相关代码
file(GLOB libcarla_carla_road_object_sources
    "${libcarla_source_path}/carla/road/object/*.cpp"
    "${libcarla_source_path}/carla/road/object/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_road_object_sources}")
install(FILES ${libcarla_carla_road_object_sources} DESTINATION include/carla/road/object)

# 没有signal这个文件夹
file(GLOB libcarla_carla_road_signal_sources
    "${libcarla_source_path}/carla/road/signal/*.cpp"
    "${libcarla_source_path}/carla/road/signal/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_road_signal_sources}")
install(FILES ${libcarla_carla_road_signal_sources} DESTINATION include/carla/road/signal)

# 添加远程过程调用（LibCarla/source/carla/rpc/）相关代码
file(GLOB libcarla_carla_rpc_sources
    "${libcarla_source_path}/carla/rpc/*.cpp"
    "${libcarla_source_path}/carla/rpc/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_rpc_sources}")
install(FILES ${libcarla_carla_rpc_sources} DESTINATION include/carla/rpc)

# 是否构建责任敏感安全变体（传感器需要调用它，所以放在传感器构建之前）
if (BUILD_RSS_VARIANT)
  # 添加责任敏感安全（LibCarla/source/carla/rss/）相关代码
  file(GLOB libcarla_carla_rss_sources
      "${libcarla_source_path}/carla/rss/*.cpp"
      "${libcarla_source_path}/carla/rss/*.h")
  set(libcarla_sources "${libcarla_sources};${libcarla_carla_rss_sources}")
  install(FILES ${libcarla_carla_rss_sources} DESTINATION include/carla/rss)
endif()

# 添加传感器（LibCarla/source/carla/sensor/）相关代码
file(GLOB libcarla_carla_sensor_sources
    "${libcarla_source_path}/carla/sensor/*.cpp"
    "${libcarla_source_path}/carla/sensor/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_sensor_sources}")
install(FILES ${libcarla_carla_sensor_sources} DESTINATION include/carla/sensor)

# 添加传感器数据（LibCarla/source/carla/sensor/data/）相关代码
file(GLOB libcarla_carla_sensor_data_sources
    "${libcarla_source_path}/carla/sensor/data/*.cpp"
    "${libcarla_source_path}/carla/sensor/data/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_sensor_data_sources}")
install(FILES ${libcarla_carla_sensor_data_sources} DESTINATION include/carla/sensor/data)

# 添加传感器序列化（LibCarla/source/carla/sensor/s11n/）相关代码
file(GLOB libcarla_carla_sensor_s11n_sources
    "${libcarla_source_path}/carla/sensor/s11n/*.cpp"
    "${libcarla_source_path}/carla/sensor/s11n/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_sensor_s11n_sources}")
install(FILES ${libcarla_carla_sensor_s11n_sources} DESTINATION include/carla/sensor/s11n)

# 添加流式传输（LibCarla/source/carla/streaming/）相关代码
file(GLOB libcarla_carla_streaming_sources
    "${libcarla_source_path}/carla/streaming/*.cpp"
    "${libcarla_source_path}/carla/streaming/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_streaming_sources}")
install(FILES ${libcarla_carla_streaming_sources} DESTINATION include/carla/streaming)

# 添加详细流式传输（LibCarla/source/carla/streaming/detail/）相关代码
file(GLOB libcarla_carla_streaming_detail_sources
    "${libcarla_source_path}/carla/streaming/detail/*.cpp"
    "${libcarla_source_path}/carla/streaming/detail/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_streaming_detail_sources}")
install(FILES ${libcarla_carla_streaming_detail_sources} DESTINATION include/carla/streaming/detail)

# 添加详细TCP流式传输（LibCarla/source/carla/streaming/detail/tcp/）相关代码
file(GLOB libcarla_carla_streaming_detail_tcp_sources
    "${libcarla_source_path}/carla/streaming/detail/tcp/*.cpp"
    "${libcarla_source_path}/carla/streaming/detail/tcp/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_streaming_detail_tcp_sources}")
install(FILES ${libcarla_carla_streaming_detail_tcp_sources} DESTINATION include/carla/streaming/detail/tcp)

# 添加低层流式传输（LibCarla/source/carla/streaming/detail/tcp/）相关代码
file(GLOB libcarla_carla_streaming_low_level_sources
    "${libcarla_source_path}/carla/streaming/low_level/*.cpp"
    "${libcarla_source_path}/carla/streaming/low_level/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_streaming_low_level_sources}")
install(FILES ${libcarla_carla_streaming_low_level_sources} DESTINATION include/carla/streaming/low_level)

# 添加多GPU（LibCarla/source/carla/multigpu/）相关代码
file(GLOB libcarla_carla_multigpu_sources
    "${libcarla_source_path}/carla/multigpu/*.cpp"
    "${libcarla_source_path}/carla/multigpu/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_multigpu_sources}")
install(FILES ${libcarla_carla_multigpu_sources} DESTINATION include/carla/multigpu)

# 添加开源道路中计算螺旋线（LibCarla/source/third-party/odrSpiral）相关代码
file(GLOB libcarla_odr_spiral_sources
    "${libcarla_source_thirdparty_path}/odrSpiral/*.cpp"
    "${libcarla_source_thirdparty_path}/odrSpiral/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_odr_spiral_sources}")
install(FILES ${libcarla_odr_spiral_sources} DESTINATION include/odrSpiral)

# 添加多生产者多消费者无锁队列（LibCarla/source/third-party/moodycamel）相关代码
file(GLOB libcarla_moodycamel_sources
    "${libcarla_source_thirdparty_path}/moodycamel/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_moodycamel_sources}")
install(FILES ${libcarla_moodycamel_sources} DESTINATION include/moodycamel)

# 添加轻量级的 C++ XML 处理库（LibCarla/source/third-party/pugixml）相关代码
file(GLOB libcarla_pugixml_sources
    "${libcarla_source_thirdparty_path}/pugixml/*.cpp"
    "${libcarla_source_thirdparty_path}/pugixml/*.hpp"
    "${libcarla_source_thirdparty_path}/pugixml/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_pugixml_sources}")
install(FILES ${libcarla_pugixml_sources} DESTINATION include/pugixml)

# 添加交通管理器（LibCarla/source/carla/trafficmanager/）相关代码
file(GLOB libcarla_carla_trafficmanager_sources
    "${libcarla_source_path}/carla/trafficmanager/*.cpp"
    "${libcarla_source_path}/carla/trafficmanager/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_trafficmanager_sources}")
install(FILES ${libcarla_carla_trafficmanager_sources} DESTINATION include/carla/trafficmanager)

# 添加机器人操作系统（LibCarla/source/carla/ros2/）相关代码
file(GLOB libcarla_carla_ros2_sources
    "${libcarla_source_path}/carla/ros2/*.cpp"
    "${libcarla_source_path}/carla/ros2/*.h")
set(libcarla_sources "${libcarla_sources};${libcarla_carla_ros2_sources}")
install(FILES ${libcarla_carla_ros2_sources} DESTINATION include/carla/ros2)

# ==============================================================================
# 在相同的构建类型中创建调试和发布的目标。
# ==============================================================================


# 构建发布版
if (LIBCARLA_BUILD_RELEASE)
  add_library(carla_client${carla_target_postfix} STATIC ${libcarla_sources})

  target_include_directories(carla_client${carla_target_postfix} SYSTEM PRIVATE
      "${BOOST_INCLUDE_PATH}"
      "${RPCLIB_INCLUDE_PATH}"
      "${RECAST_INCLUDE_PATH}"
      "${LIBPNG_INCLUDE_PATH}")

  if (BUILD_RSS_VARIANT)
    target_compile_definitions(carla_client${carla_target_postfix} PRIVATE RSS_ENABLED RSS_USE_TBB)
    target_link_libraries(carla_client${carla_target_postfix} ${ADRSS_LIBS} tbb)
    target_include_directories(carla_client${carla_target_postfix} SYSTEM PRIVATE
      ${ADRSS_INCLUDE_DIRS})
  endif()

  install(TARGETS carla_client${carla_target_postfix} DESTINATION lib)

  if (WIN32) # @todo 修复在 Windows 上的 PythonAPI 构建漏洞。
    set_target_properties(carla_client${carla_target_postfix} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE}")

    target_link_libraries(carla_client${carla_target_postfix} "${RECAST_LIB_PATH}/Recast.lib")
    target_link_libraries(carla_client${carla_target_postfix} "${RECAST_LIB_PATH}/Detour.lib")
    target_link_libraries(carla_client${carla_target_postfix} "${RECAST_LIB_PATH}/DetourCrowd.lib")
  else ()
    if (NOT DEFINED CMAKE_CXX_FLAGS_RELEASE_CLIENT)
      set(CMAKE_CXX_FLAGS_RELEASE_CLIENT ${CMAKE_CXX_FLAGS_RELEASE})
    endif()

    set_target_properties(carla_client${carla_target_postfix} PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_RELEASE_CLIENT}")

    target_link_libraries(carla_client${carla_target_postfix} "${RECAST_LIB_PATH}/libRecast.a")
    target_link_libraries(carla_client${carla_target_postfix} "${RECAST_LIB_PATH}/libDetour.a")
    target_link_libraries(carla_client${carla_target_postfix} "${RECAST_LIB_PATH}/libDetourCrowd.a")

  endif (WIN32)

endif()

# 构建调试版
if (LIBCARLA_BUILD_DEBUG)

  add_library(carla_client${carla_target_postfix}_debug STATIC ${libcarla_sources})

  target_include_directories(carla_client${carla_target_postfix}_debug SYSTEM PRIVATE
      "${BOOST_INCLUDE_PATH}"
      "${RPCLIB_INCLUDE_PATH}"
      "${RECAST_INCLUDE_PATH}"
      "${LIBPNG_INCLUDE_PATH}")

  if (BUILD_RSS_VARIANT)
    target_compile_definitions(carla_client${carla_target_postfix}_debug PRIVATE RSS_ENABLED RSS_USE_TBB)
    target_link_libraries(carla_client${carla_target_postfix}_debug ${ADRSS_LIBS} tbb)
    target_include_directories(carla_client${carla_target_postfix}_debug SYSTEM PRIVATE
      ${ADRSS_INCLUDE_DIRS})
  endif()

  install(TARGETS carla_client${carla_target_postfix}_debug DESTINATION lib)

  if (WIN32) # @todo Fix PythonAPI build on Windows.
    set_target_properties(carla_client${carla_target_postfix}_debug PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_DEBUG}")

    target_link_libraries(carla_client${carla_target_postfix}_debug "${RECAST_LIB_PATH}/Recast.lib")
    target_link_libraries(carla_client${carla_target_postfix}_debug "${RECAST_LIB_PATH}/Detour.lib")
    target_link_libraries(carla_client${carla_target_postfix}_debug "${RECAST_LIB_PATH}/DetourCrowd.lib")
  else ()
    if (NOT DEFINED CMAKE_CXX_FLAGS_DEBUG_CLIENT)
      set(CMAKE_CXX_FLAGS_DEBUG_CLIENT ${CMAKE_CXX_FLAGS_DEBUG})
    endif()

    set_target_properties(carla_client${carla_target_postfix}_debug PROPERTIES COMPILE_FLAGS "${CMAKE_CXX_FLAGS_DEBUG_CLIENT}")

    target_link_libraries(carla_client${carla_target_postfix}_debug "${RECAST_LIB_PATH}/libRecast.a")
    target_link_libraries(carla_client${carla_target_postfix}_debug "${RECAST_LIB_PATH}/libDetour.a")
    target_link_libraries(carla_client${carla_target_postfix}_debug "${RECAST_LIB_PATH}/libDetourCrowd.a")

  endif (WIN32)

  target_compile_definitions(carla_client${carla_target_postfix}_debug PUBLIC -DBOOST_ASIO_ENABLE_BUFFER_DEBUGGING)

endif()
