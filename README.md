proto-constraint
================

Constraints to C++ (prototype)

Example:

    constraint::Value<int> x, y;
    rel(x + x == y && y + 1 == x - 1);
    std::cout << x << ", " << y; // -2, -4

Missing features:

- copy & move for `constraint::Value<T>`
- support for floating point values
- support for user defined types
