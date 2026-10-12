- [Abstract](#orgb27f9ea)
- [Why four papers](#org0ad33a4)
- [What?](#orgdfe2f93)
  - [Transpose](#org6373c3e)
  - [Algorithms for Trees](#org641c40a)
  - [Fingertrees](#org537339c)
  - [Backticks For Infix](#orgfb3d3e7)
- ["Only be sure always to call it please 'Research'" &mdash; Tom Lehrer](#org5fbd580)
- ["Why in the **Standard**"](#org67aafeb)
- [Not Quite What I Set Out To Do](#org1fadcf1)
- [What I want](#orge709ef5)
- [When?](#orgfd9a042)



<a id="orgb27f9ea"></a>

# Abstract

You have a `vector<sender<int>>` and you need a `sender<vector<int>>`, or a `vector<optional<int>>` and you need an `optional<vector<int>>`.

You don't want to write a binary tree. Again. By hand.

You have a pure function. You have arguments in two vectors. You just want to zip the arguments together and apply it.

You want to keep working on a data structure while something else works on it. Making copies is expensive.

<!-- TEASER_END -->

I am on a campaign to improve support in C++ for contemporary algorithms. By contemporary I mean more recent than the Sedgewick C++ Algorithms book I used back in the 90s. There has been extraordinary work done in programming language and algorithmic research in the last 30 years. Much of it even well understood and codified enough to be standardized.

It's not one huge paper, though a couple are shaping up to be beefy. All of them have hooks for the types you already have; none of them asks you to throw away the binary tree you already wrote.


<a id="org0ad33a4"></a>

# Why four papers

This started as a fingertree. A container, adapters for the classic structures it can model, and a Rope. The algorithms we already have work with it, but only as a Sequence, and a fingertree is a tree. So it needed algorithms that did not exist yet, and those algorithms have nothing to do with fingertrees. That is the first paper boundary.

It was always a little strange that the standard library has nothing tree-shaped except `map` and `set`. Trees are basic. When I looked, it turned out the `Fix` data type could be implemented in C++, literally, and it worked surprisingly well. Pattern matching and the visitor are the same thing at the core, Church-encoded sum types fall out of that, and Kmett's recursion schemes stop being exotic. That is the tree paper, and it has its own reason to exist.

If you have trees, you want to traverse them. The insight was that `invoke` is the key operation and `ap` is implementation detail. P3200 owns that, and the tree paper wanted it before I knew what it was. And then zip lists, data parallelism, and SIMD fell out of the same research, which I had not known going in. So the base paper is broader than trees. That is why it is the base.

The papers are in the reverse of the order I found them. Each one is what the one before it turned out to need, and the last one turned out to be bigger than the need.

They come apart cleanly. Without traverse and transpose the tree algorithms are less rich, but they are separable and useful: `Fix`, the recursion schemes, adaptable to the tree types you already have, treatable as Ranges. Fingertree survives on its own as a concrete container that most other languages have had for years; the base tree algorithms and the adapters are what make it a rich one.

If only P3200 Transpose survives, we have Applicative and Traversable, instances for Ranges and others, and a worked-out path for Monad. Range of Context to Context of Ranges is enough.

The word that crosses every boundary is Monoid. It was one of the first typeclasses I wrote, years ago, trying to get what C++11 concept maps would have given us. The interface is trivial. The number of different ways an `int` can be a Monoid is what forced most of the decisions about how instances are found and passed around.

Backtick is parallel evolution. I have been thinking about it since C added `` ` ``, `@`, and `$` to the basic character set for C23, and about the uniform function call proposals as really being about infix position. That it was actually possible I did not work out until the Brno WG21 meeting in Spring 2026. There was also a faction that wanted the backtick for escaping keywords into identifiers. Both turned out to be possible, but probably only if they were done together. The crossover with the rest of this is just that infix is easier to read than nesting, even more than pipelines are.


<a id="orgdfe2f93"></a>

# What?

The base paper is P3200. I got the number by honest accident and sat on it.


<a id="org6373c3e"></a>

## Transpose

You have a `vector<sender<int>>` and you need a `sender<vector<int>>`. Or you need to swap array of struct to struct of array for lane-wise SIMD processing. And you don't want to write that loop one more time.

This is actually what McBride and Paterson invented Applicative and Transpose **for**. Transposing a matrix is the opening example of [*Applicative Programming with Effects*](https://www.staff.city.ac.uk/~ross/papers/Applicative.html) (JFP 2008).

Also, Applicative has almost nothing to do with partial application or currying. It is all about applying a pure function to arguments in contexts.

One verb, three contexts. The code is from the [transpose](https://github.com/steve-downey/transpose) repository's example, which builds and runs in CI.

P3200, *Shape-Preserving Traversal and Transposition for Contextual Computations*, proposes a shape-preserving traversal facility together with `transpose` operations that convert a structure of contextual values into a contextual structure, and the bundled customization model it takes to do that coherently.

-   **<https://github.com/steve-downey/transpose>:** Transpose Context and Structure


<a id="org641c40a"></a>

## Algorithms for Trees

Sometimes you don't want to flatten everything to a sequence. You want to preserve the shape of the structure.

It also turns out there are **efficient** tools for building recursive data structures out of flat templated types, then folding them into a value, unfolding a value into a structure, and fusing the two so the structure is never materialized. The fold/unfold/fuse trio goes back to [*Functional Programming with Bananas, Lenses, Envelopes and Barbed Wire*](https://maartenfokkinga.github.io/utwente/mmf91m.pdf) (Meijer, Fokkinga, and Paterson, 1991).

We should standardize those tools, and provide the hooks so your current handwritten binary tree can use all the same algorithms.

Here is yesterday's tree: `unique_ptr` children, not even copyable, no `Fix` anywhere. Three ingredients, and then the fold runs over it as it is. From the [tree\_algorithms](https://github.com/steve-downey/tree_algorithms) repository's examples.

P4322, *Algorithms for Trees*, proposes generic algorithms centered on recursive structure instead of flat external iteration: fold, unfold, and the fusion of the two, plus the hooks that let a hand-written tree use them.

-   **<https://github.com/steve-downey/tree_algorithms>:** Algorithms for Trees


<a id="org537339c"></a>

## Fingertrees

A twenty-year-old data structure. [Hinze and Paterson's 2006 paper](https://www.staff.city.ac.uk/~ross/papers/FingerTree.html) is still the reference, and Haskell has shipped it as [`Data.Sequence`](https://hackage.haskell.org/package/containers/docs/Data-Sequence.html) the whole time.

That is, by the way, the same Paterson on all three foundational papers.

It's "purely functional" and "persistent", which means that operations on the tree return a new tree rather than mutating the original tree. Nonetheless, by clever sharing of state, it is still very efficient. You can hand a fingertree to a long-running job on another thread and keep working on your own copy. You don't have to pause the world while writing out state to disk.

Further, by annotating the interior nodes, many other data structures can be efficiently modeled, using monoids: a type, an associative binary operation, and an identity element. The monoidal tag accumulates a measurement of what is held below a node, and the tree is navigated by that measure. It can act as a sequence, a priority queue, or an interval map, all depending on the choice of measure.

Every container in the standard library beats a fingertree at the one thing it was built for. A fingertree is merely decent at all of them at once, and a copy is O(1). That is very hard to beat.

A `shared_ptr<const vector>` allows O(1) copy of the shared\_ptr, but the `vector` can't be modified by anyone else. A `fingertree`, or any persistent data structure, can be modified without affecting anyone holding the old one. Modifications produce a new `fingertree`, cheaply.

Split by measure, and the original is untouched. From the [fingertree](https://github.com/steve-downey/fingertree) repository's example.

P4325, *A Persistent Measured Sequence for the Standard Library*, proposes a persistent measured sequence abstraction with efficient concatenation, prefix-based search, and split operations, drawing on the finger-tree literature without committing to one implementation.

-   **<https://github.com/steve-downey/fingertree>:** Hinze & Paterson's Fingertrees


<a id="orgfb3d3e7"></a>

## Backticks For Infix

``a `f` b`` == `f(a, b)`

This is apparently either something you have always desired, or you have no idea why anyone would ever need such a thing.

C++26 added the backtick to the basic character set ([P2558](https://wg21.link/p2558)). Nothing uses it yet.

It's not the same as the `pizza` operator, `|>` ([P2011](https://wg21.link/p2011)), although they are near each other.

Infix makes pipelines built from the algebraic gadgets I am proposing easier to read. Subject-verb-object is the common order in most of the native languages programmers reading this post use. No language stacks its verbs in front the way `f(g(h(x)))` does. Naming every intermediate result just to break up a chain makes the reader keep track of state that exists only to be named.

P4307, *An Infix Operator and a Keyword Escape for C++*, proposes two uses for the backtick, the last printable ASCII character the language can still claim: any callable as a binary operator, and a keyword escaped for use as an ordinary identifier. Each is defined by rewrite into something the language already has, and they are proposed together because two independent claims on one character, designed separately, would end in contradiction.

-   **<https://github.com/steve-downey/backtick>:** Infix function notation for C++
    -   **<https://github.com/steve-downey/llvm-project/tree/backtick>:** Backticks in LLVM
    -   **<https://github.com/steve-downey/gcc/tree/backtick>:** Backticks in GCC


<a id="org5fbd580"></a>

# "Only be sure always to call it please 'Research'" &mdash; Tom Lehrer

The core ideas that run through all of this work are

1.  Higher Order Functions
2.  Internal Iteration
3.  Typeclasses

None are at all novel.

Higher order functions, functions that take functions as arguments, or return functions, are the central mechanism of Ranges and Senders, as well as being in Stepanov's STL. Separation of the mechanism, or shape, of an algorithm from the operation the algorithm performs is table stakes these days.

Iteration is also a central concern of the STL. Applying functions to or inspecting each element in some collection in some manner defined by the structure (abstracted away from the algorithm that applies a user operation) was Stepanov's key insight in defining Generic Programming. The STL exposes the iteration externally, making Iterators a key component. Algorithms, Iterators, and Containers are the triad that defines the STL. Ranges provides a bridge to internal iteration, where the iteration is not necessarily exposed.

It has turned out that external iteration, using pointers or indices, is a source of needless safety problems. Moving the details of unsafe operations, such as checking if an iterator is valid, or dereferenceable, into small and reused blocks of code improves safety, and correctness. It has also turned out that there is little to no loss of performance. Sometimes even performance improvements because the compiler can see it is lowering known safe code to unsafe implementation and can elide checks it might otherwise need to do. Compiler middle and back ends have learned an astonishing number of tricks, often driven by other languages that have made safety a priority.

Typeclasses are newer to C++ as an organizing principle, but not as an implementation technique. They are merely a record that collects a coherent set of named operations together. In a language with first class support for them the compiler or runtime will arrange to make these names available to functions constrained on them.

I am not proposing a language extension for typeclasses.

I am proposing pure and efficient library mechanisms for discovering and forwarding the bundle of named operations.

Use an object holding callables with well known names, one for each operation in a related family. A concrete interface.

Provide a generic lookup mechanism for the object so that algorithms can find the instance for the types provided to the algorithms. The typeclass instance.

Structs, with named operations. No other required indirection. No required virtual functions. No required inheritance.

Everything visible to the compiler, and available for inlining and defunctionalization.

The compiler's middle and back ends are extremely happy with them.

I am proposing some more complicated machinery to help write a typeclass instance, one that derives the rest of the operations from a few core ones. Consumers of an instance do not care. All they look for is how to perform one of the named operations the typeclass provides. How it is provided is not their problem. Purely duck typing.

The generic programming facilities of C++26 are enough to implement these abstractions type-safely. Even taking into account the effects of error handling and failures. Using these facilities also produces efficient and correct by construction code, at the cost of some debugging overhead without inlining, and some complexity in the implementation of the proposed framework for users to provide typeclass instances for their own types.

Haskell is not the only language to use typeclasses as an organizing principle. [Lean](https://lean-lang.org/), which the mathematicians and the machine learning people have both taken up lately, borrowed them as well.

> A type class describes a collection of overloadable operations. To overload these operations for a new type, an instance is created that contains an implementation of each operation for the new type. For example, a type class named `Add` describes types that allow addition, and an instance of `Add` for `Nat` provides an implementation of addition for `Nat`.

From [*Functional Programming in Lean*](https://lean-lang.org/functional_programming_in_lean/Overloading-and-Type-Classes/#type-classes).


<a id="org67aafeb"></a>

# "Why in the **Standard**"

Typeclasses are shared vocabulary. A treaty between algorithm writers, data structure writers and programmers trying to use those two together. Standardizing the generic verbs of a typeclass allows generic reuse. That is why we standardize things.

A typeclass is a very lightweight adapter. Providing the translation from the vocabulary a type would like to use for its own domain to the abstract verbs something like Traversable or Monad provides is a benefit to everyone.

Providing the typeclass also signals how the native vocabulary of the type works.

Anyone can build typeclass support for themselves. I certainly have, for years. Stepanov's insight was that making the interface between algorithms and containers a distinct third thing improved both algorithms and containers. Making the iterators what each sees turns M\*N into M+N. Making them standard made them worth providing for everything, even third party code.

We then failed to provide any support for writing iterators.

This is a running problem. We also failed to provide support for writing coroutine types, and initially failed to provide the extension point for writing pipeable range algorithms. Writing typeclasses is a first-class part of the proposals, while keeping the coupling as minimal as I could make it. There's no inherent requirement to use the facilities. Algorithms and code that consume a typeclass do not care how it is implemented.

We've also learned a lot about ABI stability and the mistakes that make evolution difficult. The lack of names in the vtable makes for silent breaks if that layout is ever changed or extended. This is one of the problems with locale facets in iostreams, as well as with `pmr::memory_resource`. Typeclasses are not virtual interfaces, and are fully typed. Changing a function in them can cause errors, and be a breaking change, but extending them can be a compatible, non-breaking change if the instance uses the implementation infrastructure, because derived operations are built from a small set of core functions. We can even add a new alternative basis operation in terms of the existing ones. Entirely new operation sets would instead be a new typeclass.

Complexity of implementation is also an issue for new standard components. There can be legitimate concerns that specialized knowledge is necessary for a high quality implementation. This often comes up in numerical processing. However, in this case, the implementation is fairly boring. It is not at all complex and well within the normal work of a standard library writer.

The typeclasses being proposed are ancient. It is highly unlikely that Monoid or Functor get new or different operations which would break everything. The implementation for a particular instance can be updated with normal care as the standard typeclasses being proposed are all stateless. User written typeclasses might have state, and that can be a useful instrumentation technique, but that is a user manageable problem.


<a id="org1fadcf1"></a>

# Not Quite What I Set Out To Do

One of the main use cases I set out to solve with all of this turned out to be better not done at all. The particular problem of Unicode normalization can require many small insertions and deletions of code points in the middle of strings, and is a worst-case scenario for vector, and string. I was originally envisioning Text types that would maintain a normalization as an invariant as the text is manipulated.

That turns out to be unnecessary.

While I still believe that a Unicode Text type would be a valuable addition to the standard, simply maintaining well-formedness of the underlying UTF encoded data is sufficient to support Unicode algorithms and interoperability with facilities and languages, such as Python 3, that have well-formed UTF-8 as a hard requirement for bringing data into their string types. Normalization is best done once, as needed, at the boundaries of the system. Canonical normalization ([UAX #15](https://unicode.org/reports/tr15/)) never changes the meaning of text, simply some details of representation. Unicode algorithms generally just do not care.

I will still be bringing the Container I had planned, since it's very widely useful, available in several different languages already, and can solve many real-world problems efficiently and safely. Just not as a necessary part of Text. Instead as one of many Containers that Text could be an adapter for.

A Text adapter is, also, not on my core road map for Unicode in C++29. Access to the primary algorithms defined by the Unicode standard is much more important, and would be, in any case, the underpinnings for a Text type.


<a id="orge709ef5"></a>

# What I want

I want excellent support in the standard for algorithms that are not just processing a sequence.

I want to take what we have learned from the STL, from std::ranges, and from std::execution, and extend the capabilities to more classes of structures.

I want C++ to be as expressive as Python, Rust, Swift, or Haskell.

I want to package into the standard the tools, facilities, and vocabulary that make all of this accessible to working programmers.

Without having to go back to University and get a Maths degree specializing in Category Theory and Programming Languages.


<a id="orgfd9a042"></a>

# When?

Papers are forthcoming.

Code exists.

Public uses of the code exist, also.

-   **<https://github.com/steve-downey/fixpoint>:** Astronautics with Kmett's recursion schemes
-   **<https://github.com/steve-downey/compile-time-scheme>:** A compile time ~~Scheme~~ Common Lisp compiler
-   **<https://github.com/steve-downey/compile-time-forth>:** A compile time Forth compiler

In order to keep this feasible for C++29 I am keeping the papers minimal and focused. It's also why there are several papers instead of one "One Typeclass Proposal" paper. This means possibly leaving some work undone, without foreclosing it. The largest gap right now is a Monad typeclass. It's clear how to do one, and what would go in it. But it's not necessary for solving the problems that any of the papers I'm working on are trying to solve.

The code is up now, and issues on any of the repos are welcome. Comments on the papers, when they land, even more so.
