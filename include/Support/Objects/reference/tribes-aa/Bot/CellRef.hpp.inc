#ifndef CELLREF_HPP
#define CELLREF_HPP

struct cell_attribute
{
    cell_attribute( void )
    {
        BLOCKED = 0;
        VISIBLE = 0;
    }

    byte BLOCKED:1;
    byte VISIBLE:1;
};

struct cell_ref
{
    s32 X;
    s32 Z;
    cell_attribute ATTRIBUTES;
                cell_ref    ( void ) { X = 0; Z = 0; };
                cell_ref    ( s32 InitX, s32 InitZ ) { X = InitX; Z = InitZ; };
    cell_ref    operator+   ( const cell_ref& RHS ) const { return cell_ref( X + RHS.X, Z + RHS.Z ); };
    s32         operator==  ( const cell_ref& RHS ) const { return X == RHS.X && Z == RHS.Z; };
    s32         operator!=  ( const cell_ref& RHS ) const { return !(*this == RHS); };
};
#endif //ndef CELLREF_HPP
