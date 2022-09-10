## The Jump Table

The jump table is one of the tricks that makes the in-place interpreter fast. If we look at all the control instructions, there are only a few of them that might change the following instruction follow. They are:

- `if` If the stack top is zero, jump to either the `else` block or the instruction after `end`.
- `else` Always jump to the instruction after the `end` of the `if-else-end` block.
- `br/br_if/br_table` Jump to the the target location of one of the labels in stack, with or without condition. The target is the instruction after the `end` for `block` and `if`, and for `loop` it's the instruction at or after the `loop`.

A typical interpreter will build a label stack with target locations. Executing the jump instructions is as simple as a label stack search, and then jump to the corresponding location as below pseudo code shows.

- Preparation pass: (Usually it is done during the validation pass)
  - Build a side-table with following algorithm:
  - On `block` instructions, push a label frame.
    - If the `block` is a `loop`, set the label frame target to the `loop` itself, otherwise leave the target empty.
  - On `end` instructions, update the belonging label frame's target.
  - `if` and `else` needs a bit of special treats but the idea is the same.
- During execution:
  - On `block` instructions, push a label frame.
  - On `br` instructions, find the target label frame, then find the side table item for that frame. With the side table, we know where is the jump target, then jump to that location.
  - `if` and `else` also needs a bit of special treatment.

It works, but still not fast. There are two reasons that make things slow:

- Having to push and pop label frames to the stack is slow.
- Side table lookup is slow because even with a hash table the overhead is still too big for such a high frequency considering the `block` is one of the hottest instructions.
- Not to mention the side table is quite big.

So I thought about the issue for quite a while, and I believed that since we're paying the memory cost of a side-table, we should really have a way to entirely get rid of the runtime cost of the label stack construction and table lookup. The goal is to eliminate the above mentioned two downsides, which means

- No need for label frames, therefore the `block` and `loop` instructions will eventually become no-op.
- No need for side-table lookup. The next slot location should be pre-calculated and stored in the table.

Turns out it is possible.

Let's again take a look at the instructions that may potentially change the execution flow.

- The `if` instruction. There are two potential outcomes after executing the instruction, depends on the stack top value. If the stack top is zero, then the jump target should be either into the `else` block, if exist, or to the `end`. So when the interpreter run into an `if`, it needs the side table to tell it where's the jump location if condition is false. Therefore, during the preparation pass, we have to create a slot for the `if` and when we hit either `else` or the `end` we can update the slot with the correct jump offset.
- The `br` instruction. The `br` instruction always jump to a target label frame. When we run into a `br`, we don't know where the target is yet, until we hit the target frame's `end`. If we leave the target location of the `br` jump table slot empty, when we eventually hit the `end`, how do we know which `br` slot in the jump table we need to be updated? The way I used is to add a label frame vector member to record all of the `br` that jumps into the end of the label. Let's take below as an example:
```
block <-- This label has 3 pending br recorded.
  block
    block
      br 2
    end
    br 1
  end
  br 0
end <-- Ok now we're closing the block, so we update all of the pending brs' target to here.
```
Since all `br` instructs should be attached to one of the label frames in the label stack, when we finish validating a function and pop up all the labels, we know all the `br`s are updated with their target offset.

Now let's face the second question, how can we get rid of the table lookup? The question eventually becomes: how can we know the next lookup item's index without searching the table?

As we know, `br` will take us to a new location, and then we continue read the instructions until we hit the next `br`. (well I'll only use the `br` as an example but other jump instructions are similar). So the next `br`'s own address must be larger than the jump target of the current `br`.
```
1: block
2:  block
3:    br 0 <-- jump target is line:5. The "next branch" must be something after line:5, and here it is line:6
4:  end
5:  local.get
6:  br 0
7:end
```
So there *is* a way to know which `br` will be the next. We can simply search the opcode from the target location, and the first `br` will be the next one.

During execution we can have a "next br index" variable pointing to the next possible branch instruction index in the jump table. When we hit a branch instruction, we can update the "next br index" so that next time we can use that index to directly access the slot without any lookup.

Below is an example of how the jump table looks like:

```
0000fb func[2] <loop2>:
 0000fc: 01 7f     | local[0] type=i32
 0000fe: 41 00     | i32.const 0
 000100: 21 00     | local.set 0
 000102: 02 7f     | block i32
 000104: 03 7f     |   loop i32
 000106: 20 00     |     local.get 0
 000108: 41 01     |     i32.const 1
 00010a: 6a        |     i32.add
 00010b: 21 00     |     local.set 0
 00010d: 20 00     |     local.get 0
 00010f: 41 05     |     i32.const 5
 000111: 46        |     i32.eq
 000112: 04 40     |     if             <-- jump to 0x117
 000114: 0c 01     |       br 1         <-- jump to 0x106 (loop)
 000116: 0b        |     end
 000117: 20 00     |     local.get 0
 000119: 41 08     |     i32.const 8
 00011b: 46        |     i32.eq
 00011c: 04 40     |     if             <-- jump to 0x123
 00011e: 20 00     |       local.get 0
 000120: 0c 02     |       br 2         <-- jump to 0x12e
 000122: 0b        |     end
 000123: 20 00     |     local.get 0
 000125: 41 01     |     i32.const 1
 000127: 6a        |     i32.add
 000128: 21 00     |     local.set 0
 00012a: 0c 00     |     br 0           <-- jump to 0x106 (loop)
 00012c: 0b        |   end
 00012d: 0b        | end
 00012e: 0b        | end
```

```
Jump table:
[0] PC:000113  TGT:000117  Arity:0   StackOffset:0   Next:2
[1] PC:000115  TGT:000106  Arity:0   StackOffset:0   Next:0
[2] PC:00011D  TGT:000123  Arity:0   StackOffset:0   Next:4
[3] PC:000121  TGT:00012E  Arity:1   StackOffset:0   Next:65535
[4] PC:00012B  TGT:000106  Arity:0   StackOffset:0   Next:0
```

At the beginning, the "next index" is zero. So when we hit a branch for the first time, we have:

`[0] PC:000113  TGT:000117  Arity:0   StackOffset:0   Next:2`

It means:
- The jump target is 0x117.
  - If jump, then the next instruction is 0x117. The next slot index is 2.
  - If not jump, the next slot index will be `current + 1`, which is 1.
- The arity and StackOffset is used to shift the value stack.
  - It's similar to returning from a function or a label: we move "arity" number of stack items backward at "StackOffset" number of distance.
- The "Next" index is the possible next jump table slot.
