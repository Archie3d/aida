with Ada.Text_IO; use Ada.Text_IO;
procedure TextIOLibrary is
    file : File_Type;
    c : Character;
    done, available : Boolean;
    text : String (7 .. 9);
    last : Natural;
    checks : Natural := 0;
    procedure check (ok : Boolean) is
    begin
        checks := checks + 1;
        if not ok then
            raise Program_Error with Integer'Image (checks);
        end if;
    end check;
begin
    Create (file);
    check (Col (file) = 1 and Line (file) = 1 and Page (file) = 1);
    Set_Line_Length (file, 3);
    Set_Page_Length (file, 2);
    check (Line_Length (file) = 3 and Page_Length (file) = 2);
    Put (file, "abcd");
    check (Col (file) = 2 and Line (file) = 2);
    New_Line (file);
    check (Col (file) = 1 and Line (file) = 1 and Page (file) = 2);
    Put_Line (file, "xy");
    Set_Col (file, 3);
    Put (file, 'z');
    check (Col (file) = 4);
    begin
        Set_Col (file, 4);
        raise Program_Error;
    exception
        when Layout_Error => null;
    end;
    New_Page (file);
    check (Page (file) = 3 and Line (file) = 1);
    Flush (file);
    Reset (file, In_File);
    Look_Ahead (file, c, done);
    check (not done and c = 'a' and Col (file) = 1);
    Get_Line (file, text, last);
    check (text = "abc" and last = 9 and End_Of_Line (file));
    check (Line (file) = 1 and Col (file) = 4);
    Get_Line (file, text (7 .. 6), last);
    check (last = 6 and Line (file) = 1 and End_Of_Line (file));
    Get_Line (file, text, last);
    check (last = 6 and Line (file) = 2);
    check (Get_Line (file) = "d");
    check (Page (file) = 2 and Line (file) = 1);
    check (Get_Line (file) = "xy");
    Get (file, c);
    check (c = ' ' and Col (file) = 2);
    Look_Ahead (file, c, done);
    check (not done and c = ' ' and Col (file) = 2);
    Get_Immediate (file, c, available);
    check (available and c = ' ' and Col (file) = 2);
    Get_Immediate (file, c);
    check (c = 'z' and Col (file) = 2);
    check (End_Of_Page (file) and End_Of_File (file));
    Skip_Page (file);
    check (Page (file) = 3 and End_Of_File (file));
    begin
        Get_Immediate (file, c, available);
        raise Program_Error;
    exception
        when End_Error => null;
    end;
    Close (file);
    Create (file);
    Set_Output (file);
    Set_Line_Length (4);
    Set_Page_Length (3);
    Put ("ab");
    check (Col = 3 and Line = 1 and Page = 1);
    check (Line_Length = 4 and Page_Length = 3);
    Set_Col (4);
    Put ('c');
    Set_Line (2);
    New_Page;
    Flush;
    Set_Error (file);
    declare
        saved : constant File_Type := Current_Error;
    begin
        Put (saved, "error");
    end;
    check (Is_Open (file));
    Set_Error (Standard_Error);
    Set_Output (Standard_Output);
    Reset (file, In_File);
    Set_Input (file);
    check (Get_Line = "ab c");
    check (Page (file) = 2);
    Reset (file);
    Skip_Page;
    Look_Ahead (c, done);
    check (not done and c = 'e');
    Get_Immediate (c, available);
    check (available and c = 'e');
    Get_Immediate (c);
    check (c = 'r');
    Set_Input (Standard_Input);
    Close (file);
    declare
        big : String (1 .. 5000) := (others => 'x');
    begin
        big (2500) := Character'Val (0);
        Create (file);
        Put_Line (file, big);
        Reset (file, In_File);
        check (Get_Line (file) = big);
        Close (file);
    end;
    Put_Line ("textiolibrary: passed");
end TextIOLibrary;
