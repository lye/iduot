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

 * `PC`: program counter; used for next operation.
 * `SP`: stack pointer; expected to be used for stack operations.
 * `WMEM`: write memory segment; modifies write address.
 * `RMEM`: read memory segment; modifies read address.
 * `CARRY`: carry register; special register set/read by various instructions.
 * `ZERO`: zero register; always 0, cannot be written to.
 * 10 general purpose registers: a, b, c, d, e, f, g, h, i, j

## Instructions

Each instruction is encoded in a 12-bit word. Most of the instructions are an opcode followed by two 4-bit register identifiers, e.g.

| Opcode   | Dest register | Source Register |
|----------|---------------|-----------------|
| bits 0-4 | bits 5-8      | bits 9-12       |

The opcode table is as follows:

| Opcode     | Instruction    | Operands                     | Notes                                                |
|------------|----------------|------------------------------|------------------------------------------------------|
| `0000`     | loadimm        | register, 4-bit immediate    | `register = immediate`, immediate cannot be 0        |
| `0001`     | loadmem        | 2x register                  | `dst = *src` where src is on current RMEM segment    |
| `0010`     | storemem       | 2x register                  | `*dst = src` where dst is on current WMEM segment    |
| `0011`     | loadstk        | 2x register                  | `dst = *(sp - src)`                                  |
| `0100`     | storestk       | 2x register                  | `*(sp - dst) = src`                                  |
| `0101`     | add            | 2x register, writes carry    | `dst = dst + src`, `carry = 1` if overflow           |
| `0110`     | sub            | 2x register, writes carry    | `dst = dst - src`, `carry = 1` if underflow          |
| `0111`     | mul            | 2x register, writes carry    | `dst = dst * src`, `carry = overflow`                |
| `1000`     | div            | 2x register, writes carry    | `dst = dst / src`, `carry = dst % src`               |
| `1001`     | nand           | 2x register                  | `dst = !(dst & src)`                                 |
| `1010`     | cmp            | 2x register, writes carry    | `carry = (dst == src) ? 0 : ((dst < src) ? 1 : 2)`   |
| `1011`     | jce            | 2x register, reads carry     | `if (carry == src) pc = dst`                         |
| `1100`     | mv             | 2x register                  | `dst = src`                                          |
| `1101`     | signal         | 2x register                  | `dst = signal #`, `src = signal value`, `carry = task id` |
| `1110`     | wait           | 2x register                  | `dst = signal #`, `src = signal value`, `carry = task id` |
| `11110000` | alloc          | 1x register                  | `reg = new memory segment`                           |
| `11110001` | free           | 1x register                  | `reg = old memory segment`                           |
| `11110010` | fork           | 1x register                  | `reg = parent ? child segment : 0`                   |
| `11110011` | waitfor        | 1x register                  | `reg = task id to filter next wait call`             |
| `11110100` | getiseg        | 1x register                  | `reg = current instruction segment`                  |
| `11110101` | setiseg        | 1x register                  | `current instruction segment = reg`; delayed 1 inst  |
| `11110110` | taskid         | 1x register                  | `reg = current task id`                              |
| `11110111` |                |                              |                                                      |
| `11111000` |                |                              |                                                      |
| `11111001` |                |                              |                                                      |
| `11111010` |                |                              |                                                      |
| `11111011` |                |                              |                                                      |
| `11111100` |                |                              |                                                      |
| `11111101` |                |                              |                                                      |
| `11111110` |                |                              |                                                      |
| `11111111` | halt           | 1x register                  | `halt = reg`, parent signalled                       |

XXX: There are a bunch of open 1-operand opcodes available. Possible ideas:

 * Remove the stack segment headers and provide accessors for them instead.
 * `inc`/`dec`.

### Per-instruction notes

#### `loadimm dst imm`

`loadimm` is the load immediate instruction. It copies a 4-bit intermediate value encoded in the instruction into the provided register. As a special case, `0` is not allowed as the immediate; use the `ZERO` register instead. This allows the CPU to detect attempted execution of `NUL` bytes and fault the task.

```
; set A to 4
loadimm A 4

; 4-bit immediate, so 15 is the largest number that can be encoded
loadimm A 16 ; assembler error

; 0 can be encoded, but is an invalid instruction
loadimm A 0 ; fault
mv A ZERO   ; works, but better is to just use ZERO directly
```

#### `loadmem dst src`/`storemem dst src`

Each task has two memory segments mapped simultaneously -- one for reading, another for writing. `loadmem` copies a specified value relative to the `RMEM` segment into the provided register, while `storemem` copies the specified register value into a location relative to the `WMEM` register. The `RMEM` and `WMEM` registers can be read/modified with the `mv` instruction. If the value of `RMEM`/`WMEM` is marked as an unused segment, the task will be faulted.

```
; copy word from (0x4 0x14) to (0x3 0x12)
loadimm RMEM 4
loadimm WMEM 3
loadmem A 14
storemem A 14
```

#### `loadstk dst src`/`storestk dst src`

Each task additionally has a unique stack segment. `loadstk`/`storestk` are used for peeking/poking into the stack, and operate relative to the current value of the `SP` register. I'm guessing stacks are going to grow up. Values past the current value of `SP` cannot be addressed, since the offsets provided are unsigned and are subtracted from the value of `SP`.

```
; push A register onto the stack
storestk A ZERO ; top of stack
loadimm B 1
add SP B

; overwrite stack-stored A value with C
loadimm B 1
storestk C B
```

#### `add dst src`/`sub dst src`

Unsigned arithmetic. Overflows/underflows are stored in the `CARRY` register after execution. If there is no underflow/overflow, this clears the `CARRY` register.

```
; A = 4 + 2, "add B to A"
loadimm A 4
loadimm B 2
add A B
```

```
; A = 4 - 2, "sub B from A"
loadimm A 4
loadimm B 2
sub A B
```

#### `mul`

Unsigned multiply. Overflow is stored in the `CARRY` register. Assembler will use this to synth `lshift` instructions.

```
loadimm A 0xF ; A = 15
mul A A       ; A = 225, CARRY = 0
mul A A       ; A = 3375, CARRY = 0
mul A A       ; A = 12, CARRY = 1473
```

```
; lshift A 3 (tmp=B)
; NB: max lshift is 3; need additional instructions for more shifts
loadimm B 8
mul A B
```

#### `div`

Unsigned divide. The remainder is stored in the `CARRY` register. Assembler will use this to synth `rshift` instructions.

```
loadimm A 0xF ; A = 15
loadimm B 0x7 ; B = 7
div A B       ; A = 2, CARRY = 1
loadimm B 0x2
div A B       ; A = 1, CARRY = 0
div A ZERO    ; fault
```

#### `nand`

Bitwise nand. The assembler will use this to synthesize the rest of the binary operations (which will require temporary registers). It's anticipated that binary operations will be infrequent enough that the additional instruction overhead won't be too bad (famous last words).

```
; not A
nand A A
```

```
; and A B
nand A B
nand A A
```

```
; or A B
; NB: overwrites B.
nand A A
nand B B
nand A B
```

```
; nor A B
; NB: overwrites B.
nand A A
nand B B
nand A B
nand A A
```

```
; xor A B
; NB: overwrites B, requires temp C.
mv C A
nand C B
nand A C
nand B C
nand A B
```

```
; xnor A B
; NB: overwrites B, requires temp C.
mv C A
nand C B
nand A A
nand B B
nand A B
nand A C
```

#### `cmp val1 val2`

Compares two register values, and stores a result in `CARRY`. The result will be one of three values:

| Value A | Relation | Value B | `CARRY` |
|---------|----------|---------|---------|
| 1       | <        | 2       | 2       |
| 1       | =        | 1       | 1       |
| 2       | >        | 1       | 0       |

#### `jce val addr`

Compares the dst register with `CARRY` and, if equal, sets the `PC` to the src register value. This can be used to construct a good deal of jumping primitives. An unconditional jump can be implemented via `mv PC addr`. For example, `jle` can be implemented via:

```
; i.e., to implement
;   cmp b, 0x4
;   jle addr

  loadimm A 0x4
  cmp A B
  jce ZERO label
  mv PC addr
label: 
```

#### `mv dst src`

`mv` copies one register value to another register. It might be removed at some point, since the behavior can be replicated with load/store pairs, but it feels like that might bloat executable size too much. So it's provided for now to hopefully make executables small enough to fit in single segments.

#### `signal src_no src_value` / `wait dst_no dst_value` / `waitfor task`

`signal` sends a signal to another task. The task id should be stored in the `CARRY` register (you can just `mv CARRY id`). This instruction blocks the current task until the receiving task invokes a `wait` without a signal filter, or with a signal filter matching the sending task's task id. The signal filter can be set with `waitfor task`, which will restrict the following `wait` to receive only signals from that task. If the receiving task isn't currently blocked on `wait`, a deadlock check is performed. If a deadlock is found (e.g. a cycle in the `signal` dependency graph) all tasks in the cycle are slain.

`wait` receives a signal sent from another task. When receiving a signal, the `CARRY` register is set to the task id of the sender, and the `dst_no`/`dst_value` are set to what was specified in the `signal` invocation.

Except for the init task (`taskid = 1`) which reserves some for hardware, signal numbers are application/hardware defined (depending on whom you're talking to). Values are additionally whatever you want.

Signal `0` is reserved for "child death" signals. These are sent from a descendent when a fault occurs, or when `halt` is executed. The signal value contains the fault reason. While the descendant state can be inspected, the descendant task id should be passed to `free` to reap the descendant task.

`waitfor ZERO` clears the current signal filter.

```
; task 1
loadimm CARRY 2 ; task we're sending a signal to
loadimm A 1     ; signal number we're sending
loadimm B 2     ; signal value we're sending
signal A B      ; blocks until task 2 executes wait

; task 2
wait A B
; CARRY = 1
; A = 1
; B = 2
```

```
; task 1
loadimm CARRY 2
loadimm A 1
loadimm B 2
signal A B

; task 2
loadimm A 3
waitfor A    ; set a signal filter for the next wait call
wait A B     ; does not receive signal, only will receive signal from task 3
             ; filter is now cleared
wait A B     ; will receive from task 1 and unblock
```

```
; task 1
taskid CARRY
signal A B    ; fault, deadlocked (cycle of 1)
```

```
; task 1
loadimm CARRY 2
signal A B

; task 2
loadimm CARRY 1
signal A B     ; fault, deadlocked. Both tasks slain.
```

```
; task 1
loadimm CARRY 2
signal A B

; task 2
loadimm A 3
waitfor A
wait A B

; task 3
loadimm CARRY 1
signal A B      ; fault, deadlocked. All three tasks slain.
```

#### `alloc dst` / `free src`

`alloc` marks an unallocated segment as general-purpose memory and returns the segment identifier in `dst`. If there is no available segment, `0` is returned. The returned segment is associated with the invoking task, and automatically freed when the task is reaped. The segment may be pre-emptively freed by passing the returned segment id to `free`. This may only be done by the owning task; it will fault a task which tries to free another's segments. The returned segment is zero-initialized.

`free` may additionally be invoked on stack segments to kill and reap the underlying task. Any task may reap another at any time. Reaping a child recursively reaps all that child's descendants. This does not generate child death signals. Reaping the init task shuts down the CPU.

#### `fork dst`

`fork` creates a new task. The following operations are performed:

 * An unused segment is found and marked as a stack segment.
 * The current task's stack segment is copied to the new stack segment.
 * The new task's id is set to the stack segment's id.
 * The new task's parent is set to the invoker's task id.
 * The parent task's `dst` register is set to the new task id.
 * The new task's `dst` register is set to `0`.

All other registers remain unchanged. The child task and parent task continue execution concurrently. It is expected that the child will reinitialize it's stack (via copying any relevant values to the start of the stack, then `mv SP ZERO`).

#### `getiseg dst`/`setiseg src`

`get/setiseg` get and set the current instruction segment (from which the `PC` register is offset). `getiseg` just returns it into `dst`. `setiseg` sets the instruction segment, but the setting is deferred by an instruction. This allows the `PC` register to be initially set. If you don't need this delay slot (because the new instruction segment is magically assembled and the current `PC` is where you want it to be) then just do a random no-op instruction (`mv ZERO ZERO`?).

```
; switch PC to read from next segment
loadimm A 1
getiseg B    ; B = 1
add A B
setiseg A    ; iseg = 1
mv PC ZERO   ; iseg = 1, PC = 0
             ; iseg = 2, PC = 0
```

#### `taskid dst`

`taskid` returns the current task's id into `dst`. I'm not sure what this is useful for, but hey.

### Encoding

All instructions are encoded to 12 bits. Variable-length ISAs are for crazy people; iduot isn't about taking riscs. There's only four different instruction encodings, which each have their formats in the following sections. Registers are encoded using a 4-bit integer with the following mapping:

| Register | Value | Binary |
|----------|-------|--------|
| ZERO     | `0x0` | `0000` |
| PC       | `0x1` | `0001` |
| SP       | `0x2` | `0010` |
| WMEM     | `0x3` | `0011` |
| RMEM     | `0x4` | `0100` |
| CARRY    | `0x5` | `0101` |
| A        | `0x6` | `0110` |
| B        | `0x7` | `0111` |
| C        | `0x8` | `1000` |
| D        | `0x9` | `1001` |
| E        | `0xA` | `1010` |
| F        | `0xB` | `1011` |
| G        | `0xC` | `1100` |
| H        | `0xD` | `1101` |
| I        | `0xE` | `1110` |
| J        | `0xF` | `1111` |

XXX: Wouldn't it be more fun if `0xA-0xD` was registers A-D?

#### One Register Operands

For `fork`, `alloc`, `free`, and `halt`. Each of these instructions takes exactly one register operand, but a variable length opcode. The instructions are encoded as:

| Example   | Higher Bits | Lower Bits  |
|-----------|-------------|-------------|
|           | opcode      | register    |
| `alloc B` | `11110000`  | `0111`      |

#### Two Register Operands

For every other instruction, except load immediate and signal. These each have 4-bit opcodes and two register operands.

| Example      | Bits 8-12 | Bits 4-8 | Bits 0-4 |
|--------------|-----------|----------|----------|
|              | opcode    | reg1     | reg2     |
| `add A B`    | `0101`    | `0110`   | `0111`   |
| `wait A B`   | `1110`    | `0110`   | `0111`   |
| `signal A B` | `1101`    | `0110`   | `0111`   |

#### Load Immediate

Load immediate is similar to a two register operand, except the second register is actually an immediate value. As an additional constraint, the immediate value may not be 0 (a NUL instruction is illegal and attempting to execute it will fault the task). The immediate value is 4-bits. You can load a larger immediate by combining multiple `load immediates` with `mul`. To load 0, use `xor R R`.

| Example     | Bits 8-12 | Bits 4-8 | Bits 0-4 |
|-------------|-----------|----------|----------|
|             | opcode    | reg      | imm      |
| `loadi A 4` | `0000`    | `0110`   | `0100`   |

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

Stack segments are effectively running tasks. I haven't decided yet if they're actually executed in parallel (maybe that's an implementation-defined detail? Not sure if it changes the semantics any). Each stack segment is prefixed by a header containing the task state. Tasks can synchronize by sending each other "signals" via the `signal` instruction. `signal` takes a "signal #" operand and a single word value. Some signals are reserved for hardware usage:

| Signal # | Usage                          |
|----------|--------------------------------|
| `0`      | Child death. value = segment # |

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
| 3     | 1      | Inc signal source filter   |
| 4     | 1      | Inc signal count           |
| 5     | 16     | Saved/current registers    |

XXX: There are a pile of unused 1-operand instructions that can be made into accessors for the above values. How much do we care about that?

These words can be read (by any task, memory protection is too hard) but cannot be written to.

The initial task has stack segment 1 allocated for it, and no parent. The initial task is responsible for initializing the application and setting up the global IDT (see hardware notes). If the initial task halts or faults the CPU shuts down. The program is loaded into an arbitrary segment allocated for memory (which is put in the memseg register) and the PC is set to 0. Additional segments for each piece of attached hardware are mapped, and a signal is sent to the initial task for each attached piece of hardware.

### Signals

Tasks communicate via signals. These entirely replace interrupts, which require additional instructions, which is annoying. Signals are effectively (#, value) tuples that are delivered to a specified segment #. Waiting signals are retrieved via the `wait` instructions. Signals are not queued; `signal` will block until a corresponding `wait` is reached. When `signal` blocks (e.g. the destination is not currently blocked on a `wait` instruction), the "blocked signal destination" field of the segment header is set. The CPU then recursively checks each "blocked signal destination" field of the signalled task looking for a cycle. If a cycle is detected, the entire cycle is slain with a deadlock fault.

Before calling `wait`, a task may issue a `waitfor` instruction. The `waitfor` sets the task's inc signal source filter to the value of the passed register; if this value is non-zero, wait will only retrieve signals from the specified task. This may be used as a synchronization primitive, e.g.

```
; Send a signal as a notification (e.g. flush page)
loadimm A $SIGNAL_NO
loadimm B $SIGNAL_VAL
xor CARRY CARRY
add CARRY $TASK_ID
signal A B

; Then wait for the hardware/task to complete the operation
waitfor CARRY
wait A B
```

When a signal is retrieved via `wait`, the signal source filter is cleared. The signal source filter may additionally be cleared via `waitfor ZERO`.

XXX: the signal source filter needs to be checked during deadlock detections.

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

