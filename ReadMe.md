# Computer Graphics Metafile

The Computer Graphics Metafile (CGM) is an ISO/ANSI standard for the
interchange of 2D picture descriptions created in 1985.  Graphical data
for interchange must conform to uniform semantics in different software
applications in order for the interchange to be useflu.  CGM defines the
semantics of the data elements in a
[functional specifiation](doc/ISO_IEC_8632-1_1999(E\) CGM Functional Specification.pdf),
and one or more element encodings.

When CGM was originally defined, three encodings were standardized: binary,
character and clear text.  The [binary](doc/ISO_IEC_8632-3_1999(E\) CGM Binary Encoding.pdf)
and [clear text](doc/ISO_IEC_8632-4_1999(E\) CGM Clear Text Encoding.pdf) encodings
are publicly documented, while the character encoding is considered obsolete.

# This Library

This library implements writing of binary and clear text encoded metafiles
as a C++ API.  The implementation is lifted from an old release of the
[GR Framework](https://github.com/sciapp/gr/tree/v0.4.0/lib/gks)
and adapted to write to a `std::ostream` instead of a C `FILE`.

Unit tests have been written in [Catch2](https://github.com/catchorg/Catch2)
to cover the supported elements in the clear text and binary encodings.

This library has limitations based on its origin:

- Output only
- No character encoding
- Some parameter byte counts for binary elements appear incorrect
- Some assumptions of precision are made
- VDC coordinates are assumed to be integer (e.g. pixel) coordinates

This library is intended for use in a historical simulations of graphic
environments and in particular is intended to serve as the basis for
reading and writing CGM files via a [GKS](http://github.com/LegalizeAdulthood/gks)
metafile workstation type.  The original code was created purely for such
a metafile output (MO) workstation in the implementation of GKS for the GR Framework.

Tt can be useful to have a generic CGM library for the purposes of
other CGM tools such as analyzers, inspectors, encoding converters, etc.

To achieve the goals beyond of a GKS MO workstation with the assumed constraints,
new code will need to be written.  However, starting from an existing implementation
is easier than starting from scratch.