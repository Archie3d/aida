with Ada.Text_IO; use Ada.Text_IO;
with Primitive_Child;
procedure PrimitiveSeparate is
    use all type Primitive_Child.T;
    use all type Primitive_Child.Leaf;
    X : Primitive_Child.T := Make (2);
    Y : Primitive_Child.T := Make (4);
    A : Primitive_Child.Leaf := Make (3);
    B : Primitive_Child.Leaf := Make (5);
begin
    Put_Line (Boolean'Image (X = Y));
    Put_Line (Boolean'Image (X /= Y));
    Put_Line (Boolean'Image (A = B));
    Put_Line (Boolean'Image (A /= B));
    Put_Line (Integer'Image (Read (X)));
end PrimitiveSeparate;
