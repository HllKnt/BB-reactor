#include "valve.hpp"

using namespace frame;

Valve::Valve(State state) : lock{1}, state{state} {}

bool Valve::tryTurnOn() {
    lock.acquire();
    if (state == shut || state == ready) {
        lock.release();
        return false;
    }
    if (state == busy) {
        state = ready;
        lock.release();
        return false;
    }
    state = ready;
    lock.release();
    return true;
}

bool Valve::tryTurnOff() {
    lock.acquire();
    if (state == open || state == ready) {
        state = busy;
        lock.release();
        return false;
    }
    if (state == shut) {
        lock.release();
        return true;
    }
    state = idle;
    lock.release();
    return true;
}

bool Valve::tryEnable(const BufferIsEmpty& empty) {
    lock.acquire();
    if (empty()) {
        lock.release();
        return false;
    }
    if (state != idle || state != shut) {
        state = ready;
        lock.release();
        return false;
    }
    state = open;
    lock.release();
    return true;
}

bool Valve::tryDisable(const BufferIsEmpty& empty) {
    lock.acquire();
    if (not empty()) {
        lock.release();
        return false;
    }
    state = shut;
    lock.release();
    return true;
}

auto Valve::peerState() -> State {
    lock.acquire();
    State res = state;
    lock.release();
    return res;
}
