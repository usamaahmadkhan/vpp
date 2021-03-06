/*
 * Copyright (c) 2017 Cisco and/or its affiliates.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "vom/hw.hpp"
#include "vom/hw_cmds.hpp"
#include "vom/logger.hpp"

namespace VOM {
HW::cmd_q::cmd_q()
  : m_enabled(true)
  , m_connected(false)
  , m_conn()
{
}

HW::cmd_q::~cmd_q()
{
}

HW::cmd_q&
HW::cmd_q::operator=(const HW::cmd_q& f)
{
  return (*this);
}

/**
 * Run the connect/dispatch thread.
 */
void
HW::cmd_q::rx_run()
{
  while (m_connected) {
    m_conn.ctx().dispatch();
  }
}

void
HW::cmd_q::enqueue(cmd* c)
{
  std::shared_ptr<cmd> sp(c);

  m_queue.push_back(sp);
}

void
HW::cmd_q::enqueue(std::shared_ptr<cmd> c)
{
  m_queue.push_back(c);
}

void
HW::cmd_q::enqueue(std::queue<cmd*>& cmds)
{
  while (cmds.size()) {
    std::shared_ptr<cmd> sp(cmds.front());

    m_queue.push_back(sp);
    cmds.pop();
  }
}

bool
HW::cmd_q::connect()
{
  if (m_connected)
    return m_connected;

  if (0 == m_conn.connect()) {
    m_connected = true;
    m_rx_thread.reset(new std::thread(&HW::cmd_q::rx_run, this));
  }
  return (m_connected);
}

void
HW::cmd_q::disconnect()
{

  if (!m_connected)
    return;

  m_connected = false;

  if (m_rx_thread && m_rx_thread->joinable()) {
    m_rx_thread->join();
  }

  m_conn.disconnect();
}

void
HW::cmd_q::enable()
{
  m_enabled = true;
}

void
HW::cmd_q::disable()
{
  m_enabled = false;
}

rc_t
HW::cmd_q::write()
{
  rc_t rc = rc_t::OK;

  /*
   * The queue is enabled, Execute each command in the queue.
   * If one execution fails, abort the rest
   */
  auto it = m_queue.begin();

  while (it != m_queue.end()) {
    std::shared_ptr<cmd> c = *it;

    VOM_LOG(log_level_t::DEBUG) << *c;

    if (m_enabled) {
      /*
       * before we issue the command we must move it to the pending
       * store
       * ince a async event can be recieved before the command
       * completes
       */
      rc = c->issue(m_conn);

      if (rc_t::OK == rc) {
        /*
         * move to the next
         */
      } else {
        /*
         * barf out without issuing the rest
         */
        VOM_LOG(log_level_t::ERROR) << "Failed to execute: " << c->to_string();
        break;
      }
    } else {
      /*
       * The HW is disabled, so set each command as succeeded
       */
      c->succeeded();
    }

    ++it;
  }

  /*
   * erase all objects in the queue
   */
  m_queue.erase(m_queue.begin(), m_queue.end());

  return (rc);
}

/*
 * The single Command Queue
 */
HW::cmd_q* HW::m_cmdQ;
HW::item<bool> HW::m_poll_state;

/**
 * Initialse the connection to VPP
 */
void
HW::init(HW::cmd_q* f)
{
  m_cmdQ = f;
}

/**
 * Initialse the connection to VPP
 */
void
HW::init()
{
  m_cmdQ = new cmd_q();
}

void
HW::enqueue(cmd* cmd)
{
  m_cmdQ->enqueue(cmd);
}

void
HW::enqueue(std::shared_ptr<cmd> cmd)
{
  m_cmdQ->enqueue(cmd);
}

void
HW::enqueue(std::queue<cmd*>& cmds)
{
  m_cmdQ->enqueue(cmds);
}

bool
HW::connect()
{
  return m_cmdQ->connect();
}

void
HW::disconnect()
{
  m_cmdQ->disconnect();
}

void
HW::enable()
{
  m_cmdQ->enable();
}

void
HW::disable()
{
  m_cmdQ->disable();
}

rc_t
HW::write()
{
  return (m_cmdQ->write());
}

bool
HW::poll()
{
  std::shared_ptr<cmd> poll(new hw_cmds::poll(m_poll_state));

  HW::enqueue(poll);
  HW::write();

  return (m_poll_state);
}

template <>
std::string
HW::item<bool>::to_string() const
{
  std::ostringstream os;

  os << "hw-item:["
     << "rc:" << item_rc.to_string() << " data:" << item_data << "]";
  return (os.str());
}

template <>
std::string
HW::item<unsigned int>::to_string() const
{
  std::ostringstream os;

  os << "hw-item:["
     << "rc:" << item_rc.to_string() << " data:" << item_data << "]";
  return (os.str());
}
}

/*
 * fd.io coding-style-patch-verification: ON
 *
 * Local Variables:
 * eval: (c-set-style "mozilla")
 * End:
 */
