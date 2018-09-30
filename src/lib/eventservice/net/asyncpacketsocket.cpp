/*
*  Copyright 2018 Vzenith guangleihe@vzenith.com. All rights reserved.
*
*  Use of this source code is governed by a BSD-style license
*  that can be found in the LICENSE file in the root of the source
*  tree. An additional intellectual property rights grant can be found
*  in the file PATENTS.  All contributing project authors may
*  be found in the AUTHORS file in the root of the source tree.
*/

#include "eventservice/net/asyncpacketsocket.h"

namespace vzes {

AsyncPacketSocket::AsyncPacketSocket(EventService::Ptr event_service,
                                     AsyncSocket::Ptr socket)
  : async_socket_(socket) {
  async_socket_->SignalSocketErrorEvent.connect(
    this, &AsyncPacketSocket::OnAsyncSocketErrorEvent);
  async_socket_->SignalSocketReadEvent.connect(
    this, &AsyncPacketSocket::OnAsyncSocketReadEvent);
  async_socket_->SignalSocketWriteEvent.connect(
    this, &AsyncPacketSocket::OnAsyncSocketWriteEvent);
  //////////////////////////////////////////////////////////////////////////////
  recv_buff_ = MemBuffer::CreateMemBuffer();
}

AsyncPacketSocket::~AsyncPacketSocket() {
  LOG(L_WARNING) << "Destory async socket";
  SignalClose(0, false);
}

void AsyncPacketSocket::Close() {
  SignalClose(0, false);
}

const SocketAddress AsyncPacketSocket::local_addr() {
  BOOST_ASSERT(async_socket_);
  return async_socket_->GetLocalAddress();
}

const SocketAddress AsyncPacketSocket::remote_addr() {
  BOOST_ASSERT(async_socket_);
  return async_socket_->GetRemoteAddress();
}


std::string AsyncPacketSocket::ip_addr() {
  BOOST_ASSERT(async_socket_);
  return async_socket_->GetRemoteAddress().HostAsURIString();
}

bool  AsyncPacketSocket::IsOpen() const {
  BOOST_ASSERT(async_socket_);
  return async_socket_->IsOpen();
}

bool  AsyncPacketSocket::IsClose() {
  BOOST_ASSERT(async_socket_);
  return async_socket_->IsClose();
}

//Socket::Ptr AsyncPacketSocket::SocketNumber() {
//  BOOST_ASSERT(async_socket_);
//  return async_socket_->SocketNumber();
//}
////////////////////////////////////////////////////////////////////////////////

bool AsyncPacketSocket::AsyncWritePacket(const char *data,
    uint32 size, uint16 flag) {
  if (!async_socket_ || async_socket_->IsClose()) {
    LOG(L_ERROR) << "Socket is closed";
    return false;
  }
  if (PACKET_BODY_SIZE < size) {
    LOG(L_ERROR) << "Write data size big than " << PACKET_BODY_SIZE;
    return false;
  }

  //LOG(L_INFO) << async_socket_->remote_addr().ToString();
  //LOG(L_INFO).write(data, size);

  PacketHeader packet_header;
  packet_header.v         = 'V';
  packet_header.z         = 'Z';
  packet_header.flag      = htons(flag);
  packet_header.data_size = htonl(size);

  MemBuffer::Ptr data_buffer = MemBuffer::CreateMemBuffer();
  data_buffer->WriteBytes((const char *)&packet_header, PACKET_HEADER_SIZE);

  if (size) {
    data_buffer->WriteBytes(data, size);
  }
  async_socket_->AsyncWrite(data_buffer);
  return true;
}

bool AsyncPacketSocket::AsyncWritePacket(MemBuffer::Ptr buffer,
    uint16 flag) {
  PacketHeader packet_header;
  packet_header.v         = 'V';
  packet_header.z         = 'Z';
  packet_header.flag      = htons(flag);
  packet_header.data_size = htonl(buffer->size());

  MemBuffer::Ptr send_buffer = MemBuffer::CreateMemBuffer();
  vzes::Block::Ptr block = vzes::Block::TakeBlock();
  block->WriteBytes((char*)&packet_header, sizeof(PacketHeader));
  send_buffer->AppendBlock(block);
  send_buffer->AppendBuffer(buffer);
  async_socket_->AsyncWrite(send_buffer);
  return true;
}

bool AsyncPacketSocket::AsyncRead() {
  if (!async_socket_ || async_socket_->IsClose()) {
    LOG(L_ERROR) << "Socket is closed";
    return false;
  }
  return async_socket_->AsyncRead();
}

bool AsyncPacketSocket::AnalysisPacket(MemBuffer::Ptr buffer) {
  while (true) {
    uint32 recv_size = recv_buff_->size() + buffer->size();
    if (recv_size < sizeof(PacketHeader)) {
      // Packet未接收完，头部信息不完整，将本次接收的数据存入接收缓存
      recv_buff_->AppendBuffer(buffer);
      break;
    }

    // 读取Packet头
    PacketHeader packet_header;
    if (recv_buff_->size() >= sizeof(PacketHeader)) {
      recv_buff_->CopyBytes(0, (char*)&packet_header, sizeof(PacketHeader));
    } else {
      if (recv_buff_->size() == 0) {
        buffer->CopyBytes(0, (char*)&packet_header, sizeof(PacketHeader));
      } else {
        int head_len = recv_buff_->size();
        recv_buff_->CopyBytes(0, (char*)&packet_header, head_len);
        buffer->CopyBytes(0, (char*)&packet_header + head_len,
                          sizeof(PacketHeader) - head_len);
      }
    }

    packet_header.flag = ntohs(packet_header.flag);
    packet_header.data_size = ntohl(packet_header.data_size);
    if (packet_header.v != 'V' || packet_header.z != 'Z') {
      LOG(L_ERROR) << "Packet format error";
      return false;
    }

    if (packet_header.data_size > (recv_size - PACKET_HEADER_SIZE)) {
      // Packet未接收完，将本次接收的数据存入接收缓存
      recv_buff_->AppendBuffer(buffer);
      break;
    } else {
      // Packet接收完整，处理粘包
      MemBuffer::Ptr usr_buff = MemBuffer::CreateMemBuffer();
      if (recv_buff_->size() > 0) {
        // 读取接收缓存中的数据，即，Packet的前半部分
        usr_buff->AppendBuffer(recv_buff_);
        recv_buff_->Clear();
      }

      uint32 length = usr_buff->size();
      BlocksPtr &blocks = buffer->blocks();
      while (true) {
        // 遍历本次接收数据MemBuffer，组包
        Block::Ptr block = blocks.front();
        length += block->buffer_size;
        if ((length - PACKET_HEADER_SIZE) < packet_header.data_size) {
          // 当前Packet不完整，继续查找下一个Block
          usr_buff->AppendBlock(block);
          blocks.pop_front();
          buffer->ReduceSize(block->buffer_size);
          continue;
        } else {
          // 当前Packet完整，解析当前Block
          if ((length - PACKET_HEADER_SIZE) == packet_header.data_size) {
            // 当前Block中所有数据属于当前的Packet
            usr_buff->AppendBlock(block);
            blocks.pop_front();
            buffer->ReduceSize(block->buffer_size);
          } else {
            // 当前Block中只有部分数据属于当前的Packet
            int tail_len = packet_header.data_size -
                           (length - PACKET_HEADER_SIZE - block->buffer_size);
            vzes::Block::Ptr tail_block = vzes::Block::TakeBlock();
            block->ReadBytes((char*)tail_block->buffer, tail_len);
            tail_block->buffer_size = tail_len;
            usr_buff->AppendBlock(tail_block);
            buffer->ReduceSize(tail_len);
          }

          // 当前Packet组包完成，去除“VZ”头部，通知用户
          usr_buff->ReadBytes(NULL, sizeof(PacketHeader));
          SignalPacketEvent(shared_from_this(), usr_buff, packet_header.flag);
          // SignalPacketEvent 有可能会关闭整个AsyncPacketSocket
          if (!async_socket_) {
            return true;
          }
          break;
        }
      }
    }
  }

  return true;
}

void AsyncPacketSocket::SignalClose(int error_code, bool is_signal) {
  if (async_socket_) {
    async_socket_->Close();
    async_socket_.reset();
  }
  if (!SignalPacketError.is_empty()) {
    if (is_signal) {
      SignalPacketError(shared_from_this(), error_code);
    }
    SignalPacketError.disconnect_all();
  }
  if (!SignalPacketEvent.is_empty()) {
    SignalPacketEvent.disconnect_all();
  }
  if (!SignalPacketWrite.is_empty()) {
    SignalPacketWrite.disconnect_all();
  }
}

void AsyncPacketSocket::LiveSignalClose(int error_code, bool is_signal) {
  AsyncPacketSocket::Ptr async_packet_socket = shared_from_this();
  SignalClose(error_code, is_signal);
}

void AsyncPacketSocket::OnAsyncSocketWriteEvent(AsyncSocket::Ptr socket) {
  SignalPacketWrite(shared_from_this());
}

void AsyncPacketSocket::OnAsyncSocketReadEvent(AsyncSocket::Ptr socket,
    MemBuffer::Ptr data_buffer) {
  AsyncPacketSocket::Ptr live_this = shared_from_this();
  if (!async_socket_ || async_socket_->IsClose()) {
    LOG(L_ERROR) << "Socket is closed";
    return;
  }
  if (!AnalysisPacket(data_buffer)) {
    LiveSignalClose(1, true);
  } else {
    AsyncRead();
  }
}

void AsyncPacketSocket::OnAsyncSocketErrorEvent(AsyncSocket::Ptr socket,
    int error_code) {
  LiveSignalClose(error_code, true);
}

}  // namespace stp

