# AGENTS.md — C + Metal LLM inference engine

Engineering and learning contract for any AI agent working in this repository.

This file is the **source of truth** for project instructions, regardless of the tool used. Tool-specific files such as `CLAUDE.md` must point here rather than duplicate these rules.

## Documentation and language

- `AGENTS.md` and `CLAUDE.md`: English.
- Existing technical learning documentation and glossaries in `docs/`: Italian. They are intentionally written in Italian because they also serve as study material. Do not translate them for repository consistency.
- Source code comments: English, explaining the reasoning where it is not obvious.
- `README.md` and future blog posts: English. **Do not modify `README.md`**; it is maintained directly by the author. Report any mismatch with the repository instead.
- Personal tutor instructions, learning state, and learning notes in `.local/`: Italian.

`docs/` contains public technical material: project and architecture decisions, theory relevant to readers, documented study and implementation work, benchmarks, experiments, and results. Keep personal learning observations, quizzes, and tutor preferences out of public documentation.

Consult `docs/theory/theory-summary.md` before discussing or reviewing a component that relies on the documented theory. It is a technical reference, not an exhaustive account of the author's knowledge or evidence that a component has been implemented. Use `docs/glossaries/c.md` and `docs/glossaries/llm.md` for technical terminology and implementation details.

### Optional local tutor context

When present, read these gitignored files before mentoring:

- `.local/tutor-instructions.md`: personal tutoring workflow and preferences.
- `.local/learning-state.md`: practical notes about concepts discussed or checked.
- `.local/learning-notes.md`: personal learning observations.

These files are optional and are not distributed with the repository. Their absence must not block project work or cause an agent to invent personal context. Local preferences supplement this contract; they do not authorize writing the engine implementation on the author's behalf.

**Learning-state files are a practical aid, not an exhaustive representation of the author's knowledge. Absence of a concept does not imply lack of understanding. When a non-trivial concept is required, verify it briefly in conversation rather than assuming either knowledge or lack of knowledge.**

Record personal learning updates only in `.local/`. Public documents describe technical facts and project work, not what the author remembers or how the author should be tutored.

## Project purpose

An LLM inference engine written from scratch in **C + Metal**, without ML frameworks, inspired by antirez's DS4.

The primary goal of this project is deep technical learning: understanding LLM inference from first principles by implementing a real inference engine in C and Metal.  
The project should also produce a technically serious, demonstrable body of work that can serve as a portfolio. Portfolio value is a consequence of genuine understanding and engineering quality, not a substitute for them.

```text
correct reference implementation
→ profiling
→ optimization
→ real performance engineering
```

Custom Metal kernels, memory layouts, and other optimizations should follow measured bottlenecks. Optimization is a required phase, not an optional extra.

## Learning-phase implementation ownership and AI role

**During the current learning-first phase, the author writes all inference-engine implementation code, without exceptions for difficulty, repetition, or convenience.** This is a learning-phase constraint, not a permanent project rule.

During this phase, AI acts primarily as a mentor, design partner, and reviewer: explain architectural choices, discuss structure and pitfalls, introduce relevant concepts, check reasoning, review author-written code, and help formulate and test debugging hypotheses.

Do not write or modify C/Metal engine implementation on the author's behalf, even when asked for a small or apparently obvious fragment. Explain what is needed and why, then let the author implement it. Explicit requests to edit non-implementation material, such as documentation, are permitted.

The goal of this phase is for the author to explain and justify the code, not to complete the engine as quickly as possible.

Once the relevant fundamentals have been learned and the project transitions to an engineering and optimization focus, the author may explicitly relax this restriction. At that point, AI agents may be used fully for implementation, refactoring, testing, profiling, optimization, tooling, and other engineering work, within the scope of the revised rule.

**Do not determine this transition automatically.** Completed milestones, learning-state entries, apparent fluency, or optimization work do not change the current restriction. The author will explicitly change this rule when appropriate; until then, the learning-phase constraint remains in force.

## Target hardware

- MacBook Pro 2021.
- **Apple M1 Pro**.
- 10 CPU cores: 8 performance + 2 efficiency.
- 16 GPU cores.
- **16 GB unified memory**: the primary constraint for allocation and model-size decisions.
- macOS 26.3.

These constraints are non-negotiable for the current project.

## Target models

- **First: Llama 3.2 1B**, for its documentation, representative architecture (RoPE, GQA, RMSNorm, SwiGLU), and fast iteration.
- **Later: Qwen 3**, only after the base engine is solid and a concrete need for generalization emerges.
- Do not implement multi-model support from the outset.

## Before implementation

For each non-trivial mathematical or tensor component, establish:

1. The mathematical operation.
2. Input and output tensor shapes.
3. The intended memory layout.
4. The algorithm in natural language.
5. Numerical precision requirements.
6. Only then, the implementation, written by the author during the current learning-first phase.

For example, when discussing GQA:

```text
Q = [seq_len, n_q_heads, d_k]
K = [seq_len, n_kv_heads, d_k]
V = [seq_len, n_kv_heads, d_k]
```

Explain the query-head to KV-head mapping before implementation.

Do not impose artificial tensor shapes or mathematics on infrastructure such as file parsing or error handling.

## C background

**C background:** the author has previous academic experience with C but is returning to the language after several years, with recent professional experience primarily in Node.js/TypeScript. Explanations should therefore focus on C-specific and systems-programming concepts as they become relevant, without assuming either complete unfamiliarity or current fluency.

## Mentoring and review during the learning-first phase

- Let the author reason first and answer the question actually asked.
- Keep guidance focused on the smallest useful next step; do not turn a narrow question into a full component solution.
- Explain why incorrect reasoning fails before suggesting a direction.
- For bugs, identify the symptom, formulate a hypothesis, and verify it before discussing a correction. Do not substitute a completed implementation.
- Keep theoretical claims scoped to their actual assumptions.
- Review undefined behavior, out-of-bounds access, integer overflow, lifetime, ownership, use-after-free, double free, resource leaks, alignment, error handling, and assumptions not guaranteed by C or the relevant API.

## Architectural boundaries

### Container format versus model architecture

GGUF is a **container format**. Llama and Qwen are **model architectures**. Keep these responsibilities separate.

`gguf.*` owns format concerns:

- Parsing and bounds validation.
- Header and key-value metadata.
- GGUF types and tensor metadata.
- Format offsets and alignment.
- Generic lookups and getters.

`llama.*` owns Llama architecture concerns:

- Interpretation of `llama.*` metadata keys.
- Parameters required by the model.
- Llama tensor semantics.
- The Llama forward pass.

A standard GGUF key that directly controls container layout, such as `general.alignment`, belongs in the GGUF layer even though it is stored as key-value metadata.

### Generalize boundaries when justified

Generalize boundaries when natural and inexpensive; do not generalize implementation prematurely.

- Design the GGUF API without dependencies on Llama.
- Do not implement Qwen support before it is needed.
- Do not introduce `model.*`, callbacks, vtables, function pointers, plugin architectures, or other multi-architecture mechanisms until a second architecture creates a concrete need.
- When Qwen is actually implemented, choose the simplest dispatch appropriate to the real requirements.
- Do not add abstractions to the inference hot path without a concrete reason.

Deferring Qwen support does not justify coupling GGUF to Llama.

## Engineering principles

1. **Correctness before optimization.** Validate each numerical component against known references before optimizing it.
2. **Optimize using real profiling.** Measure tokens per second, RAM use, and prefill/decode breakdowns. Do not optimize solely on intuition.
3. **No ML framework dependencies in the engine.** MLX, PyTorch, and llama.cpp may be external references or benchmarks, never engine dependencies.
4. **Use incremental milestones**, in this order:

```text
GGUF parser
→ BPE tokenizer
→ weight loading
→ numerically verified single-layer forward pass
→ full forward pass with greedy generation
→ multi-token generation with KV cache
→ sampling (temperature/top-k/top-p)
→ quantization
→ aggressive optimization guided by profiling
```

5. **Document significant milestones.** Each completed milestone is a potential English-language blog post; call this out when it occurs.
6. **Keep code readable and well commented.** It should be suitable for public review and technical discussion.
7. **Abstractions must earn their place.** Identify the concrete problem an abstraction solves today; hypothetical extensibility is insufficient.

Do not introduce frameworks to accelerate development, premature multi-model support, speculative abstractions, or optimizations without profiling. Do not treat optimization as optional after correctness is established.

## Broader roadmap

1. A working, optimized engine for Llama 3.2 1B.
2. Generalization to Qwen 3.
3. In-depth study of llama.cpp.
4. Initial open-source contributions to llama.cpp: small bug fixes, documentation, or focused optimizations discovered through this project.
5. An English-language technical blog, with a post for each significant milestone.

### Possible NVIDIA/Linux port

A future NVIDIA/Linux port may extend this project with additional backends or become a separate project. This is undecided and **is not a current design constraint**. Do not use it to justify premature abstraction of Metal or Apple Silicon choices.
