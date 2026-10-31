with Ada.Streams; use Ada.Streams;
with Ada.Text_IO; use Ada.Text_IO;
with Ada.IO_Exceptions;
procedure StreamMemory is
    type Memory_Stream is new Root_Stream_Type with record
        Bytes : Stream_Element_Array (1 .. 256);
        Used : Stream_Element_Offset := 0;
        Position : Stream_Element_Offset := 1;
    end record;
    overriding procedure Read (Stream : in out Memory_Stream;
                               Item : out Stream_Element_Array;
                               Last : out Stream_Element_Offset);
    overriding procedure Write (Stream : in out Memory_Stream;
                                Item : Stream_Element_Array);
    overriding procedure Read (Stream : in out Memory_Stream;
                               Item : out Stream_Element_Array;
                               Last : out Stream_Element_Offset) is
    begin
        Last := Item'First - 1;
        for I in Item'Range loop
            exit when Stream.Position > Stream.Used;
            Item (I) := Stream.Bytes (Stream.Position);
            Stream.Position := Stream.Position + 1;
            Last := I;
        end loop;
    end Read;
    overriding procedure Write (Stream : in out Memory_Stream;
                                Item : Stream_Element_Array) is
    begin
        for I in Item'Range loop
            Stream.Used := Stream.Used + 1;
            Stream.Bytes (Stream.Used) := Item (I);
        end loop;
    end Write;
    type Number is new Integer;
    procedure Read_Number (Stream : access Root_Stream_Type'Class; Item : out Number);
    procedure Write_Number (Stream : access Root_Stream_Type'Class; Item : Number);
    for Number'Read use Read_Number;
    for Number'Write use Write_Number;
    procedure Read_Number (Stream : access Root_Stream_Type'Class; Item : out Number) is
    begin
        Item := Number (Integer'Input (Stream) - 10);
    end Read_Number;
    procedure Write_Number (Stream : access Root_Stream_Type'Class; Item : Number) is
    begin
        Integer'Write (Stream, Integer (Item) + 10);
    end Write_Number;
    type Derived_Number is new Number;
    type Pair is record
        A : Number;
        B : Derived_Number;
    end record;
    procedure Write_Integer (Stream : not null access Root_Stream_Type'Class; Item : Integer)
        renames Integer'Write;
    generic
        with procedure Emit (Stream : not null access Root_Stream_Type'Class; Item : Integer);
    procedure Save (Stream : not null access Root_Stream_Type'Class; Item : Integer);
    procedure Save (Stream : not null access Root_Stream_Type'Class; Item : Integer) is
    begin
        Emit (Stream, Item);
    end Save;
    procedure Save_Integer is new Save (Integer'Write);
    type Channel_Access is access all Root_Stream_Type'Class;
    Missing : Channel_Access;
    Buffer : aliased Memory_Stream;
    Value : Integer;
    Back : Pair;
begin
    Pair'Output (Buffer'Access, (12, 34));
    if Buffer.Used /= 8 then raise Program_Error; end if;
    Pair'Read (Buffer'Access, Back);
    if Back.A /= 12 or Back.B /= 34 then raise Program_Error; end if;
    Write_Integer (Buffer'Access, 123456);
    String'Output (Buffer'Access, "hello");
    Integer'Read (Buffer'Access, Value);
    if Value /= 123456 or String'Input (Buffer'Access) /= "hello" then
        raise Program_Error;
    end if;
    begin
        Integer'Read (Buffer'Access, Value);
        raise Program_Error;
    exception
        when Ada.IO_Exceptions.End_Error => null;
    end;
    Save_Integer (Buffer'Access, 77);
    if Integer'Input (Buffer'Access) /= 77 then raise Program_Error; end if;
    begin
        Number'Write (Missing, 1);
        raise Program_Error;
    exception
        when Constraint_Error => null;
    end;
    Put_Line ("streammemory: passed");
end StreamMemory;
