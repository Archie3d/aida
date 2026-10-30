with Ada.Text_IO; use Ada.Text_IO;
with Ada.Long_Integer_Text_IO;
with Ada.Long_Float_Text_IO;
with Ada.Text_IO.Integer_IO;
with Ada.Text_IO.Float_IO;
with Ada.Text_IO.Enumeration_IO;
with Ada.Text_IO.Fixed_IO;
with Ada.Text_IO.Decimal_IO;
with Ada.Text_IO.Modular_IO;
procedure TextInputGrammar is
    package I renames Ada.Long_Integer_Text_IO;
    type Small_Real is digits 6 range -10.0 .. 10.0;
    package R is new Ada.Text_IO.Float_IO (Small_Real);
    type Fixed is delta 0.125 range -100.0 .. 100.0;
    package F is new Ada.Text_IO.Fixed_IO (Fixed);
    type Decimal is delta 0.01 digits 6;
    package D is new Ada.Text_IO.Decimal_IO (Decimal);
    type Word is mod 2 ** 64;
    package M is new Ada.Text_IO.Modular_IO (Word);
    type Colour is (Red, Green, Blue);
    package E is new Ada.Text_IO.Enumeration_IO (Colour);
    package C is new Ada.Text_IO.Enumeration_IO (Character);
    file : File_Type;
    integerValue : Long_Integer;
    realValue : Small_Real;
    fixedValue : Fixed;
    decimalValue : Decimal;
    modularValue : Word;
    colourValue : Colour;
    characterValue : Character;
    last : Positive;
    outText : String (7 .. 14);
    longText : String (1 .. 2000) := (others => '0');
    procedure check (ok : Boolean) is
    begin
        if not ok then
            raise Program_Error;
        end if;
    end check;
begin
    I.Get ("  -16#7F#E+1!", integerValue, last);
    check (integerValue = -2032 and last = 12);
    declare
        text : String (Integer'Last - 4 .. Integer'Last) := "  42!";
    begin
        I.Get (text, integerValue, last);
        check (integerValue = 42 and last = Integer'Last - 1);
    end;
    R.Get (".5,-2", realValue, last);
    check (realValue = 0.5 and last = 2);
    R.Get ("16#A.#", realValue, last);
    check (realValue = 10.0 and last = 6);
    R.Get ("2#1.01#E+2!", realValue, last);
    check (realValue = 5.0 and last = 10);
    R.Get ("-0.0", realValue, last);
    check (realValue = 0.0 and last = 4);
    E.Get (" green,red", colourValue, last);
    check (colourValue = Green and last = 6);
    C.Get ("'A'!", characterValue, last);
    check (characterValue = 'A' and last = 3);
    C.Get ("HT", characterValue, last);
    check (characterValue = Character'Val (9) and last = 2);
    begin
        C.Get ("'" & Character'Val (9) & "'", characterValue, last);
        raise Program_Error;
    exception
        when Data_Error => null;
    end;
    I.Put (outText, -42);
    check (outText = "     -42");
    R.Put (outText, 1.25, Aft => 2, Exp => 0);
    check (outText = "    1.25");
    E.Put (outText, Green, Lower_Case);
    check (outText = "green   ");
    C.Put (outText, 'A', Lower_Case);
    check (outText = "'A'     ");
    longText (longText'Last) := '7';
    I.Get (longText, integerValue, last);
    check (integerValue = 7 and last = longText'Last);
    Create (file);
    Put (file, " 12-34,16#FF# 1_2E2");
    New_Line (file);
    Put (file, "  123456");
    New_Line (file);
    Put (file, ".5-2.5,2#1.01#E+2");
    New_Line (file);
    Put (file, "Red,Green 'A' .125+1.25 18446744073709551615");
    New_Line (file);
    Put_Line (file, longText);
    Reset (file, In_File);
    I.Get (file, integerValue);
    check (integerValue = 12);
    I.Get (file, integerValue);
    check (integerValue = -34);
    Get (file, characterValue);
    check (characterValue = ',');
    I.Get (file, integerValue);
    check (integerValue = 255);
    I.Get (file, integerValue);
    check (integerValue = 1200);
    Skip_Line (file);
    I.Get (file, integerValue, Width => 5);
    check (integerValue = 123);
    I.Get (file, integerValue, Width => 3);
    check (integerValue = 456);
    R.Get (file, realValue);
    check (realValue = 0.5);
    R.Get (file, realValue);
    check (realValue = -2.5);
    Get (file, characterValue);
    check (characterValue = ',');
    R.Get (file, realValue);
    check (realValue = 5.0);
    E.Get (file, colourValue);
    check (colourValue = Red);
    Get (file, characterValue);
    check (characterValue = ',');
    E.Get (file, colourValue);
    check (colourValue = Green);
    C.Get (file, characterValue);
    check (characterValue = 'A');
    F.Get (file, fixedValue);
    check (fixedValue = 0.125);
    D.Get (file, decimalValue);
    check (decimalValue = 1.25);
    M.Get (file, modularValue);
    check (modularValue = Word'Last);
    I.Get (file, integerValue);
    check (integerValue = 7);
    Set_Input (file);
    begin
        I.Get (integerValue);
        raise Program_Error;
    exception
        when End_Error => null;
    end;
    Set_Input (Standard_Input);
    Close (file);
    for failure in 1 .. 5 loop
        begin
            case failure is
                when 1 => I.Get ("1__0", integerValue, last);
                when 2 => I.Get ("9223372036854775808", integerValue, last);
                when 3 => R.Get ("11.0", realValue, last);
                when 4 => R.Get ("1.0E+", realValue, last);
                when 5 => E.Get ("Red_Green", colourValue, last);
                when others => null;
            end case;
            raise Program_Error;
        exception
            when Data_Error => null;
        end;
    end loop;
    Create (file);
    Set_Line_Length (file, 4);
    Put (file, "xx");
    I.Put (file, 123, Width => 3);
    check (Line (file) = 2 and Col (file) = 4);
    begin
        I.Put (file, 12345, Width => 0);
        raise Program_Error;
    exception
        when Layout_Error => null;
    end;
    Close (file);
    declare
        text : String (1 .. 80);
    begin
        R.Put (text, 1.0, Aft => 70, Exp => 3);
        check (text (5 .. 6) = "1." and text (77 .. 80) = "E+00");
    end;
    declare
        value : Long_Float;
    begin
        Ada.Long_Float_Text_IO.Get ("16#1.8#E1", value, last);
        check (value = 24.0 and last = 9);
    end;
    Put_Line ("textinputgrammar: passed");
end TextInputGrammar;
