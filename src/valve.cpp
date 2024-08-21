#include "valve.hpp"

using namespace frame;

Valve::Valve(State state) : lock{1}, state{state} {}

Valve::Valve(Valve&& valve) : lock{1}, state{valve.state} {}

auto Valve::peerState() -> State {
    lock.acquire();
    State res = state;
    lock.release();
    return res;
}

bool Valve::tryTurnOn() {
    lock.acquire();
    if (state != open && state != idle) {
        lock.release();
        return false;
    }
    state = ready;
    lock.release();
    return true;
}

bool Valve::tryStartUp() {
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

bool Valve::tryEnable(const Empty& empty) {
    lock.acquire();
    if (state != shut || empty()) {
        lock.release();
        return false;
    }
    state = open;
    lock.release();
    return true;
}

bool Valve::tryDisable(const Empty& empty) {
    lock.acquire();
    if (not empty()) {
        lock.release();
        return false;
    }
    state = shut;
    lock.release();
    return true;
}
