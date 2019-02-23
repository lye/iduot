# TODO

## isa

 * Clean up task/segment terminology because it's confused right now.
   - Concurrency/hardware sections in particular.
 * Reorder some sections to make more sense (put instruction details on bottom?)
 * Define init/hw signals or other boot process for device enumeration.
 * Move segment headers to trailers to make semantics cleaner (everything starts at 0).

## asm

 * Clean up error handling. EDOOFUS is terrible.
 * Probably add a bunch of 'error' constructions to the grammar to allow errors through, but add error state to the `program_t`.
 * Figure out how to get the token name for an unexpected token.
 * Write a README.

## vm

 * The entire thing.
