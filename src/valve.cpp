#include "valve.hpp"

using namespace frame;

Valve::Valve(State state) : lock{1}, state{state} {}

auto Valve::peerState() -> State {
    lock.acquire();
    State res = state;
    lock.release();
    return res;
}

bool Valve::tryTurnOnWithAgain() {
    lock.acquire();
    if (state == shut) {
        lock.release();
        return false;
    }
    if (state == ready || state == busy) {
        state = ready;
        lock.release();
        return false;
    }
    state = ready;
    lock.release();
    return true;
}

bool Valve::tryStartUpWithAgain() {
    lock.acquire();
    if (state == shut) {
        lock.release();
        return false;
    }
    if (state != ready) {
        state = idle;
        lock.release();
        return false;
    }
    state = busy;
    lock.release();
    return true;
}

bool Valve::tryEnable(const BufferIsEmpty& empty) {
    lock.acquire();
    if (state != shut || empty()) {
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
