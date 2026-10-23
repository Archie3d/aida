with Ada.Unchecked_Deallocation;
package DiscriminantData is
    Bound : Positive := 3;
    type Buffer (Length : Natural := 2) is record
        Text : String (1 .. Length) := (others => 'x');
    end record;
    subtype Fixed is Buffer (Bound);
    Global : Fixed;
    type Ref is access Buffer;
    procedure Free is new Ada.Unchecked_Deallocation (Buffer, Ref);
    procedure Set (Target : in out Buffer; Source : Buffer);
    function Copy (X : Buffer) return Buffer;
end DiscriminantData;
