#ifndef HOBBIT_BIN_FORMAT_HPP
#define HOBBIT_BIN_FORMAT_HPP

// Shared serialized records reconstructed from the PC BinIn and BinOut bodies.
// This header path and these record/member names are descriptive; no original
// BinIn/BinOut header has been found in the local sibling source snapshots.
// BinOut endian conversion supports unsigned indices, sizes and file offsets.
// Count stays signed; unknown words have no established signedness or semantics.
namespace bin_format {
    struct block {
        unsigned int NameIndex;
        int Count;
        unsigned int NextBlockOffset;
        int UnknownWord12;
    };

    struct row {
        unsigned int ByteSize;
        int UnknownWord4;
    };

    // The writer's separate 12-byte i_field adds an in-memory data pointer.
    struct field {
        unsigned int NameIndex;
        unsigned int ByteSize;
    };
} // namespace bin_format

#endif
