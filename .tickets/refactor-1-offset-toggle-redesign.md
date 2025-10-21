We need to think about offset and toggling steps.

How to design it?

Do we make 'toggled' or 'active' a property of the basic Step class?

Do we keep a separate list of 'step statuses' indicating the toggled and offset status for each step?

So the thing is we want to keep the basic step class as minimal and non-arbitrary as possible.

So it at least needs a duration. An offset is something that would only apply to certain kinds of events so we should probably remove that from the basic step.

But how about toggled?

Where do we let the toggling logic happen? At the atomic sequencer backend level?

Or more frontend where it actually is used
I mean,
being able to toggle steps is a nice feature for a user in the context of a music sequencer
and undoubtedly i nmany other contexts, but is it general enough to make toggled a property of the basic step?

What are the alternatives?
The alternatives are keeping a separate toggled status list on the atomic sequencer, which is tantamount to just adding it on the step,

best thing maybe is keep a vector `time_signature_` on Atomic_sequencer.

Read durations from that. Then keep a separate vector `events_`

That will contain the parameters that will be sent to output.
