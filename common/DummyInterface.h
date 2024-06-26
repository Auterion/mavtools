
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
