\# Clean Code Rules for AI Code Generation


\## Meaningful Names
\- Use intention-revealing names that explain why something exists
\- Avoid disinformation and meaningless distinctions (e.g., `data`, `info`, `manager`)
\- Use pronounceable, searchable names
\- Class names: nouns (e.g., `UserAccount`, `PaymentProcessor`)
\- Method names: verbs (e.g., `calculateTotal`, `sendEmail`)
\- Avoid mental mapping and encodings (Hungarian notation, prefixes)


\## Functions
\- Keep functions small (< 100 lines ideal), dont break functions just to meet this metric. If things belong together, keep them
\- Do one thing only - Single Responsibility Principle
\- One level of abstraction per function
\- Limit arguments: 0-2 ideal, 3 maximum, avoid flag arguments. Ignore this for rendering functions, those are notorious for requiring more arguments, still try to cap those at 10.
\- No side effects - function should do what its name says
\- Separate commands (change state) from queries (return info)
\- Avoid one letter variables


\## Comments
\- Code should be self-explanatory - avoid comments when possible
\- Good comments: legal info, warnings, TODOs, public API documentation
\- Bad comments: redundant, misleading, or explaining bad code
\- Never comment out code - delete it (version control preserves history)
\- If you need a comment, consider refactoring the code instead


\## Formatting
\- Keep files small and focused
\- Vertical formatting: related concepts close together, blank lines separate concepts
\- Horizontal formatting: limit line length (160 characters)
\- Use consistent indentation and team style
\- Group related functions together


\## Objects and Data Structures
\- Objects: hide data behind abstractions, expose behavior through methods
\- Data structures: expose data, have minimal behavior
\- Law of Demeter: only talk to immediate friends, avoid `a.getB().getC().doSomething()`
\- Don't expose internal structure through getters/setters blindly
\- Use const when possible
\- Avoid lambdas


\## Error Handling
\- Use exceptions, not return codes or error flags. Only if something breaks
\- Write `try-catch-finally` first when code might fail
\- Provide context in exception messages
\- Don't return `null` - return empty collections or use Optional/Maybe
\- Don't pass `null` as arguments


\## Classes
\- Small classes: measured by responsibilities, not lines
\- Single Responsibility Principle: one reason to change
\- High cohesion: class variables used by many methods
\- Low coupling: minimal dependencies between classes
\- Open/Closed Principle: open for extension, closed for modification

\## Unit Tests
\- Fast, Independent, Repeatable, Self-validating, Timely (F.I.R.S.T.)
\- One assert per test (or one concept)
\- Test code quality equals production code quality
\- Readable test names that describe what's being tested
\- Arrange-Act-Assert pattern


\## Code Quality Principles
\- \*\*DRY (Don't Repeat Yourself)\*\*: No duplication
\- \*\*YAGNI (You Aren't Gonna Need It)\*\*: Don't build for hypothetical futures. Unless requested or the original application had it.
\- \*\*KISS (Keep It Simple)\*\*: Avoid unnecessary complexity
\- \*\*Boy Scout Rule\*\*: Leave code cleaner than you found it


\## Code Smells to Avoid
\- Long functions or classes
\- Duplicate code
\- Dead code (unused variables, functions, parameters)
\- Feature envy (method more interested in other class)
\- Inappropriate intimacy (classes knowing too much about each other)
\- Long parameter lists
\- Primitive obsession (overusing primitives instead of small objects)
\- Switch/case statements (consider polymorphism)
\- Temporary fields (class variables only used sometimes)


\## Concurrency
\- Keep concurrent code separate from other code
\- Limit scope of synchronized/locked data
\- Use thread-safe collections
\- Keep synchronized sections small
\- Know your execution models and primitives


\## System Design
\- Separate construction from use (dependency injection)
\- Use factories, builders for complex object creation
\- Program to interfaces, not implementations
\- Favor composition over inheritance
\- Apply design patterns when they simplify, not to show off
\- Careful with memory locality, focus on performance. Avoid cache missies


\## Refactoring
\- Refactor continuously, not in big batches
\- Always have passing tests before and after
\- Small steps: one change at a time
\- Common refactorings: Extract Method, Rename, Move, Inline


\## Documentation
\- Self-documenting code > comments > external docs
\- Public APIs need clear documentation
\- Keep docs close to code (ideally in code)


\*\*Core Philosophy\*\*: Code is read 10x more than written. Optimize for readability and maintainability, not cleverness.



