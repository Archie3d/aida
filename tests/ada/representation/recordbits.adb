with Ada.Text_IO; use Ada.Text_IO;
with System;
with Ada.Streams.Stream_IO;
procedure RecordBits is
    function Byte_At (Address : System.Address; Bytes, Signed : Integer) return Long_Integer;
    pragma Import (C, Byte_At, "__test_size_value");

    type Number is new Integer;
    subtype Signed_Seven is Number range -64 .. 63;
    procedure Write_Signed
        (Stream : access Ada.Streams.Root_Stream_Type'Class; Item : Number);
    procedure Read_Signed
        (Stream : access Ada.Streams.Root_Stream_Type'Class; Item : out Number);
    for Number'Write use Write_Signed;
    for Number'Read use Read_Signed;
    Writes, Reads : Natural := 0;
    procedure Write_Signed
        (Stream : access Ada.Streams.Root_Stream_Type'Class; Item : Number) is
    begin
        Writes := Writes + 1;
        Integer'Write (Stream, Integer (Item) + 64);
    end Write_Signed;
    procedure Read_Signed
        (Stream : access Ada.Streams.Root_Stream_Type'Class; Item : out Number) is
    begin
        Reads := Reads + 1;
        Item := Number (Integer'Input (Stream) - 64);
    end Read_Signed;
    type Code is (Low, Middle, High);
    for Code use (Low => -3, Middle => 1, High => 3);
    type Word is mod 2 ** 64;
    type Five_Bits is mod 32;
    type Packed is record
        Flag : Boolean := True;
        Signed : Signed_Seven := -2;
        Kind : Code := Low;
        Data : Word := Word'Last;
        Guard : Five_Bits := 31;
    end record;
    for Packed use record
        Flag   at 0 range 0 .. 0;
        Signed at 0 range 1 .. 7;
        Kind   at 0 range 8 .. 10;
        Data   at 0 range 11 .. 74;
        Guard  at 0 range 75 .. 79;
    end record;
    type Derived is new Packed;
    for Derived use record
        Data at 1 range 3 .. 66;
    end record;
    type Items is array (1 .. 2) of Packed;

    type Unaligned is record
        Head : Character;
        Number : Long_Integer;
        Tail : Character;
    end record;
    for Unaligned use record
        Head   at 0 range 0 .. 7;
        Number at 1 range 0 .. 63;
        Tail   at 9 range 0 .. 7;
    end record;
    procedure Native_Set (Value : out Long_Integer);
    pragma Import (C, Native_Set, "__test_size_set_long");

    type Variant (Flag : Boolean := False) is record
        case Flag is
            when False => Small : Signed_Seven := -8;
            when True => Other : Signed_Seven := 18;
        end case;
    end record;
    for Variant use record
        Flag  at 0 range 0 .. 0;
        Small at 0 range 1 .. 7;
        Other at 0 range 1 .. 7;
    end record;
    subtype True_Variant is Variant (True);

    procedure Check is
        A : Packed;
        Alias : Signed_Seven renames A.Signed;
        generic
            Item : in out Signed_Seven;
        procedure Set_Item;
        procedure Set_Item is
        begin
            Item := -64;
        end Set_Item;
        procedure Set_Alias is new Set_Item (Alias);
        procedure Increment (Item : in out Signed_Seven) is
        begin
            Item := Item + 1;
        end Increment;
        Row : Items := (A, A);
        Copy : Derived := Derived (A);
        U : Unaligned := ('A', 12345, 'Z');
        V : Variant;
        Fixed : True_Variant;
        Rejected : Boolean := False;
    begin
        if Packed'Size /= 80 or else Packed'Alignment /= 1 or else
           A.Kind'Position /= 1 or else A.Kind'First_Bit /= 0 or else
           A.Data'Position /= 1 or else A.Data'First_Bit /= 3 or else
           A.Data'Last_Bit /= 66 or else A.Data'Size /= 64 or else
           A.Signed'Size /= 7 or else Alias'Size /= 7 or else A.Kind'Alignment /= 1 or else
           Byte_At (A'Address, 1, 0) /= 253 then
            raise Program_Error;
        end if;
        Set_Alias;
        Increment (A.Signed);
        Row (2).Data := 16#8000000000000001#;
        A.Flag := False;
        A.Kind := High;
        A.Data := 0;
        if Alias /= -63 or else A.Flag or else A.Kind /= High or else
           A.Data /= 0 or else A.Guard /= 31 or else
           Row (1).Data /= Word'Last or else Row (2).Data /= 16#8000000000000001# or else
           Row (2).Guard /= 31 or else Copy /= Derived (Row (1)) then
            raise Program_Error;
        end if;
        Native_Set (U.Number);
        if U.Number /= -99 or else U.Head /= 'A' or else U.Tail /= 'Z' or else
           U.Number'Position /= 1 or else Unaligned'Size /= 80 then
            raise Program_Error;
        end if;
        if V.Small /= -8 or else Fixed.Other /= 18 then
            raise Program_Error;
        end if;
        V := (True, 24);
        Fixed := V;
        begin
            V := (False, -5);
            Fixed := V;
        exception
            when Constraint_Error => Rejected := True;
        end;
        if not Rejected or else Fixed.Other /= 24 or else not Fixed.Flag then
            raise Program_Error;
        end if;
    end Check;
begin
    Check;
    declare
        package Files renames Ada.Streams.Stream_IO;
        File : Files.File_Type;
        A : Packed := (False, -33, High, 16#8000000000000001#, 19);
        B : Packed;
    begin
        Files.Create (File, Files.Out_File, "recordbits.tmp");
        Packed'Write (Files.Stream (File), A);
        Files.Close (File);
        Files.Open (File, Files.In_File, "recordbits.tmp");
        Packed'Read (Files.Stream (File), B);
        Files.Delete (File);
        if B /= A or else Writes /= 1 or else Reads /= 1 then
            raise Program_Error;
        end if;
    end;
    declare
        task type Worker;
        task body Worker is
        begin
            Check;
            Put_Line ("packed record OK");
        end Worker;
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
end RecordBits;
