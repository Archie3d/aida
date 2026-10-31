with Ada.Streams.Stream_IO;
with Ada.Text_IO; use Ada.Text_IO;
with Ada.Finalization;
procedure StreamRecords is
    type Buffer (Length : Natural := 2) is record
        Text : String (1 .. Length) := (others => '?');
    end record;
    type Fixed_Buffer (Length : Natural) is record
        Text : String (1 .. Length);
    end record;
    type Guard is new Ada.Finalization.Controlled with record
        Value : Integer := 17;
    end record;
    type Pair is record
        First : Guard;
        Last : Integer := 4;
    end record;
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
    A : Buffer := (3, "abc");
    B : Buffer;
    C : Buffer (2);
    D : Fixed_Buffer (3) := (3, "xyz");
    P : Pair;
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "streamrecords.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Buffer'Write (Channel, A);
    Buffer'Write (Channel, A);
    Fixed_Buffer'Output (Channel, D);
    Pair'Output (Channel, P);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "streamrecords.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Buffer'Read (Channel, B);
    if B.Length /= 3 or B.Text /= "abc" then raise Program_Error; end if;
    begin
        Buffer'Read (Channel, C);
        raise Program_Error;
    exception
        when Constraint_Error =>
            if C.Length /= 2 or C.Text /= "??" then raise Program_Error; end if;
    end;
    declare
        Back : Fixed_Buffer := Fixed_Buffer'Input (Channel);
        Other : Pair := Pair'Input (Channel);
    begin
        if Back.Length /= 3 or Back.Text /= "xyz" or Other.First.Value /= 17
            or Other.Last /= 4 then raise Program_Error; end if;
    end;
    Ada.Streams.Stream_IO.Delete (File);
    Put_Line ("streamrecords: passed");
end StreamRecords;
