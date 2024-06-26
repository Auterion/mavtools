/****************************************************************************
 *
 * Copyright (c) 2024, libmav development team
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in
 *    the documentation and/or other materials provided with the
 *    distribution.
 * 3. Neither the name libmav nor the names of its contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS
 * OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 ****************************************************************************/

#ifndef MAVTOOLS_DUMMYINTERFACE_H
#define MAVTOOLS_DUMMYINTERFACE_H

#include "mav/Network.h"

class DummyInterface : public mav::NetworkInterface {
private:
    static constexpr int MAX_RECEIVE_QUEUE_WATERMARK = 1024 * 64; // 64 KiB
    std::string receive_queue;
    std::mutex receive_queue_mutex;
    mutable std::condition_variable receive_queue_cv;
    std::string send_sponge;
    mutable std::atomic_bool should_interrupt{false};

public:

    void stop() const {
        should_interrupt.store(true);
        receive_queue_cv.notify_all();
    }

    void close() override {
        stop();
    }

    void send(const uint8_t *data, uint32_t size, mav::ConnectionPartner) override {
        send_sponge.append(reinterpret_cast<const char *>(data), size);
    }

    void addToReceiveQueue(const std::string &data) {
        std::unique_lock<std::mutex> lock(receive_queue_mutex);
        // wait, if the queue is full
        receive_queue_cv.wait(lock, [this] {
            return receive_queue.size() <= MAX_RECEIVE_QUEUE_WATERMARK || should_interrupt.load();
        });

        receive_queue.append(data);
        lock.unlock();
        receive_queue_cv.notify_all();
    }

    void waitUntilReceiveQueueEmpty() {
        std::unique_lock<std::mutex> lock(receive_queue_mutex);
        if (receive_queue.empty()) {
            return;
        }
        receive_queue_cv.wait(lock, [this] {
            return receive_queue.empty();
        });
    }

    mav::ConnectionPartner receive(uint8_t *destination, uint32_t size) override {
        std::unique_lock<std::mutex> lock(receive_queue_mutex);
        if (receive_queue.size() < size) {
            receive_queue_cv.wait(lock, [this, size] {
                return receive_queue.size() >= size || should_interrupt.load();
            });
        }
        if (should_interrupt.load()) {
            throw mav::NetworkInterfaceInterrupt();
        }
        std::copy(receive_queue.begin(), receive_queue.begin() + size, destination);
        receive_queue.erase(0, size);

        if (receive_queue.size() < MAX_RECEIVE_QUEUE_WATERMARK / 2) {
            receive_queue_cv.notify_all();
        }

        return mav::ConnectionPartner{};
    }
};
#endif //MAVTOOLS_DUMMYINTERFACE_H
