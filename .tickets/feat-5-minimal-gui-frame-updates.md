What if the ONLY addition we make to the `handler_` function passed to the
  Atomic_sequencers is simply that they will set an atomic dirty flag just before, just
  after, or concurrently (usign std::async) with triggering the synth? We could do something
  like have the @include/ui/ui.h `User_interface` class set the handler for all sequencers to
  something like this: `[&this](Event_t&& evt){this->notify_of_step_increment();
  existing_handler(std::forward<Event_t>evt);}` where notify_of_step_increment is an inline
  function that does nothing but set a little atomic flag.
