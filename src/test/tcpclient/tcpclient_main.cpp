/*
 * vzsdk
 * Copyright 2013 - 2018, Vzenith Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <iostream>
#include "eventservice/net/eventservice.h"
#include "eventservice/base/logging.h"
#include "filecache/cache/cacheclient.h"


#ifdef WIN32
#define FC_FILE_PATH   "D:/kvdb/filecache_test_1.txt"
#else
#define FC_FILE_PATH   "/tmp/app/exec/filecache_test_1.txt"
#endif

const char NOT_FOUND[] =
  "HTTP/1.0 200 OK\r\n"
  "Server:Apache Tomcat/5.0.12\r\n"
  "Content-Type:text/html\r\n\r\n"
  "<html>"
  "<head><title>Not Found</title></head>"
  "<body><h1>"
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
  "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb"
  "ccccccccccccccccccccccccccccccccccccccccccccccccccccccccccc"
  "ddddddddddddddddddddddddddddddddddddddddddddddddddddddddddd"
  "eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee"
  "fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff"
  "ggggggggggggggggggggggggggggggggggggggggggggggggggggggggggg"
  "hhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhhh"
  "iiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii"
  "jjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjjj"
  "kkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkkk"
  "lllllllllllllllllllllllllllllllllllllllllllllllllllllllllll"
  "mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmm"
  "nnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnnn"
  "ooooooooooooooooooooooooooooooooooooooooooooooooooooooooooo"
  "404 Not Found</h1></body>"
  "</html>";

class TcpClient : public vzes::MessageHandler,
  public boost::noncopyable,
  public boost::enable_shared_from_this<TcpClient>,
  public sigslot::has_slots<> {
 public:
  TcpClient(vzes::EventService::Ptr event_service)
    : event_service_(event_service),
      pack_cnt_(0) {
  }
  bool Start() {
    ASSERT_RETURN_FAILURE(async_client_, false);

    cache_client_ = cache::CacheClient::CreateCacheClient();
    async_client_ = event_service_->CreateAsyncConnect();

    vzes::SocketAddress address("127.0.0.1", 5544);
    async_client_->SignalServerConnected.connect(
      this, &TcpClient::OnConnectEvent);

    return async_client_->Connect(address, 10000);
  }

 private:
  void OnConnectEvent(vzes::AsyncConnecter::Ptr client,
                      vzes::Socket::Ptr s,
                      int err) {

    if (err != 0 || !s) {
      LOG(L_ERROR) << "socket connect remote failed";
      return ;
    }

    LOG(L_INFO) << "socket connect romote successed " << err;
    vzes::AsyncSocket::Ptr socket = event_service_->CreateAsyncSocket(s);
    socket->SignalSocketWriteEvent.connect(
      this, &TcpClient::OnSocketWriteComplete);
    socket->SignalSocketReadEvent.connect(
      this, &TcpClient::OnSocketReadComplete);
    socket->SignalSocketErrorEvent.connect(
      this, &TcpClient::OnSocketErrorEvent);

    async_sockets_.push_back(socket);

    socket->SetEncodeType(vzes::PKT_ENCODE_BASE64);
    cache_client_->Write(FC_FILE_PATH, NOT_FOUND, strlen(NOT_FOUND));

    // 读取文件
    vzes::MemBuffer::Ptr read_buffer;
    read_buffer = cache_client_->Read(FC_FILE_PATH);
    vzes::BlocksPtr &blocks = read_buffer->blocks();
    read_buffer->EnableEncode(true);

    // 新建一个MenBuffer结构
    vzes::MemBuffer::Ptr data_new = vzes::MemBuffer::CreateMemBuffer();
    // 申请头Block，并填充信息
    vzes::Block::Ptr block_head = vzes::Block::TakeBlock();
    block_head->WriteBytes("xxxxxxxxxx", 10);
    data_new->AppendBlock(block_head);

    // Append原始数据MemBuffer到新创建的MenBuffer
    data_new->AppendBuffer(read_buffer);

    // 申请尾Block，并填充信息
    vzes::Block::Ptr block_tail = vzes::Block::TakeBlock();
    block_tail->WriteBytes("xxxxxxxxxx", 10);
    data_new->AppendBlock(block_tail);

    bool res = socket->AsyncWrite(data_new);
    if (!res) {
      LOG(L_ERROR) << "send data failed";
    }

    socket->AsyncRead();
    event_service_->PostDelayed(5*1000, this, 0);
  }

  void OnSocketWriteComplete(vzes::AsyncSocket::Ptr async_socket) {
    //LOG(L_INFO) << "send data done";
  }

  void OnSocketReadComplete(vzes::AsyncSocket::Ptr async_socket,
                            vzes::MemBuffer::Ptr data) {
    //LOG(L_INFO) << "received data, size = " << data->size();
    // LOG(L_INFO).write(data->buffer_, data->buffer_size_);
    pack_cnt_ ++;
    async_socket->AsyncWrite(NOT_FOUND, strlen(NOT_FOUND));
    async_socket->AsyncRead();
  }

  void OnSocketErrorEvent(vzes::AsyncSocket::Ptr async_socket,
                          int err) {
    async_socket->Close();
    async_sockets_.remove(async_socket);
  }

  virtual void OnMessage(vzes::Message *msg) {
    int count = pack_cnt_;
    pack_cnt_ = 0;
    event_service_->PostDelayed(5*1000, this, 0);
    LOG(L_INFO) << "received packets count = " << count
                << ", total size = " << (strlen(NOT_FOUND) * count)
                << ", speed = " << ((8 * strlen(NOT_FOUND) * (count/5))/1024)
                << "kbps";
  }

 private:
  int                                        pack_cnt_;
  vzes::EventService::Ptr                    event_service_;
  vzes::AsyncConnecter::Ptr                  async_client_;
  cache::CacheClient::Ptr                    cache_client_;
  typedef std::list<vzes::AsyncSocket::Ptr>  AsyncSockets;
  AsyncSockets                               async_sockets_;
};

int main(void) {
  // Initialize the logging system
  vzes::LogMessage::LogTimestamps(true);
  vzes::LogMessage::LogContext(vzes::LS_INFO);
  vzes::LogMessage::LogThreads(true);

  vzes::EventService::Ptr event_service =
    vzes::EventService::CreateCurrentEventService("TcpClient");
  TcpClient tcp_client(event_service);
  tcp_client.Start();
  event_service->Run();

  return EXIT_SUCCESS;
}
