with Ada.Streams.Stream_IO;
with Ada.Text_IO;
procedure RuntimeComponentStreams is
    N : Integer := 3;
    subtype Text is String (2 .. N);
    type Matrix is array (1 .. N, 4 .. N + 3) of Integer;
    type Fixed is array (1 .. 2) of Integer;
    type Open_Array is array (Integer range <>) of Integer;
    type Wide_Array is array (Long_Integer range <>) of Integer;
    type Rows is array (1 .. 2) of Text;
    type Holder is record
        First : Integer;
        Value : Text;
        Data : Rows;
        Last : Long_Integer;
    end record;
    A : Text := "ab";
    B : Text := "cd";
    M : Matrix := (others => (others => 17));
    R : Rows := (others => "ef");
    H : Holder := (7, "gh", (others => "ij"), 99);
    File : Ada.Streams.Stream_IO.File_Type;
    Channel : Ada.Streams.Stream_IO.Stream_Access;
begin
    Ada.Streams.Stream_IO.Create (File, Ada.Streams.Stream_IO.Out_File, "runtimecomponentstreams.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    Text'Output (Channel, A);
    Text'Write (Channel, B);
    Matrix'Output (Channel, M);
    Rows'Output (Channel, R);
    Holder'Output (Channel, H);
    Text'Output (Channel, A);
    String'Output (Channel, String'(7 .. 8 => 'z'));
    Open_Array'Output (Channel, Open_Array'(1 .. 3 => 0));
    -- A second mismatched shape, using Fixed's Integer bound stream format.
    Integer'Write (Channel, 0);
    Integer'Write (Channel, 2);
    Ada.Streams.Stream_IO.Close (File);
    Ada.Streams.Stream_IO.Open (File, Ada.Streams.Stream_IO.In_File, "runtimecomponentstreams.tmp");
    Channel := Ada.Streams.Stream_IO.Stream (File);
    declare
        Back : Text := Text'Input (Channel);
        Other : Text;
        Grid : Matrix;
        Nested : Rows;
        Record_Back : Holder;
    begin
        Text'Read (Channel, Other);
        Grid := Matrix'Input (Channel);
        Nested := Rows'Input (Channel);
        Record_Back := Holder'Input (Channel);
        if Back /= A or Other /= B or Grid /= M or Nested /= R or Record_Back /= H
            or Back'First /= 2 then raise Program_Error; end if;
        declare
            Base_View : String := String'Input (Channel);
            Subtype_View : String := Text'Input (Channel);
        begin
            if Base_View'First /= 2 or Base_View /= A or Subtype_View'First /= 7
                or Subtype_View /= "zz" then raise Program_Error; end if;
        end;
    end;
    for Attempt in 1 .. 2 loop
        begin
            declare
                Invalid : Fixed := Fixed'Input (Channel);
            begin
                raise Program_Error;
            end;
        exception
            when Constraint_Error => null;
        end;
        -- The first check consumed the bounds, not its three payload elements.
        if Attempt = 1 then
            declare
                Skip : Integer;
            begin
                for I in 1 .. 3 loop Integer'Read (Channel, Skip); end loop;
            end;
        end if;
    end loop;
    Ada.Streams.Stream_IO.Delete (File);
    Ada.Text_IO.Put_Line ("runtime component streams ok");
end RuntimeComponentStreams;
