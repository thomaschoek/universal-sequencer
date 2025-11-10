# micro-composer

A C++ step sequencer application that aims to provide precise timing, on-the-fly edits and compile-time polymorphism

A work in progress

## Design Philosophy

### Timing accuracy

Prioritize timing accuracy over data accuracy: it is more important that events are scheduled on time than that they reflect the latest edits made by users. So if a user is editing an event in a running sequence, we don't want the scheduler to have to wait for these edits to be complete before playing back the event. Rather, we just want it to load whatever value of the event is soonest available, whether it's the value before or after the edits, and play that back on time.

### On the fly edits
On-the-fly edit event values: we want to be able to edit the queued-up events even as the sequencer is running.
On-the-fly edits to event container: we want on-the-fly push, pop, resize, insert, erase etc. operations the data structure containing the events being sequenced.

To combine these on-the-fly edits with the timing accuracy requirement, we use atomic stores and loads of the time at which the next event is to be scheduled (`Scheduler::t_next_`) in addition to conventional `std::scoped_lock`'s so that any edits made to a running sequencer will not cause interference during the critical time window in which the `Scheduler::runner_` thread needs immediate access to the data structure to maintain timing accuracy. See `Scheduler::await_runner_idle()` (@include/scheduler/scheduler.h, @src/scheduler/scheduler.cpp).

### Generic events

We want this sequencer to be generic; it should be able to schedule a wide variety of data types including user defined classes.

## Build Instructions

### Prerequisites
- C++20 compatible compiler
- see meson.build for full list of dependencies

### Building the Project

- `meson setup build && cd build && meson compile`

### Project Structure

- `include/` - Header files
- `src/` - Source files
- `meson.build` - Meson build configuration

### Testing

- catch2
- `cd build && meson test`

Usage:
- meson setup build
- cd build
- meson compile
