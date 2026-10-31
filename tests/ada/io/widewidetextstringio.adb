with Ada.Wide_Wide_Text_IO; use Ada.Wide_Wide_Text_IO;
with Ada.Wide_Wide_Text_IO.Wide_Wide_Bounded_IO;
with Ada.Wide_Wide_Text_IO.Wide_Wide_Unbounded_IO;
with Ada.Strings;
with Ada.Strings.Wide_Wide_Bounded;
with Ada.Strings.Wide_Wide_Unbounded;
procedure widewideTextStringIO is
    package B is new Ada.Strings.Wide_Wide_Bounded.Generic_Bounded_Length (8);
    package BI is new Ada.Wide_Wide_Text_IO.Wide_Wide_Bounded_IO (B);
    package U renames Ada.Strings.Wide_Wide_Unbounded;
    package UI renames Ada.Wide_Wide_Text_IO.Wide_Wide_Unbounded_IO;
    use type B.Bounded_Wide_Wide_String;
    use type U.Unbounded_Wide_Wide_String;
    file : File_Type;
    bounded : B.Bounded_Wide_Wide_String := B.To_Bounded_Wide_Wide_String ("bounded");
    unbounded : U.Unbounded_Wide_Wide_String := U.To_Unbounded_Wide_Wide_String ("unbounded");
    procedure check (ok : Boolean) is
    begin
        if not ok then
            raise Program_Error;
        end if;
    end check;
begin
    Create (file);
    BI.Put_Line (file, bounded);
    UI.Put_Line (file, unbounded);
    Set_Output (file);
    BI.Put (B.To_Bounded_Wide_Wide_String ("a"));
    BI.Put_Line (B.To_Bounded_Wide_Wide_String ("b"));
    UI.Put (U.To_Unbounded_Wide_Wide_String ("c"));
    UI.Put_Line (U.To_Unbounded_Wide_Wide_String ("d"));
    Set_Output (Standard_Output);
    Reset (file, In_File);
    BI.Get_Line (file, bounded);
    UI.Get_Line (file, unbounded);
    check (bounded = "bounded" and unbounded = "unbounded");
    Set_Input (file);
    check (BI.Get_Line = "ab");
    check (UI.Get_Line = "cd");
    Set_Input (Standard_Input);
    Close (file);
    Create (file);
    Put_Line (file, "too long for the bounded string");
    Put_Line (file, "next");
    Reset (file, In_File);
    begin
        BI.Get_Line (file, bounded);
        raise Program_Error;
    exception
        when Ada.Strings.Length_Error => null;
    end;
    check (bounded = "bounded" and UI.Get_Line (file) = "next");
    Close (file);
    Put_Line ("textstringio: passed");
end widewideTextStringIO;
