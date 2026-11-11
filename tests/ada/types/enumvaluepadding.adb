with Ada.Text_IO;
procedure Enumvaluepadding is
    type Colour is (Red, Green, Blue);
    type Letter is (' ', 'a', 'A');
    Tab : constant Character := ASCII.HT;
    procedure Check (Condition : Boolean) is
    begin
        if not Condition then
            raise Program_Error;
        end if;
    end Check;
    procedure Exercise (Text : String; Valid : Boolean) is
        Wide : Wide_String (Text'Range);
        Wider : Wide_Wide_String (Text'Range);
        Result : Colour;
    begin
        for I in Text'Range loop
            Wide (I) := Wide_Character'Val (Character'Pos (Text (I)));
            Wider (I) := Wide_Wide_Character'Val (Character'Pos (Text (I)));
        end loop;
        begin
            Result := Colour'Value (Text);
            Check (Valid and Result = Green);
        exception
            when Constraint_Error => Check (not Valid);
        end;
        begin
            Result := Colour'Wide_Value (Wide);
            Check (Valid and Result = Green);
        exception
            when Constraint_Error => Check (not Valid);
        end;
        begin
            Result := Colour'Wide_Wide_Value (Wider);
            Check (Valid and Result = Green);
        exception
            when Constraint_Error => Check (not Valid);
        end;
    end Exercise;
    Shifted : String (5 .. 13) := "  gReEn  ";
begin
    Exercise ("Green", True);
    Exercise (" Green", True);
    Exercise ("Green ", True);
    Exercise (Shifted, True);
    Exercise (Tab & "Green", False);
    Exercise ("Green" & Tab, False);
    Exercise (" " & Tab & "Green ", False);
    Exercise (" Green" & Tab & " ", False);
    Exercise ("", False);
    Exercise ("   ", False);
    Check (Boolean'Value (" true "));
    Check (Letter'Value ("  ' '  ") = ' ');
    Check (Letter'Value (" 'a' ") = 'a');
    Check (Letter'Value (" 'A' ") = 'A');
    Ada.Text_IO.Put_Line ("enumeration padding passed");
end Enumvaluepadding;
