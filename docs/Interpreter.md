# Interpreter: Jump Table Optimization

The jump table is a technique that significantly improves the performance of the in-place interpreter. Among all control instructions, only a few can change the following instruction. They are:

- `if`: If the stack top is zero, jump to either the `else` block or the instruction after `end`.
- `else`: Always jump to the instruction after the `end` of the `if-else-end` block.
- `br/br_if/br_table`: Jump to the target location of one of the labels in the stack, with or without a condition. The target is the instruction after the `end` for `block` and `if`, and for `loop`, it's the instruction at or after the `loop`.

A typical interpreter will build a label stack with target locations. Executing jump instructions involves a label stack search, followed by jumping to the corresponding location, as shown in the pseudocode below.

- Preparation pass (usually done during the validation pass):
  - Build a side-table using the following algorithm:
    - On `block` instructions, push a label frame.
      - If the `block` is a `loop`, set the label frame target to the `loop` itself; otherwise, leave the target empty.
    - On `end` instructions, update the belonging label frame's target.
    - `if` and `else` require special treatment, but the idea is the same.
- During execution:
  - On `block` instructions, push a label frame.
  - On `br` instructions, find the target label frame and the side-table item for that frame. Using the side-table, we can locate the jump target and then jump to that location.
  - `if` and `else` also require special treatment.

This approach works but is not fast enough. There are three main reasons for its slow performance:

1. Pushing and popping label frames to the stack is slow.
2. Side-table lookup is slow because even with a hash table, the overhead is too large for such high frequency, considering that `block` is one of the hottest instructions.
3. The side-table is quite large.

To address these issues, I devised a solution that eliminates the runtime cost of label stack construction and table lookup while maintaining the memory cost of a side-table. The goal is to:

- Remove the need for label frames, making `block` and `loop` instructions effectively no-ops.
- Eliminate the need for side-table lookup. The next slot location should be pre-calculated and stored in the table.

The solution is as follows.

First, let's examine the instructions that may potentially change the execution flow.

- The `if` instruction: There are two potential outcomes after executing the instruction, depending on the stack top value. If the stack top is zero, the jump target should be either the `else` block, if it exists, or to the `end`. When the interpreter encounters an `if`, the side table must provide the jump location if the condition is false. During the preparation pass, we must create a slot for the `if` and update the slot with the correct jump offset when we hit either `else` or the `end`.
- The `br` instruction: The `br` instruction always jumps to a target label frame. When we encounter a `br`, we don't know the target's location until we hit the target frame's `end`. We can leave the target location of the `br` jump table slot empty, and when we eventually hit the `end`, we can update the `br` slot in the jump table by adding a label frame vector member to record all the `br` instructions that jump to the end of the label. This is illustrated in the example below:

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

Since all `br` instructions should be attached to one of the label frames in the label stack, when we finish validating a function and pop up all the labels, we know all the `br`s are updated with their target offset.

Now, let's tackle the second question: how can we get rid of the table lookup? We need to determine the next lookup item's index without searching the table.

We know that `br` will take us to a new location, and then we continue reading the instructions until we hit the next `br`. (Using `br` as an example, but other jump instructions are similar). So, the next `br`'s own address must be larger than the jump target of the current `br`.

```
1: block
2:  block
3:    br 0 <-- jump target is line:5. The "next branch" must be something after line:5, and here it is line:6
4:  end
5:  local.get
6:  br 0
7:end
```

We *can* determine the next `br`. Simply search the opcode from the target location, and the first `br` will be the next one.

During execution, we can have a "next br index" variable pointing to the next possible branch instruction index in the jump table. When we hit a branch instruction, we can update the "next br index" so that next time we can use that index to directly access the slot without any lookup.

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

At the beginning, the "next index" is zero. So, when we hit a branch for the first time, we have:

```[0] PC:000113 TGT:000117 Arity:0 StackOffset:0 Next:2```

This means:

- The jump target is 0x117.
  - If the jump is taken, the next instruction is 0x117. The next slot index is 2.
  - If the jump is not taken, the next slot index will be `current + 1`, which is 1.
- The arity and StackOffset are used to shift the value stack.
  - This is similar to returning from a function or a label: we move "arity" number of stack items backward at "StackOffset" number of distance.
  - The "Next" index is the possible next jump table slot.

By implementing the jump table this way, we can eliminate the need for label frames, making `block` and `loop` instructions no-op. We also avoid the need for side-table lookups, as the next slot location is pre-calculated and stored in the table. This results in a more efficient interpreter that can execute WebAssembly code faster.