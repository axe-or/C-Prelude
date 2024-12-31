#include "prelude.hpp"

//// Spinlock //////////////////////////////////////////////////////////////////
void Spinlock::acquire(){
	for(;;){
		if(!atomic_exchange(&_state, SPINLOCK_LOCKED, Memory_Order::Acquire)){
			break;
		}
		/* Busy wait while locked */
		while(atomic_load(&_state, Memory_Order::Relaxed));
	}
}

bool Spinlock::try_acquire(){
    return !atomic_exchange(&_state, SPINLOCK_LOCKED, Memory_Order::Acquire);
}

void Spinlock::release(){
	atomic_store(&_state, SPINLOCK_UNLOCKED);
}
