# TODO

## isa

 * Define fork semantics.
 * Clean up task/segment terminology because it's confused right now.
 * Define init/hw signals or other boot process for device enumeration.
 * Does load/store stack actually benefit from relative addressing?
 * Move segment headers to trailers to make semantics cleaner (everything starts at 0).
 * Explicitly figure out which direction the stack is growing.
 * Is there a good sync primitive that can be thrown in there? `signal ; wait` kind of sucks.

## asm

 * Write some more abstractions for representing a list of `inst_t`'s.
 * Write the `uint16_t` -> `uint12_t` packing stuff.
 * Write the actual parser hurrr.

## vm

 * The entire thing.
