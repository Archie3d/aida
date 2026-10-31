with Ada.Text_IO; use Ada.Text_IO;
procedure TextGetStrings is
    File : File_Type;
    Empty : String (4 .. 3);
    Text : String (7 .. 11);
    Buffer : String (10 .. 15) := (others => '#');
    Letter : Character;
    Caught : Boolean;
    procedure Check (Value : Boolean) is
    begin
        if not Value then
            raise Program_Error;
        end if;
    end Check;
    protected Results is
        procedure Done (Good : Boolean);
        function Passed return Boolean;
    private
        Count : Natural := 0;
        Errors : Natural := 0;
    end Results;
    protected body Results is
        procedure Done (Good : Boolean) is
        begin
            Count := Count + 1;
            if not Good then
                Errors := Errors + 1;
            end if;
        end Done;
        function Passed return Boolean is
        begin
            return Count = 4 and Errors = 0;
        end Passed;
    end Results;
    procedure Exercise_Tasks is
        task type Worker;
        task body Worker is
            Data : File_Type;
            Item : String (5 .. 8);
        begin
            for Round in 1 .. 10 loop
                Create (Data);
                Put_Line (Data, "AB");
                Put_Line (Data, "CD");
                Reset (Data, In_File);
                Get (Data, Item);
                Check (Item = "ABCD");
                Delete (Data);
            end loop;
            Results.Done (True);
        exception
            when others => Results.Done (False);
        end Worker;
        type Group is array (1 .. 4) of Worker;
        Workers : Group;
    begin
        null;
    end Exercise_Tasks;
begin
    -- A null string performs zero character reads, even on a closed file.
    Get (File, Empty);
    Caught := False;
    begin
        Get (File, Text);
    exception
        when Status_Error => Caught := True;
    end;
    Check (Caught);
    Create (File);
    Get (File, Empty);
    Caught := False;
    begin
        Get (File, Text);
    exception
        when Mode_Error => Caught := True;
    end;
    Check (Caught);
    New_Line (File);
    Put_Line (File, "a b");
    Put (File, Character'Val (12));
    Put (File, Character'Val (0));
    Put (File, "cd");
    Reset (File, In_File);
    Get (File, Empty);
    Get (File, Text);
    Check (Text = "a b" & Character'Val (0) & 'c');
    Get (File, Letter);
    Check (Letter = 'd');
    Get (File, Empty);
    Check (End_Of_File (File));
    Caught := False;
    begin
        Get (File, Text);
    exception
        when End_Error => Caught := True;
    end;
    Check (Caught);
    Delete (File);
    Create (File);
    Put_Line (File, "AB");
    Put_Line (File, "CD");
    Reset (File, In_File);
    Set_Input (File);
    Get (Empty);
    Get (Buffer (12 .. 13));
    Check (Buffer = "##AB##" and End_Of_Line);
    Get (Buffer (12 .. 13));
    Check (Buffer = "##CD##" and End_Of_Line);
    Get (Empty);
    Caught := False;
    begin
        Get (Buffer);
    exception
        when End_Error => Caught := True;
    end;
    Check (Caught);
    Set_Input (Standard_Input);
    Delete (File);
    -- Too little input must raise rather than return a short successful read.
    Create (File);
    Put (File, "xy");
    Reset (File, In_File);
    Caught := False;
    begin
        Get (File, Text);
    exception
        when End_Error => Caught := True;
    end;
    Check (Caught and End_Of_File (File));
    Delete (File);
    Exercise_Tasks;
    Check (Results.Passed);
    Put_Line ("string Get passed");
end TextGetStrings;
