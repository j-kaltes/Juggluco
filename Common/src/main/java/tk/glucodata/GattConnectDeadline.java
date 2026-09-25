package tk.glucodata;

import java.util.function.Consumer;

/** A cancellable deadline tied to one GATT attempt, serialized with its owner. */
final class GattConnectDeadline<T> {
    interface Cancellation { void cancel(); }
    interface Scheduler { Cancellation schedule(Runnable task, long delayMillis); }

    private final Object lock;
    private final Scheduler scheduler;
    private final Consumer<T> expired;
    private Object ticket;
    private T current;
    private Cancellation pending;

    GattConnectDeadline(Object lock, Scheduler scheduler, Consumer<T> expired) {
        this.lock = lock;
        this.scheduler = scheduler;
        this.expired = expired;
    }

    void arm(T attempt, long delayMillis) {
        synchronized (lock) {
            cancel();
            current = attempt;
            final Object next = new Object();
            ticket = next;
            pending = scheduler.schedule(() -> {
                synchronized (lock) {
                    // Cancellation may race with a task already dispatched by the executor.
                    if (ticket != next) return;
                    ticket = null;
                    current = null;
                    pending = null;
                    expired.accept(attempt);
                }
            }, delayMillis);
        }
    }

    void completed(T attempt) {
        synchronized (lock) {
            if (ticket != null && current == attempt) cancel();
        }
    }

    void cancel() {
        synchronized (lock) {
            ticket = null;
            current = null;
            if (pending != null) {
                pending.cancel();
                pending = null;
            }
        }
    }
}
