// Copyright (c) 2022 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#include "carla/multigpu/router.h"

#include "carla/multigpu/listener.h"
#include "carla/streaming/EndPoint.h"

namespace carla {
namespace multigpu {

Router::Router(void) :
  _next(0) { }

Router::~Router() {
  Stop();
}

void Router::Stop() {
  ClearSessions();    // 清除所有活动的会话。
  _listener->Stop();  // 停止监听器，防止接受新连接
  _listener.reset();  // 释放监听器对象的内存。
  _pool.Stop();       // 停止相关的线程池以释放资源。
}

Router::Router(uint16_t port) :
  _next(0) {

  // 创建一个TCP端点，监听所有网络接口（0.0.0.0）上的指定端口
  _endpoint = boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("0.0.0.0"), port);
  // 初始化_listener为指向Listener对象的共享指针，用于处理传入连接。
  _listener = std::make_shared<carla::multigpu::Listener>(_pool.io_context(), _endpoint);
}

void Router::SetCallbacks() {
  // 准备服务器
  std::weak_ptr<Router> weak = shared_from_this();

  carla::multigpu::Listener::callback_function_type on_open = [=](std::shared_ptr<carla::multigpu::Primary> session) {
    auto self = weak.lock();
    if (!self) return;
    self->ConnectSession(session);
  };

  carla::multigpu::Listener::callback_function_type on_close = [=](std::shared_ptr<carla::multigpu::Primary> session) {
    auto self = weak.lock();
    if (!self) return;
    self->DisconnectSession(session);
  };

  carla::multigpu::Listener::callback_function_type_response on_response =
    [=](std::shared_ptr<carla::multigpu::Primary> session, carla::Buffer buffer) {
      auto self = weak.lock();
      if (!self) return;
      std::lock_guard<std::mutex> lock(self->_mutex);
      auto prom =self-> _promises.find(session.get());
      if (prom != self->_promises.end()) {
        log_info("Got data from secondary (with promise): ", buffer.size());
        prom->second->set_value({session, std::move(buffer)});
        self->_promises.erase(prom);
      } else {
        log_info("Got data from secondary (without promise): ", buffer.size());
      }
    };

  _commander.set_router(shared_from_this());

  _listener->Listen(on_open, on_close, on_response);
  log_info("Listening at ", _endpoint);
}

void Router::SetNewConnectionCallback(std::function<void(void)> func)
{
  _callback = func;
}

void Router::AsyncRun(size_t worker_threads) {
  _pool.AsyncRun(worker_threads);
}

boost::asio::ip::tcp::endpoint Router::GetLocalEndpoint() const {
  return _endpoint;
}

void Router::ConnectSession(std::shared_ptr<Primary> session) {
  DEBUG_ASSERT(session != nullptr);
  std::lock_guard<std::mutex> lock(_mutex);
  _sessions.emplace_back(std::move(session));
  log_info("Connected secondary servers:", _sessions.size());
  // 对新连接运行外部回调
  if (_callback)
    _callback();
}

void Router::DisconnectSession(std::shared_ptr<Primary> session) {
  DEBUG_ASSERT(session != nullptr);
  std::lock_guard<std::mutex> lock(_mutex);
  if (_sessions.size() == 0) return;
  _sessions.erase(
      std::remove(_sessions.begin(), _sessions.end(), session),
      _sessions.end());
  log_info("Connected secondary servers:", _sessions.size());
}

void Router::ClearSessions() {
  std::lock_guard<std::mutex> lock(_mutex);
  _sessions.clear();
  log_info("Disconnecting all secondary servers");
}

void Router::Write(MultiGPUCommand id, Buffer &&buffer) {
  // 定义命令头
  CommandHeader header;
  header.id = id;
  header.size = buffer.size();
  Buffer buf_header((uint8_t *) &header, sizeof(header));

  auto view_header = carla::BufferView::CreateFrom(std::move(buf_header));
  auto view_data = carla::BufferView::CreateFrom(std::move(buffer));
  auto message = Primary::MakeMessage(view_header, view_data);

  // 写入多个服务器
  std::lock_guard<std::mutex> lock(_mutex);
  for (auto &s : _sessions) {
    if (s != nullptr) {
      s->Write(message);
    }
  }
}

std::future<SessionInfo> Router::WriteToNext(MultiGPUCommand id, Buffer &&buffer) {
  // 定义命令头
  CommandHeader header;
  header.id = id;
  header.size = buffer.size();
  Buffer buf_header((uint8_t *) &header, sizeof(header));

  auto view_header = carla::BufferView::CreateFrom(std::move(buf_header));
  auto view_data = carla::BufferView::CreateFrom(std::move(buffer));
  auto message = Primary::MakeMessage(view_header, view_data);

  // 为可能的答案创建承诺自动响应
  auto response = std::make_shared<std::promise<SessionInfo>>();

  // 只写特定服务器
  std::lock_guard<std::mutex> lock(_mutex);
  if (_next >= _sessions.size()) {
    _next = 0;
  }
  if (_next < _sessions.size()) {
    // std::cout << "Sending to session " << _next << std::endl;
    auto s = _sessions[_next];
    if (s != nullptr) {
      _promises[s.get()] = response;
      std::cout << "Updated promise into map: " << _promises.size() << std::endl;
      s->Write(message);
    }
  }
  ++_next;
  return response->get_future();
}

std::future<SessionInfo> Router::WriteToOne(std::weak_ptr<Primary> server, MultiGPUCommand id, Buffer &&buffer) {
  // 定义命令头
  CommandHeader header;
  header.id = id;
  header.size = buffer.size();
  Buffer buf_header((uint8_t *) &header, sizeof(header));

  auto view_header = carla::BufferView::CreateFrom(std::move(buf_header));
  auto view_data = carla::BufferView::CreateFrom(std::move(buffer));
  auto message = Primary::MakeMessage(view_header, view_data);

  // 为可能的答案创建承诺自动响应

  auto response = std::make_shared<std::promise<SessionInfo>>();

  // 只写特定服务器

  std::lock_guard<std::mutex> lock(_mutex);
  auto s = server.lock();
  if (s) {
    _promises[s.get()] = response;
    s->Write(message);
  }
  return response->get_future();
}

std::weak_ptr<Primary> Router::GetNextServer() {
  std::lock_guard<std::mutex> lock(_mutex);
  if (_next >= _sessions.size()) {
    _next = 0;
  }
  if (_next < _sessions.size()) {
    return std::weak_ptr<Primary>(_sessions[_next]);
  } else {
    return std::weak_ptr<Primary>();
  }
}

} // 名称空间 multigpu
} // 名称空间 carla
