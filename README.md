# J1939 Library

Library to work with J1939 Frames used in CAN bus in car and trucks industries

## Coding style

The whole software is programmed using the Linux kernel coding style, see
http://lxr.linux.no/linux/Documentation/CodingStyle for details.

## Amendments to the Linux kernel coding style

1. Use stdint types.

2. Always add brackets around bodies of if, while and for statements, even
   if the body contains only one expression. It is dangerous to not have them
   as it easily happens that one adds a second expression and is hunting for
   hours why the code is not working just because of a missing bracket pair.

3. Use `size_t` for array indexes, for length values and similar.

4. Use C99 index declaration inside for loop body where possible. This will ensure
   that index will live within the loop scope and not outside.

## Semantic versioning

Software is numbered according to (Semantic versioning 2.0.0)[https://semver.org] rules. 
Semantic versioning uses a sequence of three digits (Major.Minor.Patch),
an optional prerelease tag and optional build meta tag.

## Documentation

Doxygen is used to generate API docs so it is mandatory to follow that
style for function and definition commentary where possible.

## WARNING

WARNING: Currently this project is in alpha-state! Some of the features are
not completely working!

## License

Licensed under Apache License, Version 2.0 ([LICENSE-APACHE](LICENSE-APACHE) or
  http://www.apache.org/licenses/LICENSE-2.0)


### Contribution

Unless you explicitly state otherwise, any contribution intentionally submitted for
inclusion in the work by you, as defined in the Apache-2.0 license, without any
additional terms or conditions.

Please do not ask for features, but send a Pull Request instead.
