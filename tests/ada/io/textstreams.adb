with Ada.Text_IO;
with Ada.Text_IO.Text_Streams;
with Ada.Streams;
with Ada.Text_IO;
procedure TextStreams is
    package T renames Ada.Text_IO;
    package S renames Ada.Text_IO.Text_Streams;
    use type T.Count;
    use type S.Stream_Access;
    use type Ada.Streams.Stream_Element;
    use type Ada.Streams.Stream_Element_Offset;
    F : T.File_Type;
    P : S.Stream_Access;
    C : Character;
    E : Boolean;
    Data : Ada.Streams.Stream_Element_Array (3 .. 10) := (65, 13, 10, 12, 233, 81, 233, 82);
    Last : Ada.Streams.Stream_Element_Offset;
    Empty : Ada.Streams.Stream_Element_Array (8 .. 7);
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then raise Program_Error; end if;
    end Check;
    procedure Read_Byte (Expected : Ada.Streams.Stream_Element) is
        Byte : Ada.Streams.Stream_Element_Array (-2 .. -2);
    begin
        Ada.Streams.Read (P.all, Byte, Last);
        Check (Last = -2 and Byte (-2) = Expected);
        Check (T.Col (F) = 1 and T.Line (F) = 1 and T.Page (F) = 1);
    end Read_Byte;
    procedure Independent is
        Local : T.File_Type;
        Channel : S.Stream_Access;
        Value : Character;
        End_Line : Boolean;
        Byte : Character;
    begin
        for Iteration in 1 .. 20 loop
            T.Create (Local);
            Channel := S.Stream (Local);
            T.Put (Local, Character'Val (65));
            Character'Write (Channel, 'B');
            Check (T.Col (Local) = 2);
            T.Reset (Local, T.In_File);
            T.Look_Ahead (Local, Value, End_Line);
            Character'Read (Channel, Byte);
            Check (Byte = 'A');
            T.Get (Local, Value);
            Check (Value = Character'Val (66) and T.Col (Local) = 2);
            T.Close (Local);
        end loop;
    end Independent;
    task type Worker;
    task body Worker is
    begin
        Independent;
    end Worker;
begin
    -- Standard/current handles are borrowed and obtaining views writes nothing.
    Check (S.Stream (T.Standard_Output) = S.Stream (T.Standard_Output));
    Check (S.Stream (T.Standard_Error) = S.Stream (T.Current_Error));
    Check (S.Stream (T.Standard_Input) = S.Stream (T.Current_Input));
    T.Create (F);
    P := S.Stream (F);
    T.Set_Output (F);
    Check (P = S.Stream (T.Current_Output));
    T.Set_Output (T.Standard_Output);
    Ada.Streams.Write (P.all, Data);
    Check (T.Col (F) = 1 and T.Line (F) = 1 and T.Page (F) = 1);
    declare
        Byte : Character;
    begin
        Character'Read (P, Byte);
        raise Program_Error;
    exception
        when T.Mode_Error => null;
    end;
    T.Reset (F, T.In_File);
    Check (P = S.Stream (F));
    T.Set_Input (F);
    declare
        Borrowed : T.File_Type := T.Current_Input;
    begin
        Check (P = S.Stream (Borrowed));
    end;
    Check (T.Is_Open (F));
    T.Set_Input (T.Standard_Input);
    T.Look_Ahead (F, C, E);
    Check (C = Character'Val (65) and not E);
    Ada.Streams.Read (P.all, Empty, Last);
    Check (Last = 7);
    Read_Byte (65);
    Check (T.End_Of_Line (F) and not T.End_Of_File (F));
    Read_Byte (13);
    Check (T.End_Of_Line (F));
    Read_Byte (10);
    Check (T.End_Of_Page (F));
    Read_Byte (12);
    T.Look_Ahead (F, C, E);
    Check (C = Character'Val (233) and not E);
    Read_Byte (233);
    Read_Byte (81);
    T.Look_Ahead (F, C, E);
    Check (C = Character'Val (233) and not E);
    Read_Byte (233);
    T.Get (F, C);
    Check (C = Character'Val (82) and T.Col (F) = 2 and T.Line (F) = 1 and T.Page (F) = 1);
    begin
        Ada.Streams.Write (P.all, Empty);
        raise Program_Error;
    exception
        when T.Mode_Error => null;
    end;
    T.Reset (F);
    declare
        Byte : Character;
    begin
        Character'Read (P, Byte);
        Check (Byte = 'A' and T.Col (F) = 1);
    end;
    T.Look_Ahead (F, C, E);
    T.Reset (F, T.Out_File);
    Check (P = S.Stream (F));
    Character'Write (P, 'X');
    T.Put (F, Character'Val (89));
    Check (T.Col (F) = 2);
    T.Reset (F, T.In_File);
    declare
        Byte : Character;
    begin
        Character'Read (P, Byte);
        Check (Byte = 'X');
    end;
    T.Get (F, C);
    Check (C = Character'Val (89));
    T.Close (F);
    begin
        P := S.Stream (F);
        raise Program_Error;
    exception
        when T.Status_Error => null;
    end;
    declare
        Workers : array (1 .. 4) of Worker;
    begin
        null;
    end;
    Ada.Text_IO.Put_Line ("TextStreams passed");
end TextStreams;
