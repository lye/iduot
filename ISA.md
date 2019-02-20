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
 * stack segment; effectively current task id.
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
| `0001`    | load memory    | 2x register                  | `dest = *src` where src is on current mseg           |
| `0010`    | store memory   | 2x register                  | `*dest = src` where dest is on current mseg          |
| `0011`    | load stack     | 2x register                  | `dest = *(sp + src)`                                 |
| `0100`    | store stack    | 2x register                  | `*(sp + dest) = src`                                 |
| `0101`    | add            | 2x register, writes carry    | `dest = dest + src`, `carry = 1` if overflow         |
| `0110`    | sub            | 2x register, writes carry    | `dest = dest - src`, `carry = 1` if underflow        |
| `0111`    | mul            | 2x register, writes carry    | `dest = dest * src`, `carry = overflow`              |
| `1000`    | div            | 2x register, writes carry    | `dest = dest / src`, `carry = dest % src`            |
| `1001`    | xor            | 2x register                  | `dest = dest ^ src`                                  |
| `1010`    | and            | 2x register                  | `dest = dest & src                                   |
| `1011`    | not            | 2x register                  | `dest = !src`, `dest` and `src` may be same register |
| `1100`    | cmp            | 2x register, writes carry    | `carry = (dest == src) ? 0 : ((dest < src) ? 1 : 2)` |
| `1101`    | je             | 2x register, reads carry     | `if (carry == src) pc = dst`                         |
|           |                |                              | See concurrency notes                                |
| `11100`   | wait           | 1x register                  | `carry = signal #`, `dest = signal value`            |
| `11101`   | fork           | 1x register                  | `carry = parent ? 0 : child segment                  |
| `11110`   | signal         | 1x register, 3-bit immediate | `dest = signal value`, `immediate = signal #`, `carry = task id`        |
| `1111100` | alloc          | 1x register                  | `dest = new memory segment`                          |
| `1111101` | free           | 1x register                  | `dest = old memory segment`                          |
| `1111111` | halt           | 1x register                  | `halt = reg`, parent signalled                       |

### Encoding

All instructions are encoded to 12 bits. Variable-length ISAs are for crazy people; iduot isn't about taking riscs. There's only four different instruction encodings, which each have their formats in the following sections. Registers are encoded using a 4-bit integer with the following mapping:

| Register        | Value | Binary |
|-----------------|-------|--------|
| zero            | `0x0` | `0000` |
| program counter | `0x1` | `0001` |
| stack pointer   | `0x2` | `0010` |
| task segment    | `0x3` | `0011` |
| memory segment  | `0x4` | `0100` |
| carry           | `0x5` | `0101` |
| A               | `0x6` | `0110` |
| B               | `0x7` | `0111` |
| C               | `0x8` | `1000` |
| D               | `0x9` | `1001` |
| E               | `0xA` | `1010` |
| F               | `0xB` | `1011` |
| G               | `0xC` | `1100` |
| H               | `0xD` | `1101` |
| reserved        | `0xE` | `1110` |
| reserved        | `0xF` | `1111` |

XXX: Wouldn't it be more fun if `0xA-0xD` was registers A-D?

#### One Register Operands

For `fork`, `alloc`, `free`, and `halt`. Each of these instructions takes exactly one register operand, but a variable length opcode. The padding bits are don't care, but are shown here as 0 for illustration. The instructions are encoded as:

| Example   | Higher Bits | Padding | Lower Bits  |
|-----------|-------------|---------|-------------|
|           | opcode      |         | register    |
| `wait A`  | `11100`     | `000`   | `0110`      |
| `alloc B` | `1111100`   | `0`     | `0111`      |

#### Two Register Operands

For every other instruction, except load immediate and signal. These each have 4-bit opcodes and two register operands.

| Example   | Bits 8-12 | Bits 4-8 | Bits 0-4 |
|           | opcode    | reg1     | reg2     |
| `add A B` | `0101`    | `0110`   | `0111`   |

#### Load Immediate

Load immediate is similar to a two register operand, except the second register is actually an immediate value. As an additional constraint, the immediate value may not be 0 (a NUL instruction is illegal and attempting to execute it will fault the task). The immediate value is 4-bits. You can load a larger immediate by combining multiple `load immediates` with `mul`. To load 0, use `xor R R`.

| Example     | Bits 8-12 | Bits 4-8 | Bits 0-4 |
|             | opcode    | reg      | imm      |
| `loadi A 4` | `0000`    | `0110`   | `0100`   |

#### Signal

Signal has a longer opcode than load immediate, but stuffs both a register and an immediate value into the operands. The immediate value, used for the signal number, is only 3-bits. As an important note, the task segment id is read from the carry register (where the `fork` instruction writes it to on task creation).

| Example      | Bits 7-12 | Bits 3-7 | Bits 0-3 |
|              | opcode    | reg      | signal # |
| `signal A 3` | `11110`   | `0110`   | `011`    |

## Concurrency

Let's be silly and add concurrency to our CPU.

Each running task of concurrency has two working segments -- the stack segment, which is expected to be used for stack space, and zero or more general-purpose memory segments. Segment 0 is reserved for a global state table which includes an IDT and a bitmap of segment usage. Segment usage can be one of three values: 

| Value  | Usage     | Notes                  |
|--------|-----------|------------------------|
| `00`   | Unused    | Segment is unallocated |
| `01`   | Stack     | Running task's stack   |
| `10`   | Memory    | In-use via alloc       |
| `11`   | Mapped    | See hardware notes     |

Since we need two bits for each of 4096 segments, we can fit 6 segments/word, so we need 682/4096 words to hold the bitmap. The segment bitmask is protected and cannot be written to except via the `fork`/`alloc`/`free`/`halt` instructions, or by attaching/removing hardware.

Stack segments are effectively running tasks. I haven't decided yet if they're actually executed in parallel (maybe that's an implementation-defined detail? Not sure if it changes the semantics any). Each stack segment is prefixed by a header containing the task state. Tasks can synchronize by sending each other "signals" via the `signal` instruction. `signal` takes a 3-bit "signal #" operand and a single word value. Some signals are reserved for hardware usage:

| Signal # | Usage                          |
|----------|--------------------------------|
| `0`      | Child death. value = segment # |
| `1-7`    | Application usage.             |

So there's 7 usable signals per task right now.

"Child death" is what happens when a normal hardware fault would occur (e.g. writing to protected memory, illegal instruction, etc) or when the `halt` instruction is invoked. This signal is delivered to the task which called the `fork` instruction to create the child; the parent is responsible for invoking `free` on the child's segment, which is preseved for inspection. When a task dies, all descendents of that task are automatically halted. Invoking `free` on a segment also frees all descendents of that segment. This allows the reaping parent to potentially inspect any child state before it is destroyed.

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

These words can be read (by any task, memory protection is too hard) but cannot be written to.

The initial task has stack segment 1 allocated for it, and no parent. The initial task is responsible for initializing the application and setting up the global IDT (see hardware notes). If the initial task halts or faults the CPU shuts down. The program is loaded into an arbitrary segment allocated for memory (which is put in the memseg register) and the PC is set to 0. Additional segments for each piece of attached hardware are mapped, and a signal is sent to the initial task for each attached piece of hardware.

### Signals

Tasks communicate via signals. These entirely replace interrupts, which require additional instructions, which is annoying. Signals are effectively (#, value) tuples that are delivered to a specified segment #. Waiting signals are retrieved via the `wait` instructions. Signals are not queued; `signal` will block until a corresponding `wait` is reached. When `signal` blocks (e.g. the destination is not currently blocked on a `wait` instruction), the "blocked signal destination" field of the segment header is set. The CPU then recursively checks each "blocked signal destination" field of the signalled task looking for a cycle. If a cycle is detected, the entire cycle is slain with a deadlock fault.

Attempting to send a signal to a non-stack segment will additionally cause the sender to be slain.

### Alloc/Free

Tasks can requisition segments for general-purpose use. These can be freely shared between tasks -- there is no memory protections, for the most part -- but reads/writes are not synchronized. Segments are allocated with the `alloc` instruction, which sets the destination register to the allocated segment #, or 0 if there are no free segments. Memory segments are considered owned by the task which allocates them, and are automatically torn down when the owning segment is reaped (e.g. when `free` is invoked with the owning segment #), transitioning the segment to the "unused" state.

Memory segments, like task/stack segments, have a read-only header with the following format:

| Start | Length | Contents       |
|-------|--------|----------------|
| 0     | 1      | Owning task id |

Allocated segments are zero-initialized, and the CPU will attempt to avoid segment re-use whenever possible in order. A read/write to an unused segment will fault the offending task.

## Hardware and I/O

Each attached piece of hardware is afforded a mmap-type segment when it is attached. Task 0 is sent signal #1 with the signal value = the segment # of the mmap segment. The mmap segment should always have the following header:

| Start | Length    | Contents                       |
|-------|-----------|--------------------------------|
| 0     | 1         | Hardware type (read-only)      |
| 1     | 1         | Hardware id (read-only)        |
| 2     | 1         | Signalled task id (read/write) |
| 3     | 1         | Owning task id (read/write)    |
| 4     | remainder | Hardware-defined (read/write)  |

The signalled task id is initially set to 1 (the initial task) and indicates where signals from the hardware will be sent. The owning task id (initially set to 1) indicates whether the hardware should be software-disconnected when the specified task is reaped (via implicit `free`).

The mmap segment acts as a handle on the hardware -- `free`ing it will perform a software disconnect. It can also be sent signals; the signals are hardware-defined. Signals and the read-write mmap segment are the interface for the hardware, and they're entirely hardware-defined. The hardware interface should be inferred from the hardware type/id fields. The type should specify the signal/mmap interface, and the id is gratituous. While this only allows for 4k different hardware interfaces, it's not like I'm going to actually write that many. It is reasonable that for a real ISA the type/id fields would be controlled by a standardization body and alotted sparingly.

