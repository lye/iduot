# ISA

 * Register-based.
 * Word size is 12-bits. Words are smallest addressable unit.
 * Weird memory protection; not enough room for conventional MMU semantics.
 * Use segment registers to expand addressible range.
 * Signed values are silly.
 * No floating point operations.
 * Want two active segments: one for stack, one for general-purpose memory.
 * Let's also have concurrency because why not!
 * I/O handled via combination interrupts + mmaping to segment.
 * Segment 0 to serve for global hardware state/IDT/etc.

## Registers

Registers are identified by instructions with 4 bits; that gives 16 registers.

 * program counter; used for next operation.
 * stack pointer; expected to be used for stack operations.
 * stack segment; effectively current thread id.
 * memory segment; modifies load/write address src/destination.
 * carry register; special register set/read by various instructions.
 * zero register; always 0, cannot be written to.
 * 8 general purpose registers: a, b, c, d, e, f, g, h
 * 2 reserved registers

## Instructions

Each instruction is encoded in a 12-bit word. Most of the instructions are an opcode followed by two 4-bit register identifiers, e.g.

| Opcode   | Dest register | Source Register |
|----------|---------------|-----------------|
| bits 0-4 | bits 5-8      | bits 9-12       |

The opcode table is as follows:

| Opcode    | Instruction    | Operands                     | Notes                                                |
|-----------|----------------|------------------------------|------------------------------------------------------|
| `0000`    | load immediate | register, 4-bit immediate    | `register = immediate`, immediate cannot be 0        |
| `0001`    | load address   | 2x register                  | `dest = *src` where src is on current mseg           |
| `0010`    | write address  | 2x register                  | `*dest = src` where dest is on current mseg          |
| `0011`    | lshift         | 2x register                  | `dest = dest << src`                                 |
| `0100`    | rshift         | 2x register                  | `dest = dest >> src`                                 |
| `0101`    | add            | 2x register, writes carry    | `dest = dest + src`                                  |
| `0110`    | sub            | 2x register, writes carry    | `dest = dest - src`                                  |
| `0111`    | mul            | 2x register, writes carry    | `dest = dest * src`, `carry = 1` if overflow         |
| `1000`    | div            | 2x register, writes carry    | `dest = dest / src`, `carry = dest % src`            |
| `1001`    | xor            | 2x register                  | `dest = dest ^ src`                                  |
| `1010`    | and            | 2x register                  | `dest = dest & src                                   |
| `1011`    | not            | 2x register                  | `dest = !src`, `dest` and `src` may be same register |
| `1100`    | cmp            | 2x register, writes carry    | `carry = (dest == src) ? 0 : ((dest < src) ? 1 : 2)` |
| `1101`    | je             | 2x register, reads carry     | `if (carry == src) pc = dst`                         |
|           |                |                              | See concurrency notes                                |
| `11100`   | wait           | 1x register                  | `carry = signal #`, `dest = signal value`            |
| `11101`   | fork           | 1x register                  | `carry = parent ? 0 : child segment                  |
| `11110`   | signal         | 1x register, 3-bit immediate | `dest = signal value`, `immediate = signal #`        |
| `1111100` | alloc          | 1x register                  | `dest = new memory segment`                          |
| `1111101` | free           | 1x register                  | `dest = old memory segment`                          |
| `1111111` | halt           | 1x register                  | `halt = reg`, parent signalled                       |

## Concurrency

Let's be silly and add concurrency to our CPU.

Each running "thread" of concurrency has two working segments -- the stack segment, which is expected to be used for stack space, and zero or more general-purpose memory segments. Segment 0 is reserved for a global state table which includes an IDT and a bitmap of segment usage. Segment usage can be one of three values: 

| Value  | Usage     | Notes                  |
|--------|-----------|------------------------|
| `00`   | Unused    | Segment is unallocated |
| `01`   | Stack     | Running "thread"       |
| `10`   | Memory    | In-use via alloc       |
| `11`   | Mapped    | See hardware notes     |

Since we need two bits for each of 4096 segments, we can fit 6 segments/word, so we need 682/4096 words to hold the bitmap. The segment bitmask is protected and cannot be written to except via the `fork`/`alloc`/`free`/`halt` instructions, or by attaching/removing hardware.

Stack segments are effectively running "threads". I haven't decided yet if they're actually executed in parallel (maybe that's an implementation-defined detail? Not sure if it changes the semantics any). Each stack segment is prefixed by a header containing the "thread" state. "Threads" can synchronize by sending each other "signals" via the `signal` instruction. `signal` takes a 3-bit "signal #" operand and a single word value. Some signals are reserved for hardware usage:

| Signal # | Usage                          |
|----------|--------------------------------|
| `0`      | Child death. value = segment # |
| `1-7`    | Application usage.             |

So there's 7 usable signals/"thread" right now.

"Child death" is what happens when a normal hardware fault would occur (e.g. writing to protected memory, illegal instruction, etc) or when the `halt` instruction is invoked. This signal is delivered to the "thread" which called the `fork` instruction to create the child; the parent is responsible for invoking `free` on the child's segment, which is preseved for inspection. When a "thread" dies, all descendents of that thread are automatically halted. Invoking `free` on a segment also frees all descendents of that segment. This allows the reaping parent to potentially inspect any child state before it is destroyed.

Halt/fault codes are disambiguated by the topmost bit. Current exceptions include

| Code    | Reason                       |
|---------|------------------------------|
| 0       | Natural death                |
| 1-2047  | User-defined codes           |
| 2048    | Write to unused segment      |
| 2049    | Null instruction executed    |
| 2050    | Invalid instruction executed |
| 2051    | Deadlock detected            |
| 2052    | Signalled invalid segment    |

The first 18 words of each stack segment are fixed, and look like this

| Start | Length | Contents                   |
|-------|--------|----------------------------|
| 0     | 1      | Parent segment #           |
| 1     | 1      | Halt/fault reason          |
| 2     | 1      | Blocked signal destination |
| 3     | 16     | Saved/current registers    |

These words can be read (by any "thread", memory protection is too hard) but cannot be written to.

The initial "thread" has stack segment 1 allocated for it, and no parent. The initial "thread" is responsible for initializing the application and setting up the global IDT (see hardware notes). If the initial "thread" halts or faults the CPU shuts down. The program is loaded into an arbitrary segment mapped for memory (which is put in the memseg register) and the PC is set to 0.

### Signals

Threads communicate by signals. These entirely replace interrupts, which require additional instructions, which is annoying. Signals are effectively (#, value) tuples that are delivered to a specified segment #. Waiting signals are retrieved via the `wait` instructions. Signals are not queued; `signal` will block until a corresponding `wait` is reached. When `signal` blocks (e.g. the destination is not currently blocked on a `wait` instruction), the "blocked signal destination" field of the segment header is set. The CPU then recursively checks each "blocked signal destination" field of the signalled "thread" looking for a cycle. If a cycle is detected, the entire cycle is slain with a deadlock fault.

Attempting to send a signal to a non-stack segment will additionally cause the sender to be slain.

### Alloc/Free

TODO

## Hardware

TODO
