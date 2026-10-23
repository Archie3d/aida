with Ada.Finalization;
with Ada.Streams.Stream_IO;
procedure DiscriminantComponentErrors is
    type Bad_Expression (D : Natural) is record
        Text : String (1 .. D + 1);
    end record;
    type Bad_Scalar (D : Integer) is record
        Value : Integer range 0 .. D;
    end record;
    type Guard is new Ada.Finalization.Controlled with null record;
    type Guards is array (Integer range <>) of Guard;
    type Bad_Controlled (D : Natural) is record
        Parts : Guards (1 .. D);
    end record;
    type Pair (Left, Right : Integer) is null record;
    X : Pair (Left => 1, Left => 2);
    Y : Pair (Unknown => 1, Right => 2);
    Z : Pair (Left => 1, 2);
    type Buffer (D : Natural := 2) is record
        Text : String (1 .. D);
    end record;
    type Tagged_Buffer is tagged record
        Value : Buffer;
    end record;
    type Limited_Buffer (D : Natural) is limited record
        Text : String (1 .. D);
    end record;
    type Wide is range 1 .. 9223372036854775807;
    type Huge is array (Wide range 1 .. 9223372036854775752) of Character;
    type Overflow (D : Natural) is record
        Text : String (1 .. D);
        Padding : Huge;
    end record;
    B : Buffer;
    Stream : Ada.Streams.Stream_IO.Stream_Access;
begin
    Buffer'Write (Stream, B);
end DiscriminantComponentErrors;
